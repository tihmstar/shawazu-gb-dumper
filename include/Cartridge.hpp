#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// #define CARTRIDGE_TITLE_SIZE 17 // 16 bytes + NULL terminator

enum CartridgeType {
  kCartridgeTypeNone = 0,
  kCartridgeTypeGB = 1,
  kCartridgeTypeGBA = 2
};

class Cartridge {
protected:
  CartridgeType _type;
  uint8_t _subtype;
  uint8_t _storage[0x10];

private:
  virtual bool isConnected();
public:
  Cartridge();
  virtual ~Cartridge();

  CartridgeType getType();

  virtual uint8_t getSubType();
  virtual size_t readTitle(char *buf, size_t bufSize, bool *isColor);

  virtual uint32_t getROMSize();
  virtual uint32_t getRAMSize();

  virtual uint32_t read(void *buf, uint32_t size, uint32_t offset = 0);
  virtual uint32_t writeRaw(const void *buf, uint32_t size, uint32_t offset = 0);

  virtual uint32_t readROM(void *buf, uint32_t size, uint32_t offset = 0);
  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0);
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0);
  virtual int      eraseRAM();
  virtual int      readRTC(void *buf, size_t bufSize);
  virtual int      writeRTC(const void *buf, size_t bufSize);
  virtual bool     hasRTC();
};

#endif // GBCARTRIDGE_HPP
