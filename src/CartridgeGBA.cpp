
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

  for (int i=0xA0; i<0xBC; i++){
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
  
  cassure(readROM(&dummy, sizeof(&dummy), 0x01004000) == sizeof(dummy));
  if (dummy == 0x20002000){
    /*
      Pokemon leaf green GER
    */
    *stype = GBA_ROM_SIZE_16M;
    goto reeval_loc;
  }

  {
    char buf[0x200] = {};
    tud_task();
    if (readROM(&buf, sizeof(buf), 0x800000) != sizeof(buf)) goto not_8m_rom;
    tud_task();

    for (size_t i = 0x10; i < sizeof(buf); i++){
      if (buf[i] != 0xFF) goto not_8m_rom;
    }

    tud_task();
    /*
      The first test fails to detect Pokemon leaf green GER
    */
    if (readROM(&buf, sizeof(buf), 0x00e00000) != sizeof(buf)) goto not_8m_rom;
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

#pragma mark GB specifics
const char *CartridgeGBA::getStorageType(){
  if (_subtype == kGBACartridgeTypeNotChecked){
    getSubType();
  }
  if (_subtype == kGBACartridgeTypeNotChecked) return "SUBTYPE ERROR";
  if (_subtype == kGBACartridgeTypeUnknown) return "Unknown";
  return (char*)_storage;
}