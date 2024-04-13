#ifndef CARTRIDGEGBA_HPP
#define CARTRIDGEGBA_HPP

#include "Cartridge.hpp"

#include <stdint.h>

enum GBCartridgeType {
  kGBACartridgeTypeUnknown = 0,
  kGBACartridgeTypeNoSave,
  kGBACartridgeTypeSaveSRAM,
  kGBACartridgeTypeSaveFlash,
  kGBACartridgeTypeSaveFlashExtended,
  kGBACartridgeTypeSaveEEPROM,
};

class CartridgeGBA : public  Cartridge{
  virtual bool isConnected() override;

#pragma mark GBA specifics
  void flashSendCmd(uint8_t cmd);
public:
  CartridgeGBA();
  virtual ~CartridgeGBA();

#pragma mark public provider
  virtual size_t readTitle(char *buf, size_t bufSize, bool *isColor) override;

  virtual uint32_t getROMSize() override;
  virtual uint32_t getRAMSize() override;

  virtual uint32_t readROM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;

#pragma mark GBA specifics
  uint16_t flashReadVIDandPID();
  void flashWriteByte(uint16_t offset, uint8_t data);
  int flashEraseSector(uint16_t sectorAddress);

};

#endif // GBCARTRIDGEGBA_HPP
