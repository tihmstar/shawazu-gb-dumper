
#include "CartridgeGBASaveEEPROM.hpp"
#include "gbarw.h"
#include "all.h"

#include <hardware/timer.h>
#include <hardware/clocks.h>
#include <hardware/sync.h>
#include <tusb.h>
#include <stdio.h>


#pragma mark GBA specifics private

#pragma mark public provider
uint32_t CartridgeGBASaveEEPROM::getRAMSize(){
  return 0x2000;
}

uint32_t CartridgeGBASaveEEPROM::readRAM(void *buf, uint32_t size, uint32_t offset){
  int err = 0;
  uint8_t *ptr = (uint8_t*)buf;
  uint32_t didread = 0;
  while (didread < size){
    uint64_t bdata = 0;
    cassure(eepromReadBlock(offset >> 3, &bdata) == sizeof(bdata));
    uint8_t boff = offset & 7;
    uint8_t bsize = 8-boff;
    memcpy(ptr, ((uint8_t*)&bdata)+boff,bsize);
    ptr += bsize;
    offset += bsize;
    didread += bsize;
  }
  
error:
  return didread;
}

uint32_t CartridgeGBASaveEEPROM::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  return 0;
}

int CartridgeGBASaveEEPROM::eraseRAM(){
  return 0;
}

#pragma mark GBA specifics public
int CartridgeGBASaveEEPROM::eepromReadBlock(uint16_t blocknum, uint64_t *out){
  if (blocknum > 0x3ff) return -1;
  return 0;
}
