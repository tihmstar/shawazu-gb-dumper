
#include "Cartridge.hpp"
#include "CartridgeGB.hpp"
#include "CartridgeGBA.hpp"

#include <utility>
#include <new>

#pragma mark Cartridge
Cartridge::Cartridge()
: _type{kCartridgeTypeNone}, _subtype{0}
, _storage{}
{
  //
}

Cartridge::~Cartridge(){
  //
}

bool Cartridge::isConnected(){
  return false;
}

#pragma mark selfimplement
CartridgeType Cartridge::getType(){
  if (isConnected()) return _type;

  /*
    Check GBA
  */
  this->~Cartridge();
  new(this) CartridgeGBA;
  if (isConnected()){
    getSubType(); //morph to correct object!
    return _type;
  }

  /*
    Check GB
  */
  this->~Cartridge();
  new(this) CartridgeGB;
  if (isConnected()){
    getSubType(); //morph to correct object!
    return _type;
  }

  /*
    Fallback to no cartridge
  */
  this->~Cartridge();
  new(this) Cartridge;
  return _type;
}

uint8_t Cartridge::getSubType(){
  return _subtype;
}


#pragma mark public dummies
size_t Cartridge::readTitle(char *buf, size_t bufSize, bool *isColor){
  return 0;
}

uint32_t Cartridge::getROMSize(){
  return 0;
}

uint32_t Cartridge::getRAMSize(){
  return 0;
}

uint32_t Cartridge::read(void *buf, uint32_t size, uint32_t offset){
  return 0;
}

uint32_t Cartridge::writeRaw(const void *buf, uint32_t size, uint32_t offset){
  return 0;
}

uint32_t Cartridge::readROM(void *buf, uint32_t size, uint32_t offset){
  return 0;
}

uint32_t Cartridge::readRAM(void *buf, uint32_t size, uint32_t offset){
  return 0;
}

uint32_t Cartridge::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  return 0;
}

int Cartridge::eraseRAM(){
  return -99;
}

int Cartridge::readRTC(void *buf, size_t bufSize){
  return -99;
}

int Cartridge::writeRTC(const void *buf, size_t bufSize){
  return -99;
}
