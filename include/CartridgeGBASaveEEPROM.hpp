#ifndef CARTRIDGEGBASAVEEEPROM_HPP
#define CARTRIDGEGBASAVEEEPROM_HPP

#include "CartridgeGBA.hpp"

#include <stdint.h>

class CartridgeGBASaveEEPROM : public  CartridgeGBA{
#pragma mark GBA specifics
  uint32_t writeRAMBank(const void *buf, uint32_t size, uint16_t offset = 0);
public:
  using CartridgeGBA::CartridgeGBA;

#pragma mark public provider
  virtual uint32_t getRAMSize() override;

  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;

#pragma mark GBA specifics
  int eepromReadBlockSmall(uint16_t blocknum, uint64_t *out);
  int eepromReadBlock(uint16_t blocknum, uint64_t *out);

  int eepromWriteBlockSmall(uint16_t blocknum, uint64_t data);
  int eepromWriteBlock(uint16_t blocknum, uint64_t data);
};

#endif // CARTRIDGEGBASAVEEEPROM_HPP
