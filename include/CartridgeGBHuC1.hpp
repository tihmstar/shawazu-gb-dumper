#ifndef GBCARTRIDGEGBHUC1_HPP
#define GBCARTRIDGEGBHUC1_HPP

#include "CartridgeGB.hpp"

class CartridgeGBHuC1 : public CartridgeGB {
public:
  using CartridgeGB::CartridgeGB;
  
  virtual uint32_t readROM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;
};

#endif // GBCARTRIDGEGBHUC1_HPP