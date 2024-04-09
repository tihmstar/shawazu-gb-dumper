#ifndef GBCARTRIDGEGBMBC1_HPP
#define GBCARTRIDGEGBMBC1_HPP

#include "CartridgeGB.hpp"

class CartridgeGBMBC1 : public CartridgeGB {
public:
  CartridgeGBMBC1();
  virtual ~CartridgeGBMBC1();

  virtual uint32_t readROM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;
};

#endif // GBCARTRIDGEGBMBC1_HPP