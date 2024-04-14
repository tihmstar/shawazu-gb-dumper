#include "bsp/board.h"
#include "tusb.h"
#include "class/msc/msc.h"
#include "pico.h"
#include "fakefiles.hpp"
#include "all.h"
#include "pico/stdlib.h"
#include "Cartridge.hpp"
#include "CartridgeGB.hpp"
#include "EmuFATFS.hpp"
#include <hardware/watchdog.h>

extern Cartridge *gCart;

static tihmstar::EmuFATFS<35,0x400> gEmuFat("SHAWAZU");

// whether host does safe-eject
static bool ejected = false;
static bool fatIsInited = false;
static bool gSaveRamWasDeleted = false;
static int gIsIOInProgress = 0;

int32_t cb_read_info(uint32_t offset, void *buf, uint32_t size, const char *filename){
  size_t infoSize = 0;
  const char *info = fakefiles_read_infotxt(gCart, &infoSize);
  if (!info) return 0;
  if (offset > infoSize) return 0;
  info += offset;
  infoSize -= offset;
  if (size > infoSize) size = infoSize;
  memcpy(buf, info, size);
  return size;
}

int32_t cb_read_rom(uint32_t offset, void *buf, uint32_t size, const char *filename){
  return gCart->readROM(buf, size, offset);
}

static uint8_t gRTCmem[0x10] = {};

int32_t cb_read_ram(uint32_t offset, void *buf, uint32_t size, const char *filename){
  gSaveRamWasDeleted = false;
  uint32_t didRead = gCart->readRAM(buf, size, offset);
  if (offset + didRead == gCart->getRAMSize()){
    uint8_t *ptr = (uint8_t *)buf;
    uint32_t remainingSize = size-didRead;
    if (remainingSize > sizeof(gRTCmem)) remainingSize = sizeof(gRTCmem);
    memcpy(&ptr[didRead], gRTCmem, remainingSize);
    didRead += remainingSize;
  } 
  return didRead;
}

int32_t cb_write_ram(uint32_t offset, const void *buf, uint32_t size, const char *filename){
  if (buf == NULL && size == 0){
    /*
      File was deleted
    */
    gSaveRamWasDeleted = true;
    return 0;
  }else{
    gSaveRamWasDeleted = false;
    uint32_t didWrite = gCart->writeRAM(buf, size, offset);
    if (offset + didWrite == gCart->getRAMSize()){
      const uint8_t *ptr = (const uint8_t *)buf;
      uint32_t remainingSize = size-didWrite;
      if (remainingSize > sizeof(gRTCmem)) remainingSize = sizeof(gRTCmem);
      memcpy(gRTCmem, &ptr[didWrite], remainingSize);
      didWrite += remainingSize;
    } 
    return didWrite;
  }
}

int32_t cb_read_cam_image(uint32_t offset, void *buf, uint32_t size, const char *filename){
  int num = 0;
  if (filename[0] == 'I'){
    num = atoi(&filename[sizeof("IMG_")-1]);
  }else{
    num = atoi(&filename[sizeof("DEL_IMG_")-1]);
  }
  size_t imgSize = 0;
  uint8_t *img = (uint8_t*)gbcam_read_image(gCart, num, &imgSize);
  if (offset > imgSize) return 0;
  img += offset;
  imgSize -= offset;
  if (size > imgSize) size = imgSize;
  memcpy(buf, img, size);

  return size;
}

void init_fakefatfs(void){
  gEmuFat.resetFiles();

  {
    size_t infoSize = 0;
    fakefiles_read_infotxt(gCart, &infoSize);
    gEmuFat.addFile("Info","txt", infoSize, cb_read_info);
  }

  CartridgeType cType = gCart->getType();
  if (cType == kCartridgeTypeNone) return;

  char title[0x20] = {};
  bool isColor = false;
  const char *suffix = (cType == kCartridgeTypeGBA) ? "gba" : "gb";
  gCart->readTitle(title, sizeof(title), &isColor);

  {
    char romname[0x40] = {};
    snprintf(romname, sizeof(romname), "%s%s",title, isColor ? " (Color)" : "");
    gEmuFat.addFile(romname,suffix, gCart->getROMSize(), cb_read_rom);
    if (uint32_t ramsize = gCart->getRAMSize()){
      gEmuFat.addFile(romname, "sav", ramsize + sizeof(gRTCmem), cb_read_ram, cb_write_ram);
    }
  }

  if (cType == kCartridgeTypeGB){
    CartridgeGB *gbcart = (CartridgeGB*)gCart;
    if (gbcart->getSubType() == kGBCartridgeTypeCamera){
      char activePhotos[0x1E];
      gbcart->readRAM(activePhotos, sizeof(activePhotos), 0x11B2);

      for (int p = 0; p < sizeof(activePhotos); p++) {
        size_t fileSize = 7286;
        char filename[sizeof("DEL_IMG_") + 0x1E];
        if (activePhotos[p] != 0xFF) {
          // This photo is used
          snprintf(filename, sizeof(filename), "IMG_%02d", p);
        } else {
          // This photo is not used
          snprintf(filename, sizeof(filename), "DEL_IMG_%02d", p);
        }
        gEmuFat.addFile(filename, "bmp", fileSize, cb_read_cam_image);
      }    
    }
  }
  fatIsInited = true;
  gSaveRamWasDeleted = false;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]){
  (void) lun;

  const char vid[] = "Shawazu";
  const char pid[] = "Mass Storage";
  const char rev[] = "1.0";

  memcpy(vendor_id  , vid, strlen(vid));
  memcpy(product_id , pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun){
  (void) lun;

  // RAM disk is ready until ejected
  if (ejected) {

    if (gSaveRamWasDeleted){
      gSaveRamWasDeleted = false;
      gCart->eraseRAM();
      watchdog_reboot(0,0,0);
    }

    // Additional Sense 3A-00 is NOT_FOUND
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return false;
  }

  if (!gIsIOInProgress){
    gIsIOInProgress++;

    if (gCart->getType() == kCartridgeTypeNone){
      fatIsInited = false;

    } else{
      if (!fatIsInited){
        init_fakefatfs();
      }
    }

    gIsIOInProgress--;
  }
  
  return fatIsInited;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
  (void) lun;

  *block_count = gEmuFat.diskBlockNum();
  *block_size  = gEmuFat.diskBlockSize();
}


// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject){
  (void) lun;
  (void) power_condition;

  if ( load_eject )
  {
    if (start)
    {
      // load disk storage
    }else
    {
      // unload disk storage
      ejected = true;
    }
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize){
  (void) lun;

  int32_t didread = 0;
  if (!gIsIOInProgress){
    gIsIOInProgress++;
    didread =  gEmuFat.hostRead(offset + lba * gEmuFat.diskBlockSize(), buffer, bufsize);
    gIsIOInProgress--;
  }

  return didread & ~(gEmuFat.diskBlockSize()-1);
}

bool tud_msc_is_writable_cb (uint8_t lun) {
  (void) lun;
  return true;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize){
  (void) lun;

  int32_t didWrite = 0;
  if (!gIsIOInProgress){
    gIsIOInProgress++;
    didWrite = gEmuFat.hostWrite(offset + lba * gEmuFat.diskBlockSize(), buffer, bufsize);
    gIsIOInProgress--;
  }

  return didWrite & ~(gEmuFat.diskBlockSize()-1);
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
  // read10 & write10 has their own callback and MUST not be handled here

  void const* response = NULL;
  int32_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
    default:
      // Set Sense = Invalid Command Operation
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

      // negative means error -> tinyusb could stall and/or response with failed status
      resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if ( resplen > bufsize ) resplen = bufsize;

  if ( response && (resplen > 0) )
  {
    if(in_xfer)
    {
      memcpy(buffer, response, (size_t) resplen);
    }else
    {
      // SCSI output
    }
  }

  return (int32_t) resplen;
}