#ifndef CARTRIDGEGBASAVEFLASH_HPP
#define CARTRIDGEGBASAVEFLASH_HPP

#include "CartridgeGBA.hpp"

#include <stdint.h>

class CartridgeGBASaveFlash : public  CartridgeGBA{
#pragma mark GBA specifics
  void flashSendCmd(uint8_t cmd);
public:
  using CartridgeGBA::CartridgeGBA;

#pragma mark public provider
  virtual uint32_t getRAMSize() override;

  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual int      eraseRAM() override;

#pragma mark GBA specifics
  uint16_t flashReadVIDandPID();
  int flashWriteByte(uint16_t offset, uint8_t data);
  int flashEraseSector(uint16_t sectorAddress);
  int flashEraseFullChip();
};

#endif // CARTRIDGEGBASAVEFLASH_HPP
