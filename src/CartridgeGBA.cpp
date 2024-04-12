
#include "CartridgeGBA.hpp"
#include "gbarw.h"
#include "all.h"

#include <stdio.h>

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
  return 0;
}

uint32_t CartridgeGBA::readROM(void *buf, uint32_t size, uint32_t offset){
  return gba_read(offset, buf, size);
}

uint32_t CartridgeGBA::readRAM(void *buf, uint32_t size, uint32_t offset){
  return gba_read(0x1000000 + offset, buf, size);
}

uint32_t CartridgeGBA::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  return 0;
}