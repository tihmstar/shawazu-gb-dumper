
#include "CartridgeGBASaveFlash.hpp"
#include "gbarw.h"
#include "all.h"

#include <hardware/timer.h>
#include <hardware/clocks.h>
#include <hardware/sync.h>
#include <tusb.h>
#include <stdio.h>

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
    ? 0x20000 
    : 0x10000;
}


uint32_t CartridgeGBASaveFlash::readRAM(void *buf, uint32_t size, uint32_t offset){
  if (_subtype == kGBACartridgeTypeSaveFlashExtended){
    uint8_t *ptr = (uint8_t*)buf;
    uint32_t didRead = 0;
    if (offset < 0x10000){
      flashSendCmd(FLAH_CMD_SET_BANK);
      gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0, 0);
      uint32_t needsRead = 0x10000 - offset;
      if (needsRead > size) needsRead = size;
      uint32_t actuallyRead = CartridgeGBA::readRAM(ptr, needsRead, offset);
      if (actuallyRead != needsRead) return actuallyRead;
      didRead += actuallyRead;
      offset += actuallyRead;
      ptr += actuallyRead;
      size -= actuallyRead;
    }

    if (offset >= 0x10000 && size){
      flashSendCmd(FLAH_CMD_SET_BANK);
      gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0, 1);
      uint32_t needsRead = 0x20000 - offset;
      if (needsRead > size) needsRead = size;
      uint32_t actuallyRead = CartridgeGBA::readRAM(ptr, needsRead, offset);
      if (actuallyRead != needsRead) return actuallyRead;
      didRead += actuallyRead;
    }
    
    return didRead;
  }else{
    return CartridgeGBA::readRAM(buf, size, offset);
  }
}

uint32_t CartridgeGBASaveFlash::writeRAMBank(const void *buf, uint32_t size, uint16_t offset){
  uint8_t *ptr = (uint8_t*)buf;
  uint32_t didWriteBytes = 0;
  if ((offset & 0xfff) == 0){
    while (size - didWriteBytes >= 0x1000){
      bool needsSectorErase = false;
      uint8_t sector[0x1000] = {};
      uint32_t didRead = gba_read(GBA_SAVEGAME_MAP_ADDRESS + offset, sector, sizeof(sector));
      tud_task();
      for (int i = 0; i < sizeof(sector); i++){
        if ((sector[i] & ptr[i]) < ptr[i]){
          needsSectorErase = true;
          break;
        }
      }
      if (needsSectorErase || didRead != sizeof(sector)){
        flashEraseSector(offset);
      }
      for (size_t i = 0; i < 0x1000; i++){
        if (flashWriteByte(offset,*ptr) < 0){
          if (flashWriteByte(offset,*ptr) < 0){
            if (flashWriteByte(offset,*ptr) < 0){
              return didWriteBytes;
            }
          }
        }
        offset++;
        ptr++;
        didWriteBytes++;
      }
    }
  }
  return didWriteBytes;
}

uint32_t CartridgeGBASaveFlash::writeRAM(const void *buf, uint32_t size, uint32_t offset){
  if (_subtype == kGBACartridgeTypeSaveFlashExtended){
    const uint8_t *ptr = (const uint8_t*)buf;
    uint32_t didWrite = 0;
    if (offset < 0x10000){
      flashSendCmd(FLAH_CMD_SET_BANK);
      gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0, 0);
      uint32_t needsWrite = 0x10000 - offset;
      if (needsWrite > size) needsWrite = size;
      uint32_t actuallyWritten = writeRAMBank(ptr, needsWrite, offset);
      didWrite += actuallyWritten;
      offset += actuallyWritten;
      ptr += actuallyWritten;
      size -= actuallyWritten;
    }

    if (offset >= 0x10000 && offset < 0x20000 && size){
      flashSendCmd(FLAH_CMD_SET_BANK);
      gba_write_byte(GBA_SAVEGAME_MAP_ADDRESS + 0, 1);
      uint32_t needsWrite = 0x20000 - offset;
      if (needsWrite > size) needsWrite = size;
      uint32_t actuallyWritten = writeRAMBank(ptr, needsWrite, offset);
      didWrite += actuallyWritten;
    }
    
    return didWrite;
  }else{
    uint32_t didWrite = writeRAMBank(buf, size, offset);
    return didWrite;
  }
}

int CartridgeGBASaveFlash::eraseRAM(){
  return flashEraseFullChip();
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
  while (time_us_64() - time < 100*USEC_PER_MSEC){
    if (gba_read_byte(GBA_SAVEGAME_MAP_ADDRESS + offset) == data) return 0;
    tud_task();
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
  while (time_us_64() - time < 300*USEC_PER_MSEC){
    if (gba_read_byte(GBA_SAVEGAME_MAP_ADDRESS + sectorAddress) == 0xff) return 0;
    tud_task();
  }
  return -1;
}

int CartridgeGBASaveFlash::flashEraseFullChip(){
  flashSendCmd(FLAH_CMD_ERASE_PREPARE);
  flashSendCmd(FLAH_CMD_ERASE_FULL_CHIP);
  uint64_t time = time_us_64();
  while (time_us_64() - time < 400*USEC_PER_MSEC){
    if (gba_read_byte(GBA_SAVEGAME_MAP_ADDRESS + 0) == 0xff) return 0;
    tud_task();
  }
  return -1;
}
