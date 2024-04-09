#ifndef GBCARTRIDGEGB_HPP
#define GBCARTRIDGEGB_HPP

#include "Cartridge.hpp"

#include <stdint.h>

enum GBCartridgeType {
  kGBCartridgeTypeUnknown = 0,
  kGBCartridgeTypeRomOnly,
  kGBCartridgeTypeMBC1,
  kGBCartridgeTypeMBC3,
  kGBCartridgeTypeCamera,
};

class CartridgeGB : public Cartridge {
  virtual bool isConnected() override;
public:
  CartridgeGB();
  virtual ~CartridgeGB();

#pragma mark public provider
  virtual uint8_t getSubType();
  virtual size_t readTitle(char *buf, size_t bufSize, bool *isColor) override;

  virtual uint32_t getROMSize() override;
  virtual uint32_t getRAMSize() override;

  virtual uint32_t read(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRaw(const void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t readROM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;

#pragma mark GB specifics
  uint8_t getGBTypeRaw();
  uint8_t getROMRevision();
  uint32_t getROMBankCount();
  uint32_t getRAMBankCount();
};

#endif // GBCARTRIDGEGB_HPP
