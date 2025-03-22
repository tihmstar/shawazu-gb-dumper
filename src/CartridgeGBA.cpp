
#include "CartridgeGBA.hpp"
#include "CartridgeGBASaveFlash.hpp"
#include "CartridgeGBASaveEEPROM.hpp"
#include "gbarw.h"
#include "all.h"

#include <new>
#include <stdio.h>
#include <string.h>
#include <hardware/timer.h>
#include <hardware/clocks.h>
#include <tusb.h>

#define GBA_ROM_SIZE_8M     0x81
#define GBA_ROM_SIZE_16M    0x16
#define GBA_ROM_SIZE_32M    0x32


#define GBA_RTC_ADDR_IO_PINS     0xC4
#define GBA_RTC_ADDR_IO_PINDIRS  0xC6
#define GBA_RTC_ADDR_IO_CONTROL  0xC8

#define GBA_RTC_PIN_SCK (1<<0)
#define GBA_RTC_PIN_SIO (1<<1)
#define GBA_RTC_PIN_CS  (1<<2)

#define GBA_RTC_PINMASK__CS_SIO_SCK(CS,SIO,SCK) (((CS!=0) << 2) | ((SIO!=0) << 1) | ((SCK!=0) << 0))

#define RTC_CMD_READ(x)   (((x)<<1) | 0x61)
#define RTC_CMD_WRITE(x)  (((x)<<1) | 0x60)

#define RTC_CMD_OP_STATUS_REGISTER_ACCESS 0b001
#define RTC_CMD_OP_REALTIME_DATA_ACCESS_1 0b010

#pragma mark CartridgeGBA
CartridgeGBA::CartridgeGBA(){
  _type = kCartridgeTypeGBA;
  gba_rw_init();
}

CartridgeGBA::~CartridgeGBA(){
  gba_rw_cleanup();
}

#pragma mark private provider

bool CartridgeGBA::isConnected(){
  int err = 0;
  uint8_t buf[0xc0] = {};
  uint8_t chksum = 0;
  cassure(readROM(buf, sizeof(buf), 0x00) == sizeof(buf));

  if (buf[0xB2] != 0x96) return false;

  for (int i=0xA0; i<=0xBC; i++){
    chksum -= buf[i];
  }
  chksum -= 0x19;

  return (chksum ^ buf[0xbd]) == 0;

error:
  return false;
}

#pragma mark public provider
uint8_t CartridgeGBA::getSubType(){
  int err = 0;
  uint8_t curtype = _subtype;
  char storageString[0x10-1] = {};

  if (_subtype == kGBACartridgeTypeNotChecked){
    char buf[0x1000] = {};
    uint32_t romsize = getROMSize();
    for (int i = 0; i < romsize; i+=sizeof(buf)){
      cassure(readROM(buf, sizeof(buf), i) == sizeof(buf));
      for (int z = 0; z<sizeof(buf); z+=4){
        uint32_t offset = 0;
        uint32_t v = *((uint32_t*)&buf[z]);
        switch (v){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"
          case 'RPEE': //EEPROM
          case 'MARS': //SRAM
          case 'SALF': //FLASH
#pragma GCC diagnostic pop
          {
            cassure(readROM(storageString, sizeof(storageString), i+z) == sizeof(storageString));
            if (strncmp(storageString, "EEPROM_V", sizeof("EEPROM_V")-1) == 0){
              curtype = kGBACartridgeTypeSaveEEPROM;
              _storage[sizeof("EEPROM_V")+3] = 0;
              goto type_search_end;
            } else if (strncmp(storageString, "SRAM_V", sizeof("SRAM_V")-1) == 0){
              curtype = kGBACartridgeTypeSaveSRAM;
              _storage[sizeof("SRAM_V")+3] = 0;
              goto type_search_end;
            } else if (strncmp(storageString, "FLASH_V", sizeof("FLASH_V")-1) == 0){
              curtype = kGBACartridgeTypeSaveFlash;
              _storage[sizeof("FLASH_V")+3] = 0;
              goto type_search_end;
            } else if (strncmp(storageString, "FLASH512_V", sizeof("FLASH512_V")-1) == 0){
              curtype = kGBACartridgeTypeSaveFlash;
              _storage[sizeof("FLASH512_V")+3] = 0;
              goto type_search_end;
            } else if (strncmp(storageString, "FLASH1M_V", sizeof("FLASH1M_V")-1) == 0){
              curtype = kGBACartridgeTypeSaveFlashExtended;
              _storage[sizeof("FLASH1M_V")+3] = 0;
              goto type_search_end;
            } else {
              curtype = kGBACartridgeTypeUnknown;
            } 
          }
          break;
        default:
          break;
        }
      }
    }
    type_search_end:;
  }
  
  if (_subtype != curtype){
    switch (curtype){
      case kGBACartridgeTypeSaveFlash:
      case kGBACartridgeTypeSaveFlashExtended:
        this->~Cartridge();
        new(this) CartridgeGBASaveFlash;
        break;

      case kGBACartridgeTypeSaveEEPROM:
        this->~Cartridge();
        new(this) CartridgeGBASaveEEPROM;
        break;

      case kGBACartridgeTypeUnknown:
      default:
        break;
    }
  }

error:
  _subtype = curtype;
  memcpy(_storage, storageString, MIN(sizeof(_storage),sizeof(storageString)));
  return _subtype;
}

size_t CartridgeGBA::readTitle(char *buf, size_t bufSize, bool *isColor){
  int err = 0;
  size_t ret = 0;
  char tmp[0x20] = {};

  cassure(readROM(tmp, sizeof(tmp), 0xa0) == sizeof(tmp));
  
  ret = snprintf(buf, bufSize, "%.12s (%.4s) (%.2s)",&tmp[0], &tmp[12], &tmp[16]);

  if (isColor) *isColor = false;

error:
  if (err){
    return 0;
  }
  return ret;
}

uint32_t CartridgeGBA::getROMSize(){
  int err = 0;
  uint32_t dummy = 0;
  uint8_t *stype = (uint8_t*)&_storage[sizeof(_storage)-1];
reeval_loc:
  switch (*stype){
  case GBA_ROM_SIZE_8M:
    return 0x0800000;
  case GBA_ROM_SIZE_16M:
    return 0x1000000;  
  case GBA_ROM_SIZE_32M:
    return 0x2000000;  
  default:
    break;
  }


  cassure(readROM(&dummy, sizeof(&dummy), 0x00800200) == sizeof(dummy));
  if (dummy == 0x01000100){
    *stype = GBA_ROM_SIZE_8M;
    goto reeval_loc;
  }
  
  {
    char buf[0x200] = {};
    tud_task();
    // if (readROM(&buf, sizeof(buf), 0x800000) != sizeof(buf)) goto not_8m_rom;
    // tud_task();

    // for (size_t i = 0x10; i < sizeof(buf); i++){
    //   if (buf[i] != 0xFF) goto not_8m_rom;
    // }

    // tud_task();
    // /*
    //   Detect Pokemon leaf green GER
    // */
    // if (readROM(&buf, sizeof(buf), 0x00e00000) != sizeof(buf)) goto not_8m_rom;
    // tud_task();

    // for (size_t i = 0x10; i < sizeof(buf); i++){
    //   if (buf[i] != 0xFF) goto not_8m_rom;
    // }

    /*
      Detect Pokemon leaf green JPN
    */
    if (readROM(&buf, sizeof(buf), 0x00d00000) != sizeof(buf)) goto not_8m_rom;
    tud_task();

    for (size_t i = 0x10; i < sizeof(buf); i++){
      if (buf[i] != 0xFF) goto not_8m_rom;
    }

    tud_task();
    *stype = GBA_ROM_SIZE_8M;
    goto reeval_loc;
    not_8m_rom:;
    tud_task();
  }

{
    char buf[0x200] = {};
    tud_task();
    if (readROM(&buf, sizeof(buf), 0x01004000) != sizeof(buf)) goto not_16m_rom;
    tud_task();

    for (size_t i = 0x10; i < sizeof(buf); i++){
      if (buf[i] != 0xFF) goto not_16m_rom;
    }

    tud_task();
    *stype = GBA_ROM_SIZE_16M;
    goto reeval_loc;
    not_16m_rom:;
    tud_task();
  }

error:
  *stype = GBA_ROM_SIZE_32M;
  goto reeval_loc;
}

uint32_t CartridgeGBA::getRAMSize(){
  return 0x10000;
}

uint32_t CartridgeGBA::readROM(void *buf, uint32_t size, uint32_t offset){
  return gba_read(offset, buf, size);
}

uint32_t CartridgeGBA::readRAM(void *buf, uint32_t size, uint32_t offset){
  uint32_t ramSize = getRAMSize();
  if (offset >= ramSize) return 0;
  if (size > ramSize - offset) size = ramSize - offset;  
  return gba_read(GBA_SAVEGAME_MAP_ADDRESS + offset, buf, size);
}

uint32_t CartridgeGBA::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  return gba_write(GBA_SAVEGAME_MAP_ADDRESS + offset, buf, size);
}


#pragma mark RTC
static void rtc_write_cmd(uint8_t byte){
  uint16_t word = byte << 1;
  for (int i = 7; i >= 0; i--){
    uint8_t b = (word >> i) & 2;
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | b);
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | b);
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | b);
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | b | GBA_RTC_PIN_SCK);
  }
}

static void rtc_write_data(uint8_t byte){
  uint16_t word = byte << 1;
  for (int i = 0; i < 8; i++){
    uint8_t b = (word >> i) & 2;
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | b);
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | b);
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | b);
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | b | GBA_RTC_PIN_SCK);
  }
}

static uint8_t rtc_read(){
  uint16_t ret = 0;
  for (size_t i = 0; i < 8; i++){
    for (int z = 0; z < 5; z++){
      gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS);
    }
    gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | 0 | GBA_RTC_PIN_SCK);
    uint8_t b = gba_read_byte(GBA_RTC_ADDR_IO_PINS);
		ret |= ((b & 2)<<i);
  }
  return ret >> 1;
}

static uint8_t enableRTC(){
  gba_write_byte(GBA_RTC_ADDR_IO_CONTROL, 1);
  gba_write_byte(GBA_RTC_ADDR_IO_PINS, 0 | 0 | GBA_RTC_PIN_SCK);
  gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | 0 | GBA_RTC_PIN_SCK);
  gba_write_byte(GBA_RTC_ADDR_IO_PINDIRS, GBA_RTC_PIN_CS | GBA_RTC_PIN_SIO | GBA_RTC_PIN_SCK);
  rtc_write_cmd(RTC_CMD_READ(RTC_CMD_OP_STATUS_REGISTER_ACCESS));
  gba_write_byte(GBA_RTC_ADDR_IO_PINDIRS, GBA_RTC_PIN_CS | 0 | GBA_RTC_PIN_SCK);
  uint8_t res = rtc_read();
  return res;
}

static void disableRTC(){
  gba_write_byte(GBA_RTC_ADDR_IO_CONTROL, 0);
}

int CartridgeGBA::readRTC(void *buf, size_t bufSize){
  int err = 0;
  uint8_t *ptr = (uint8_t*)buf;
  uint8_t status = enableRTC();

  if (bufSize > 7) ptr[7] = status;

  gba_write_byte(GBA_RTC_ADDR_IO_PINS, 0 | 0 | GBA_RTC_PIN_SCK);
  gba_write_byte(GBA_RTC_ADDR_IO_PINDIRS, GBA_RTC_PIN_CS | GBA_RTC_PIN_SIO | GBA_RTC_PIN_SCK);
  gba_write_byte(GBA_RTC_ADDR_IO_PINS, 0 | 0 | GBA_RTC_PIN_SCK);
  gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | 0 | GBA_RTC_PIN_SCK);

  rtc_write_cmd(RTC_CMD_READ(RTC_CMD_OP_REALTIME_DATA_ACCESS_1));
  gba_write_byte(GBA_RTC_ADDR_IO_PINDIRS, GBA_RTC_PIN_CS | 0 | GBA_RTC_PIN_SCK);

	for (int i=0; i<4 && i<bufSize; i++) ptr[i] = rtc_read();

  gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | 0 | GBA_RTC_PIN_SCK);

  for (int i=4; i<7 && i<bufSize; i++) ptr[i] = rtc_read();

error:
  disableRTC();
  if (err){
    return 0;
  }
  return 8;
}

int CartridgeGBA::writeRTC(const void *buf, size_t bufSize){
  int err = 0;
  uint8_t *ptr = (uint8_t*)buf;
  int i = 0;

  gba_write_byte(GBA_RTC_ADDR_IO_CONTROL, 1);
  gba_write_byte(GBA_RTC_ADDR_IO_PINS, 0 | 0 | GBA_RTC_PIN_SCK);
  gba_write_byte(GBA_RTC_ADDR_IO_PINS, GBA_RTC_PIN_CS | 0 | GBA_RTC_PIN_SCK);
  gba_write_byte(GBA_RTC_ADDR_IO_PINDIRS, GBA_RTC_PIN_CS | GBA_RTC_PIN_SIO | GBA_RTC_PIN_SCK);

  rtc_write_cmd(RTC_CMD_WRITE(RTC_CMD_OP_REALTIME_DATA_ACCESS_1));

  for (i=0; i<4 && i<bufSize; i++) rtc_write_data(ptr[i]);

  for (i=4; i<7 && i<bufSize; i++) rtc_write_data(ptr[i]);

error:
  disableRTC();
  if (err){
    return 0;
  }
  return i;
}

bool CartridgeGBA::hasRTC(){
  bool ret = enableRTC() == 0x40;
  disableRTC();
  return true;
}

#pragma mark GBA specifics
const char *CartridgeGBA::getStorageType(){
  if (_subtype == kGBACartridgeTypeNotChecked){
    getSubType();
  }
  if (_subtype == kGBACartridgeTypeNotChecked) return "SUBTYPE ERROR";
  if (_subtype == kGBACartridgeTypeUnknown) return "Unknown";
  return (char*)_storage;
}