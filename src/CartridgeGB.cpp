
#include "CartridgeGB.hpp"
#include "gbrw.h"
#include "all.h"

#include "CartridgeGBMBC1.hpp"
#include "CartridgeGBMBC3.hpp"
#include "CartridgeGBMBC5.hpp"

#include "pico/stdlib.h"
#include <stdio.h>
#include <new>

#pragma mark CartridgeGB
CartridgeGB::CartridgeGB(){
  _type = kCartridgeTypeGB;
  _subtype = kGBCartridgeTypeUnknown;
  gb_rw_init();
  gb_read(0x140, _storage, sizeof(_storage));
}

CartridgeGB::~CartridgeGB(){
  gb_rw_cleanup();
}

#pragma mark private provider

bool CartridgeGB::isConnected(){
  int err = 0;
  uint8_t buf[0x1a] = {};
  uint8_t chksum = 0;
  cassure(readROM(buf, sizeof(buf), 0x134) == sizeof(buf));

  for (int i=0; i<0x19; i++){
    chksum -= buf[i] + 1;
  }

  if ((chksum ^ buf[0x19]) == 0){
    gpio_put(MINI_VOLTAGE_CTRL_PIN, 1);
    return true;
  }
error:
  return false;
}

#pragma mark public provider
uint8_t CartridgeGB::getSubType(){
  int err = 0;
  uint8_t curtype;
  uint8_t raw = 0xFF;
  readROM(&raw, 1, 0x147);
  switch (raw) {
      case 0x00:
          curtype = kGBCartridgeTypeRomOnly;
          break;

      case 0x01:
      case 0x02:
      case 0x03:
          curtype = kGBCartridgeTypeMBC1;
          break;

      case 0x0F:
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
          curtype = kGBCartridgeTypeMBC3;
          break;

      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
          curtype = kGBCartridgeTypeMBC5;
          break;

      case 0xFC:
          curtype = kGBCartridgeTypeCamera;
          break;

      default:
          curtype = kGBCartridgeTypeUnknown;
          break;
  }

  if (_subtype != curtype){
    switch (curtype){
      case kGBCartridgeTypeMBC1:
        this->~Cartridge();
        new(this) CartridgeGBMBC1;
        break;

      case kGBCartridgeTypeCamera:
      case kGBCartridgeTypeMBC3:
        this->~Cartridge();
        new(this) CartridgeGBMBC3;
        break;

      case kGBCartridgeTypeMBC5:
        this->~Cartridge();
        new(this) CartridgeGBMBC5;
        break;

      case kGBCartridgeTypeRomOnly:
      case kGBCartridgeTypeUnknown:
      default:
        break;
    }
  }

  _subtype = curtype;
  return _subtype;

error:
  return kGBCartridgeTypeUnknown;
}

size_t CartridgeGB::readTitle(char *buf, size_t bufSize, bool *isColor){
  int err = 0;
  size_t ret = 0;
  bool color = false;

  if (bufSize > 17) bufSize = 17;  
  cassure((ret = readROM(buf, bufSize, 0x134)) == bufSize);
  buf[ret] = '\0';

  if (ret >= 16) {
      if (buf[15] == 0x80 || buf[15] == 0xC0) {
          buf[15] = '\0';
          color = true;
      }
  }

  if (isColor) *isColor = color;

  return ret;
error:
  return 0;
}

uint32_t CartridgeGB::getROMSize(){
  return getROMBankCount() << 14;
}

uint32_t CartridgeGB::getRAMSize(){
  return getRAMBankCount() << 13;
}

uint32_t CartridgeGB::read(void *buf_, uint32_t size, uint32_t offset){
  uint32_t didread = 0;
  uint8_t *buf = (uint8_t*)buf_;

  if (offset < 0xA000){
    uint32_t wantRead = size;
    if (wantRead + offset > 0xA000){
      wantRead = 0xA000 - offset;
    }
    didread = readROM(buf, wantRead, offset);
    buf += didread;
    offset += didread;
    size -= didread;
  }
  
  didread += readRAM(buf, size, offset);
  return didread;
}

uint32_t CartridgeGB::writeRaw(const void *buf, uint32_t size, uint32_t offset){
  return gb_write(offset, buf, size);
}

uint32_t CartridgeGB::readROM(void *buf, uint32_t size, uint32_t offset){
  return gb_read(offset, buf, size);
}

uint32_t CartridgeGB::readRAM(void *buf, uint32_t size, uint32_t offset){
  return 0;
}

uint32_t CartridgeGB::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  return 0;
}

#pragma mark GB specifics
uint8_t CartridgeGB::getGBTypeRaw(){
  return _storage[0x7]; //0x147
}

uint8_t CartridgeGB::getROMRevision(){
  return _storage[0xc]; //0x14c
}

uint32_t CartridgeGB::getROMBankCount(){
  return 2 << _storage[0x8]; //0x148
}

uint32_t CartridgeGB::getRAMBankCount(){
  uint8_t val = _storage[0x9]; //0x149
  switch (val) {
    case 0x00:
        return 0;
    case 0x01:
        return 0;
    case 0x02:
        return 1;
    case 0x03:
        return 4;
    case 0x04:
        return 16;
    case 0x05:
        return 8;
    default:
        return 0;
  }
}