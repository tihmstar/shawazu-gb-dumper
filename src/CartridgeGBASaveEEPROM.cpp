
#include "CartridgeGBASaveEEPROM.hpp"
#include "gbarw.h"
#include "all.h"

#include <hardware/timer.h>
#include <hardware/clocks.h>
#include <hardware/sync.h>
#include <tusb.h>
#include <stdio.h>


#define STORAGE_TYPE_4KBIT  0x41
#define STORAGE_TYPE_64KBIT 0x69

#pragma mark GBA specifics private

#pragma mark public provider
uint32_t CartridgeGBASaveEEPROM::getRAMSize(){
  uint8_t *stype = (uint8_t*)&_storage[sizeof(_storage)-1];
  switch (*stype){
  case STORAGE_TYPE_4KBIT:
    return 0x200;
  case STORAGE_TYPE_64KBIT:
    return 0x2000;  
  default:
    break;
  }

  uint64_t lastBlock = 0;
  for (size_t i = 0; i < 0x400; i++){
    uint64_t block = 0;
    if (eepromReadBlockLarge(i, &block) != 8) return 0;
    if ((i & 0xff) && lastBlock != block){
      *stype = STORAGE_TYPE_64KBIT;
      return 0x2000; 
    }else{
      tud_task();
    }
    lastBlock = block;
  }

  *stype = STORAGE_TYPE_4KBIT;
  return 0x200; 
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
  int err = 0;
  const uint8_t *ptr = (const uint8_t*)buf;
  uint32_t didwrite = 0;

  if (offset & 7){
    uint64_t bdata = 0;
    uint8_t boff = offset & 7;
    uint8_t bsize = 8-boff;
    cassure(eepromReadBlock(offset >> 3, &bdata) == sizeof(bdata));
    memcpy(((uint8_t*)&bdata)+boff,ptr,bsize);
    cassure(eepromWriteBlock(offset >> 3, bdata) == sizeof(bdata));
    ptr += bsize;
    offset += bsize;
    didwrite += bsize;
  }
  

  while (didwrite+8 <= size){
    uint64_t bdata = 0;
    memcpy(&bdata, ptr, sizeof(bdata));
    cassure(eepromWriteBlock(offset >> 3, bdata) == sizeof(bdata));
    ptr += sizeof(bdata);
    offset += sizeof(bdata);
    didwrite += sizeof(bdata);
  }

  if (didwrite < size){
    //unaligned post write
    uint64_t adata = 0;
    uint8_t asize = size - didwrite;
    cassure(eepromReadBlock(offset >> 3, &adata) == sizeof(adata));
    memcpy(&adata,ptr,asize);
    cassure(eepromWriteBlock(offset >> 3, adata) == sizeof(adata));
    ptr += asize;
    offset += asize;
    didwrite += asize;
  }  
  
error:
  return didwrite;
}

#pragma mark GBA specifics public
int CartridgeGBASaveEEPROM::eepromReadBlockSmall(uint16_t blocknum, uint64_t *out){
  int err = 0;
  if (blocknum > 0x3f) return -1;

  uint16_t eepromcmd[2+6+1] = {0x01, 0x01};
  uint16_t rbuf[68] = {};
  uint64_t ret = 0;

  for (int i=0; i<6; i++){
    eepromcmd[i+2] = ((blocknum >> 5) & 1);
    blocknum <<= 1;
  }

  cassure(gba_write(GBA_EEPROM_READ_ADDRESS, eepromcmd, sizeof(eepromcmd)) == sizeof(eepromcmd));
  cassure(gba_read(GBA_EEPROM_READ_ADDRESS, rbuf, sizeof(rbuf)) == sizeof(rbuf));
  for (size_t i = 0; i < 64; i++){
    ret <<= 1;
    ret |= rbuf[4+i] & 1;
  }
  
  if (out) *out = __builtin_bswap64(ret);

error:
  if (err){
    return 0;
  }
  return 8;
}

int CartridgeGBASaveEEPROM::eepromReadBlockLarge(uint16_t blocknum, uint64_t *out){
  int err = 0;
  if (blocknum > 0x3ff) return -1;

  uint16_t eepromcmd[2+14+1] = {0x01, 0x01};
  uint16_t rbuf[68] = {};
  uint64_t ret = 0;

  for (int i=0; i<10; i++){
    eepromcmd[i+2+4] = ((blocknum >> 9) & 1);
    blocknum <<= 1;
  }

  cassure(gba_write(GBA_EEPROM_READ_ADDRESS, eepromcmd, sizeof(eepromcmd)) == sizeof(eepromcmd));
  cassure(gba_read(GBA_EEPROM_READ_ADDRESS, rbuf, sizeof(rbuf)) == sizeof(rbuf));
  for (size_t i = 0; i < 64; i++){
    ret <<= 1;
    ret |= rbuf[4+i] & 1;
  }
  
  if (out) *out = __builtin_bswap64(ret);

error:
  if (err){
    return 0;
  }
  return 8;
}

int CartridgeGBASaveEEPROM::eepromReadBlock(uint16_t blocknum, uint64_t *out){
  if (getRAMSize() == 0x200){
    return eepromReadBlockSmall(blocknum, out);
  }else{
    return eepromReadBlockLarge(blocknum, out);
  }
}

int CartridgeGBASaveEEPROM::eepromWriteBlockSmall(uint16_t blocknum, uint64_t data){
  int err = 0;
  if (blocknum > 0x3f) return -1;

  uint16_t eepromcmd[2+6+64+1] = {0x01, 0x00};
  uint64_t oldData = 0;

  data = __builtin_bswap64(data);

  {
    /*
      Don't overwrite data if it's already there
    */
    if (eepromReadBlock(blocknum, &oldData) == 8 && oldData == data) return 8;
  }

  for (int i=0; i<6; i++){
    eepromcmd[i+2] = ((blocknum >> 5) & 1);
    blocknum <<= 1;
  }

  for (int i=0; i<64; i++){
    eepromcmd[i+2+6] = ((data >> 63) & 1);
    data <<= 1;
  }

  cassure(gba_write(GBA_EEPROM_READ_ADDRESS, eepromcmd, sizeof(eepromcmd)) == sizeof(eepromcmd));

  {
    uint64_t time = time_us_64();
    while (time_us_64() - time < 400*USEC_PER_MSEC){
      if (gba_read_byte(GBA_EEPROM_READ_ADDRESS) & 1) 
        return 8;
      tud_task();
    }
    return -3;
  }

error:
  return -2;
}

int CartridgeGBASaveEEPROM::eepromWriteBlockLarge(uint16_t blocknum, uint64_t data){
  int err = 0;
  if (blocknum > 0x3ff) return -1;

  uint16_t eepromcmd[2+14+64+1] = {0x01, 0x00};
  uint64_t oldData = 0;

  data = __builtin_bswap64(data);

  {
    /*
      Don't overwrite data if it's already there
    */
    if (eepromReadBlock(blocknum, &oldData) == 8 && oldData == data) return 8;
  }

  for (int i=0; i<10; i++){
    eepromcmd[i+2+4] = ((blocknum >> 9) & 1);
    blocknum <<= 1;
  }

  for (int i=0; i<64; i++){
    eepromcmd[i+2+4+10] = ((data >> 63) & 1);
    data <<= 1;
  }

  cassure(gba_write(GBA_EEPROM_READ_ADDRESS, eepromcmd, sizeof(eepromcmd)) == sizeof(eepromcmd));

  {
    uint64_t time = time_us_64();
    while (time_us_64() - time < 400*USEC_PER_MSEC){
      if (gba_read_byte(GBA_EEPROM_READ_ADDRESS) & 1) 
        return 8;
      tud_task();
    }
    return -3;
  }

error:
  return -2;
}

int CartridgeGBASaveEEPROM::eepromWriteBlock(uint16_t blocknum, uint64_t data){
  if (getRAMSize() == 0x200){
    return eepromWriteBlockSmall(blocknum, data);
  }else{
    return eepromWriteBlockLarge(blocknum, data);
  }
}
