#ifndef GBCARTRIDGEGBMBC3_HPP
#define GBCARTRIDGEGBMBC3_HPP

#include "CartridgeGB.hpp"

class CartridgeGBMBC3 : public CartridgeGB {
public:
  using CartridgeGB::CartridgeGB;

  virtual uint32_t readROM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t readRAM(void *buf, uint32_t size, uint32_t offset = 0) override;
  virtual uint32_t writeRAM(const void *buf, uint32_t size, uint32_t offset = 0) override;

  virtual int      readRTC(void *buf, size_t bufSize) override;
  virtual int      writeRTC(const void *buf, size_t bufSize);
};
#endif // GBCARTRIDGEGBMBC3_HPP