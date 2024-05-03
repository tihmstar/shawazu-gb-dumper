
#include "CartridgeGBMBC3.hpp"
#include "all.h"
#include "gbrw.h"

#include <pico/stdio.h>
#include <pico/stdlib.h>

#pragma mark public provider
uint32_t CartridgeGBMBC3::readROM(void *buf_, uint32_t size, uint32_t offset){
uint8_t *buf = (uint8_t*)buf_;
  // Implementation specific to MBC3
  uint32_t bankCount = getROMBankCount();

  uint32_t readSize = 0;
  for (uint32_t i = offset >> 14; i < bankCount; i++) {
      if (readSize >= size) {
          break;
      }

      gb_write_byte(0x2000, i);

      uint32_t offsetInBank = offset & 0x3FFF;
      uint32_t toRead = (16 * 1024) - offsetInBank;
      if (size - readSize < toRead) {
          toRead = size - readSize;
      }

      if (__builtin_expect(i == 0, 0))
          gb_read(0x0 + offsetInBank, buf, toRead);
      else
          gb_read(0x4000 + offsetInBank, buf, toRead);

      readSize += toRead;
      buf += toRead;
      offset = 0;
  }

  // Reset bank selection
  gb_write_byte(0x2000, 0x0);

  return readSize;
}

uint32_t CartridgeGBMBC3::readRAM(void *buf_, uint32_t size, uint32_t offset){
  uint8_t *buf = (uint8_t*)buf_;
  uint32_t ramBankCount = getRAMBankCount();

  if (ramBankCount == 0) {
      return 0;
  }

  // Enable RAM
  gb_write_byte(0x0000, 0x0A);

  uint32_t readSize = 0;
  for (uint32_t i = offset >> 13; i < ramBankCount; i++) {
      if (readSize >= size) {
          break;
      }

      gb_write_byte(0x4000, i);

      uint32_t offsetInBank = offset & 0x1FFF;
      uint32_t toRead = (8 * 1024) - offsetInBank;
      if (size - readSize < toRead) {
          toRead = size - readSize;
      }

      gb_read(0xA000 + offsetInBank, buf, toRead);

      readSize += toRead;
      buf += toRead;
      offset = 0;
  }

  // Disable RAM
  gb_write_byte(0x0000, 0x00);

  // Reset bank selection
  gb_write_byte(0x4000, 0x0);

  return readSize;
}

uint32_t CartridgeGBMBC3::writeRAM(const void *buf_, uint32_t size, uint32_t offset){
  const uint8_t *buf = (uint8_t*)buf_;
  uint32_t ramBankCount = getRAMBankCount();

  if (ramBankCount == 0) {
      return 0;
  }

  // Enable RAM
  gb_write_byte(0x0000, 0x0A);

  uint32_t writeSize = 0;
  for (uint32_t i = offset >> 13; i < ramBankCount; i++) {
      if (writeSize >= size) {
          break;
      }

      gb_write_byte(0x4000, i);

      uint32_t offsetInBank = offset & 0x1FFF;
      uint32_t toWrite = (8 * 1024) - offsetInBank;
      if (size - writeSize < toWrite) {
          toWrite = size - writeSize;
      }

      gb_write(0xA000 + offsetInBank, buf, toWrite);

      writeSize += toWrite;
      buf += toWrite;
      offset = 0;
  }

  // Disable RAM
  gb_write_byte(0x0000, 0x00);

  // Reset bank selection
  gb_write_byte(0x4000, 0x0);

  return writeSize;
}

int CartridgeGBMBC3::readRTC(void *buf, size_t bufSize){
  int err = 0;
  /*
    IMPORTANT: DO NOT INTERRUPT THIS FUNCTION WITH USB CALLBACKS OR RAM WILL GET CORRUPTED!!!!
  */

  uint8_t *ptr = (uint8_t*)buf;
  int didRead = 0;

  // Enable RTC access
  gb_write_byte(0x0000, 0x0A);

  //perform latch
  gb_write_byte(0x6000, 0x00);
  gb_write_byte(0x6000, 0x01);

  for (didRead = 0; didRead < 5 && didRead < bufSize; didRead++){
    gb_write_byte(0x4000, 0x08 + didRead);
    sleep_ms(4);
    ptr[didRead] = gb_read_byte(0xA000);
    sleep_ms(4);
  }

error:
  // Disable RTC access
  gb_write_byte(0x0000, 0x00);

  // Reset bank selection
  gb_write_byte(0x4000, 0x00);

  if (err){
    return -err;
  }
  return didRead;
}

int CartridgeGBMBC3::writeRTC(const void *buf, size_t bufSize){
  int err = 0;
  /*
    IMPORTANT: DO NOT INTERRUPT THIS FUNCTION WITH USB CALLBACKS OR RAM WILL GET CORRUPTED!!!!
  */

  const uint8_t *ptr = (const uint8_t*)buf;
  int didWrite = 0;

  // Enable RTC access
  gb_write_byte(0x0000, 0x0A);

  //perform latch
  gb_write_byte(0x6000, 0x00);
  gb_write_byte(0x6000, 0x01);

  {
    //stop RTC
    gb_write_byte(0x4000, 0x0C);
    uint8_t ctrlByte = gb_read_byte(0xA000);
    ctrlByte |= 0x40;
    sleep_ms(4);
    gb_write_byte(0xA000,ctrlByte);
    sleep_ms(4);
  }

  for (didWrite = 0; didWrite < 5 && didWrite < bufSize; didWrite++){
    gb_write_byte(0x4000, 0x08 + didWrite);
    sleep_ms(4);
    gb_write_byte(0xA000, ptr[didWrite]);
    sleep_ms(4);
  }

error:
  // Disable RTC access
  gb_write_byte(0x0000, 0x00);

  // Reset bank selection
  gb_write_byte(0x4000, 0x00);
  if (err){
    return -err;
  }
  return didWrite;
}
