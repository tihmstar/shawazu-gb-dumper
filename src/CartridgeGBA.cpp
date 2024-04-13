
#include "CartridgeGBA.hpp"
#include "gbarw.h"
#include "all.h"

#include <stdio.h>
#include <hardware/timer.h>
#include <hardware/clocks.h>

#define FLAH_CMD_CHIPD_MODE_ENTER   0x90
#define FLAH_CMD_CHIPD_MODE_EXIT    0xF0

#define FLAH_CMD_ERASE_PREPARE      0x80
#define FLAH_CMD_ERASE_FULL_CHIP    0x10
#define FLAH_CMD_ERASE_4KIB_SECTOR  0x30

#define FLAH_CMD_WRITE_PREPARE      0xA0

#define FLAH_CMD_SET_BANK           0xB0

#pragma mark CartridgeGBA
CartridgeGBA::CartridgeGBA(){
  _type = kCartridgeTypeGBA;
  gba_rw_init();
}

CartridgeGBA::~CartridgeGBA(){
  gba_rw_cleanup();
}

#pragma mark private provider

bool CartridgeGBA::isConnected(){
  int err = 0;
  uint8_t buf[0xc0] = {};
  uint8_t chksum = 0;
  cassure(readROM(buf, sizeof(buf), 0x00) == sizeof(buf));

  if (buf[0xB2] != 0x96) return false;

  for (int i=0xA0; i<0xBC; i++){
    chksum -= buf[i];
  }
  chksum -= 0x19;

  return (chksum ^ buf[0xbd]) == 0;

error:
  return false;
}

#pragma mark public provider
size_t CartridgeGBA::readTitle(char *buf, size_t bufSize, bool *isColor){
  int err = 0;
  size_t ret = 0;
  char tmp[0x20] = {};

  cassure(readROM(tmp, sizeof(tmp), 0xa0) == sizeof(tmp));
  
  ret = snprintf(buf, bufSize, "%.12s (%.4s) (%.2s)",&tmp[0], &tmp[12], &tmp[16]);

  if (isColor) *isColor = false;

error:
  if (err){
    return 0;
  }
  return ret;
}

uint32_t CartridgeGBA::getROMSize(){
  int err = 0;
  uint32_t dummy = 0;

  cassure(readROM(&dummy, sizeof(&dummy), 0x00800200) == sizeof(dummy));
  if (dummy == 0x01000100) return 0x800000;

error:
  return 0x1000000;
}

uint32_t CartridgeGBA::getRAMSize(){
  return 0x10000;
}

uint32_t CartridgeGBA::readROM(void *buf, uint32_t size, uint32_t offset){
  return gba_read(offset, buf, size);
}

uint32_t CartridgeGBA::readRAM(void *buf, uint32_t size, uint32_t offset){
  return gba_read(GBA_SAVEGAME_MAP_ADDRESS + offset, buf, size);
}

uint32_t CartridgeGBA::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  return gba_write(GBA_SAVEGAME_MAP_ADDRESS + offset, buf, size);
}

#pragma mark GBA specifics private
void CartridgeGBA::flashSendCmd(uint8_t cmd){
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0x5555, 0xAA);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0x2aaa, 0x55);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0x5555, cmd);
}

#pragma mark GBA specifics public
uint16_t CartridgeGBA::flashReadVIDandPID(){
  uint16_t ret = 0;
  flashSendCmd(FLAH_CMD_CHIPD_MODE_ENTER);
  gba_read(GBA_SAVEGAME_MAP_ADDRESS + 0x00, &ret, 2);
  flashSendCmd(FLAH_CMD_CHIPD_MODE_EXIT);
  return ret;
}

void CartridgeGBA::flashWriteByte(uint16_t offset, uint8_t data){
  flashSendCmd(FLAH_CMD_WRITE_PREPARE);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + offset, data);
}

int CartridgeGBA::flashEraseSector(uint16_t sectorAddress){
  sectorAddress &= 0xf000;
  flashSendCmd(FLAH_CMD_ERASE_PREPARE);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0x5555, 0xAA);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0x2aaa, 0x55);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + sectorAddress, FLAH_CMD_ERASE_4KIB_SECTOR);
  uint64_t time = time_us_64();
  while (time_us_64() - time < 100*USEC_PER_MSEC){
    if (gba_read_byte(GBA_SAVEGAME_MAP_ADDRESS + sectorAddress) == 0xff) return 0;
  }
  return -1;
}
