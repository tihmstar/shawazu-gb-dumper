
#include "CartridgeGBASaveFlash.hpp"
#include "gbarw.h"
#include "all.h"

#include <hardware/timer.h>
#include <hardware/clocks.h>

#define FLAH_CMD_CHIPD_MODE_ENTER   0x90
#define FLAH_CMD_CHIPD_MODE_EXIT    0xF0

#define FLAH_CMD_ERASE_PREPARE      0x80
#define FLAH_CMD_ERASE_FULL_CHIP    0x10
#define FLAH_CMD_ERASE_4KIB_SECTOR  0x30

#define FLAH_CMD_WRITE_PREPARE      0xA0

#define FLAH_CMD_SET_BANK           0xB0


#pragma mark GBA specifics private
void CartridgeGBASaveFlash::flashSendCmd(uint8_t cmd){
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0x5555, 0xAA);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0x2aaa, 0x55);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0x5555, cmd);
}

#pragma mark public provider
uint32_t CartridgeGBASaveFlash::getRAMSize(){
  return _subtype == kGBACartridgeTypeSaveFlashExtended 
    ? 0x100000 
    : 0x10000;
}


uint32_t CartridgeGBASaveFlash::readRAM(void *buf, uint32_t size, uint32_t offset){
  // if (_subtype == kGBACartridgeTypeSaveFlashExtended){
  //   //TODO
  // }
  return CartridgeGBA::readRAM(buf, size, offset);
}

uint32_t CartridgeGBASaveFlash::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  uint8_t *ptr = (uint8_t*)buf;
  uint32_t didWriteBytes = 0;
  // if (_subtype == kGBACartridgeTypeSaveFlashExtended){
  //   //TODO
  // }
  if ((offset & 0xfff) == 0){
    while (size - didWriteBytes >= 0x1000){
      flashEraseSector(offset);
      for (size_t i = 0; i < 0x1000; i++){
        flashWriteByte(offset++,*ptr++);didWriteBytes++;
      }
    }
  }
  return didWriteBytes;
}

#pragma mark GBA specifics public
uint16_t CartridgeGBASaveFlash::flashReadVIDandPID(){
  uint16_t ret = 0;
  flashSendCmd(FLAH_CMD_CHIPD_MODE_ENTER);
  gba_read(GBA_SAVEGAME_MAP_ADDRESS + 0x00, &ret, 2);
  flashSendCmd(FLAH_CMD_CHIPD_MODE_EXIT);
  return ret;
}

int CartridgeGBASaveFlash::flashWriteByte(uint16_t offset, uint8_t data){
  flashSendCmd(FLAH_CMD_WRITE_PREPARE);
  gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + offset, data);
  uint64_t time = time_us_64();
  while (time_us_64() - time < 10*USEC_PER_MSEC){
    if (gba_read_byte(GBA_SAVEGAME_MAP_ADDRESS + offset) == data) return 0;
  }
  return -1;
}

int CartridgeGBASaveFlash::flashEraseSector(uint16_t sectorAddress){
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
