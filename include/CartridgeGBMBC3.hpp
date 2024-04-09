#ifndef GBCARTRIDGEGBMBC3_HPP
#define GBCARTRIDGEGBMBC3_HPP

#include "CartridgeGB.hpp"

class CartridgeGBMBC3 : public CartridgeGB {
public:
  CartridgeGBMBC3();
  virtual ~CartridgeGBMBC3();

  virtual uint32_t readROM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;
};
#endif // GBCARTRIDGEGBMBC3_HPP