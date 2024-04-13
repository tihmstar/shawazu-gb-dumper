
#include "CartridgeGBA.hpp"
#include "CartridgeGBASaveFlash.hpp"
#include "gbarw.h"
#include "all.h"

#include <new>
#include <stdio.h>
#include <string.h>
#include <hardware/timer.h>
#include <hardware/clocks.h>


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
  uint8_t curtype = kGBACartridgeTypeUnknown;
  char storageString[0x10] = {};

  {
    char buf[0x1000] = {};
    for (int i = 0; i < getROMSize(); i+=sizeof(buf)){
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

  cassure(readROM(&dummy, sizeof(&dummy), 0x00800200) == sizeof(dummy));
  if (dummy == 0x01000100) return 0x800000;

error:
  return 0x1000000;
}

uint32_t CartridgeGBA::getRAMSize(){
  return 0x10000;
}

uint32_t CartridgeGBA::readROM(void *buf, uint32_t size, uint32_t offset){
  return gba_read(offset, buf, size);
}

uint32_t CartridgeGBA::readRAM(void *buf, uint32_t size, uint32_t offset){
  return gba_read(GBA_SAVEGAME_MAP_ADDRESS + offset, buf, size);
}

uint32_t CartridgeGBA::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  return gba_write(GBA_SAVEGAME_MAP_ADDRESS + offset, buf, size);
}

#pragma mark GB specifics
const char *CartridgeGBA::getStorageType(){
  if (_subtype == kGBACartridgeTypeUnknown) return "Unknown";
  return (char*)_storage;
}