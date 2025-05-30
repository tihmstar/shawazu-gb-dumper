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
#include <pico/bootrom.h>

#define RTC_IMMUTABLE_TIMESTAMP 0x7fffffff7fffffff

struct GBRTC{
  uint8_t s;
  uint8_t m;
  uint8_t h;
  uint8_t dl;
  uint8_t dh;
};

struct GBRATC{
  uint8_t y;
  uint8_t mon;
  uint8_t d;
  uint8_t dow;
  uint8_t h;
  uint8_t min;
  uint8_t sec;
  uint8_t status;
};

struct RTCGBASave {
  struct GBRATC rtc;
  uint64_t timestamp;
};

struct RTCGBSave {
  uint32_t real_s;
  uint32_t real_m;
  uint32_t real_h;
  uint32_t real_dl;
  uint32_t real_dh;
  uint32_t lateched_s;
  uint32_t lateched_m;
  uint32_t lateched_h;
  uint32_t lateched_dl;
  uint32_t lateched_dh;
  uint64_t timestamp;
} rtcsave;

extern Cartridge *gCart;

static tihmstar::EmuFATFS<35,0x400> gEmuFat("SHAWAZU");

// whether host does safe-eject
static bool ejected = false;
static bool fatIsInited = false;
static bool gSaveRamWasDeleted = false;
static int gIsIOInProgress = 0;
static CartridgeType gLastCartridgeType = kCartridgeTypeNone;

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

int32_t cb_read_ram(uint32_t offset, void *buf, uint32_t size, const char *filename){
  gSaveRamWasDeleted = false;
  uint32_t didRead = gCart->readRAM(buf, size, offset);
  if (offset + didRead == gCart->getRAMSize()){
    uint32_t rtcSize = 0;
    void *rtcBuf = NULL;
    struct RTCGBSave gbRTCmem = {};
    struct RTCGBASave gbaRTCmem = {};

    if (gCart->getType() == kCartridgeTypeGB){
      //read timestamp from cart
      uint8_t rawTS[0x10] = {};
      int tsDidRead = gCart->readRTC(rawTS,sizeof(rawTS));
      if (tsDidRead == 5){
        //Pokemon Gold GB Timestamp
        gbRTCmem.real_s = gbRTCmem.lateched_s = rawTS[0];
        gbRTCmem.real_m = gbRTCmem.lateched_m = rawTS[1];
        gbRTCmem.real_h = gbRTCmem.lateched_h = rawTS[2];
        gbRTCmem.real_dl = gbRTCmem.lateched_dl = rawTS[3];
        gbRTCmem.real_dh = gbRTCmem.lateched_dh = rawTS[4];        
      }
      gbRTCmem.timestamp = RTC_IMMUTABLE_TIMESTAMP;

      rtcBuf = &gbRTCmem;
      rtcSize = sizeof(gbRTCmem);
    } else if (gCart->getType() == kCartridgeTypeGBA){
      gCart->readRTC(&gbaRTCmem,sizeof(gbaRTCmem));
      gbaRTCmem.timestamp = RTC_IMMUTABLE_TIMESTAMP;
      rtcBuf = &gbaRTCmem;
      rtcSize = sizeof(gbaRTCmem);
    }

    uint8_t *ptr = (uint8_t *)buf;
    uint32_t remainingSize = size-didRead;
    if (remainingSize > rtcSize) remainingSize = rtcSize;
    memcpy(&ptr[didRead], rtcBuf, remainingSize);
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
      uint32_t rtcSize = 0;
      void *rtcBuf = NULL;
      struct RTCGBSave gbRTCmem = {};
      struct RTCGBASave gbaRTCmem = {};
      if (gCart->getType() == kCartridgeTypeGB){
        rtcBuf = &gbRTCmem;
        rtcSize = sizeof(gbRTCmem);
      } else if (gCart->getType() == kCartridgeTypeGBA){
        rtcBuf = &gbaRTCmem;
        rtcSize = sizeof(gbaRTCmem);
      }
      
      const uint8_t *ptr = (const uint8_t *)buf;
      uint32_t remainingSize = size-didWrite;
      if (remainingSize > rtcSize) remainingSize = rtcSize;
      memcpy(rtcBuf, &ptr[didWrite], remainingSize);
      didWrite += remainingSize;  

      if (remainingSize == sizeof(gbRTCmem) && gCart->getType() == kCartridgeTypeGB){
        //Pokemon Gold GB Timestamp
        uint8_t rawTS[0x10] = {};

        rawTS[0] = (uint8_t)gbRTCmem.real_s;
        rawTS[1] = (uint8_t)gbRTCmem.real_m;
        rawTS[2] = (uint8_t)gbRTCmem.real_h;
        rawTS[3] = (uint8_t)gbRTCmem.real_dl;
        rawTS[4] = ((uint8_t)gbRTCmem.real_dh) & 0xC1;
        gCart->writeRTC(rawTS, 5);
      }else if (remainingSize == sizeof(gbaRTCmem) && gCart->getType() == kCartridgeTypeGBA){
        gCart->writeRTC(&gbaRTCmem,sizeof(gbaRTCmem));
      }
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

void cb_newFile(const char *filename, const char filenameSuffix[3], uint32_t fileSize, uint32_t clusterLocation){
  if (strncasecmp(filenameSuffix, "UF2",3) == 0){
    reset_usb_boot(0,0);
  }
}

void init_fakefatfs(void){
  gEmuFat.resetFiles();
  gEmuFat.registerNewfileCallback(cb_newFile);

  {
    size_t infoSize = 0;
    fakefiles_read_infotxt(gCart, &infoSize);
    gEmuFat.addFile("Info","txt", infoSize, cb_read_info);
  }

  gEmuFat.addFile(".metadata_never_index","", 0, cb_read_info);

  CartridgeType cType = gCart->getType();
  if (cType != kCartridgeTypeNone){
    char title[0x20] = {};
    bool isColor = false;
    const char *suffix = (cType == kCartridgeTypeGBA) ? "gba" : "gb";
    gCart->readTitle(title, sizeof(title), &isColor);

    {
      char romname[0x40] = {};
      snprintf(romname, sizeof(romname), "%s%s",title, isColor ? " (Color)" : "");
      gEmuFat.addFileDynamic(romname,suffix, gCart->getROMSize(), 0, cb_read_rom);
      if (uint32_t ramsize = gCart->getRAMSize()){
        uint32_t savsize = ramsize;
        if (gCart->hasRTC()){
          if (gCart->getType() == kCartridgeTypeGB){
            savsize += sizeof(struct RTCGBSave);
          } else if (gCart->getType() == kCartridgeTypeGBA) {
            savsize += sizeof(struct RTCGBASave);
          }
        }
        gEmuFat.addFileDynamic(romname, "sav", savsize, 0, cb_read_ram, cb_write_ram);
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
          gEmuFat.addFileDynamic(filename, "bmp", fileSize, 0, cb_read_cam_image);
        }    
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

    CartridgeType curType = gCart->getType();
    if (curType != gLastCartridgeType){
      fatIsInited = false;
      tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);

    } else{
      if (!fatIsInited){
        init_fakefatfs();
      }
    }
    gLastCartridgeType = curType;

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
#ifdef READONLY_MSC
  return false;
#endif
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