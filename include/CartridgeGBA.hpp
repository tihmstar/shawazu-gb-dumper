#ifndef CARTRIDGEGBA_HPP
#define CARTRIDGEGBA_HPP

#include "Cartridge.hpp"

#include <stdint.h>

enum GBACartridgeType {
  kGBACartridgeTypeUnknown = 0,
  kGBACartridgeTypeSaveSRAM,
  kGBACartridgeTypeSaveFlash,
  kGBACartridgeTypeSaveFlashExtended,
  kGBACartridgeTypeSaveEEPROM,
};

class CartridgeGBA : public  Cartridge{
  virtual bool isConnected() override;
public:
  CartridgeGBA();
  virtual ~CartridgeGBA();

#pragma mark public provider
  virtual uint8_t getSubType() override;
  virtual size_t readTitle(char *buf, size_t bufSize, bool *isColor) override;

  virtual uint32_t getROMSize() override;
  virtual uint32_t getRAMSize() override;

  virtual uint32_t readROM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;

#pragma mark GB specifics
  const char *getStorageType();
};

#endif // GBCARTRIDGEGBA_HPP
