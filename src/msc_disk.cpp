#include "bsp/board.h"
#include "tusb.h"
#include "class/msc/msc.h"
#include "pico.h"
#include "fakefiles.hpp"
#include "all.h"
#include "pico/stdlib.h"
#include "Cartridge.hpp"

extern Cartridge *gCart;


// whether host does safe-eject
static bool ejected = false;
static bool fatIsInited = false;

// // Some MCU doesn't have enough 8KB SRAM to store the whole disk
// // We will use Flash as read-only disk with board that has

// #define DATA_Page_Data \
//  "UF2 Bootloader v3.0\n\
// Model: Raspberry Pi RP2\n\
// Board-ID: RPI-RP2\n"

// #define MAX_FILES 0x10

// #define INDEX_INFO_TXT  0
// #define INDEX_CART      1

// #define BLOCK_INFO_TXT 2
// #define BLOCK_CART_ROM 3
// #define BLOCK_CART_RAM 0x103
// #define BLOCK_CART_RAM_END 0x113
// #define BLOCK_DYNAMIC_FILES BLOCK_CART_RAM_END

// enum {
//   DISK_BLOCK_NUM  = 0x3ffff, // 8KB is the smallest size that windows allow to mount
//   DISK_BLOCK_SIZE = 512,
//   DISK_CLUSTER_SIZE = 64,
// };

// //indexs of places
// enum {
//   INDEX_RESERVED = 0,
//   INDEX_FAT_TABLE_1_START = 1, //Fat table size is 0x81
//   INDEX_FAT_TABLE_2_START = 0x82,
//   INDEX_ROOT_DIRECTORY = 0x103,
//   INDEX_DATA_STARTS = 0x123,
//   // INDEX_DATA_END = (INDEX_DATA_STARTS + BLOCK_CART_RAM_END + DISK_CLUSTER_SIZE - 1)
// };

// #define MAX_SECTION_COUNT_FOR_FLASH_SECTION (FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE)
// struct flashingLocation {
//   unsigned int pageCountFlash;
// } flashingLocation = {.pageCountFlash = 0};


// uint8_t DISK_reservedSection[DISK_BLOCK_SIZE] =  {
//     0xEB, 0x3C, 0x90, 
//     'S', 'H', 'A', 'W', 'A', 'Z', 'U', 0x00,  //OEM NAME
//     0x00, (DISK_BLOCK_SIZE>>8),  //sector 512
//     DISK_CLUSTER_SIZE,        //cluster size 8 sectors -unit for file sizes
//     0x01, 0x00,  //BPB_RsvdSecCnt
//     0x02,        //Number of fat tables
//     0x00, 0x02,  //BPB_RootEntCnt  
//     0x00, 0x00,  //16 bit fsat sector count - 0 larger then 0x10000
//     0xF8,        //- non-removable disks a 
//     0x81, 0x00,       //BPB_FATSz16 - Size of fat table
//     0x01, 0x00, //BPB_SecPerTrk 
//     0x01, 0x00, //BPB_NumHeads
//     0x01, 0x00, 0x00, 0x00, //??? BPB_HiddSec 
//     0xFF, 0xFF, 0x03, 0x00, //BPB_TotSec32
//     0x00,  //BS_DrvNum  - probably be 0x80 but is not? 
//     0x00,  //
//     0x29,
//     0x50, 0x04, 0x0B, 0x00, //Volume Serial Number
//     'S' , 'H' , 'A' , 'W' , 'A' , 'Z' , 'U' , ' ' , ' ' , ' ' , ' ' , 
//     'F', 'A', 'T', '1', '6', ' ', ' ', ' ', 

//     0x0e, 0x1f,
//     0xbe, 0x5b, 0x7c, 0xac, 0x22, 0xc0, 0x74, 0x0b, 0x56, 0xb4, 0x0e, 0xbb, 0x07, 0x00, 0xcd, 0x10,
//     0x5e, 0xeb, 0xf0, 0x32, 0xe4, 0xcd, 0x16, 0xcd, 0x19, 0xeb, 0xfe, 0x54, 0x68, 0x69, 0x73, 0x20,
//     0x69, 0x73, 0x20, 0x6e, 0x6f, 0x74, 0x20, 0x61, 0x20, 0x62, 0x6f, 0x6f, 0x74, 0x61, 0x62, 0x6c,
//     0x65, 0x20, 0x64, 0x69, 0x73, 0x6b, 0x2e, 0x20, 0x20, 0x50, 0x6c, 0x65, 0x61, 0x73, 0x65, 0x20,
//     0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x20, 0x61, 0x20, 0x62, 0x6f, 0x6f, 0x74, 0x61, 0x62, 0x6c,
//     0x65, 0x20, 0x66, 0x6c, 0x6f, 0x70, 0x70, 0x79, 0x20, 0x61, 0x6e, 0x64, 0x0d, 0x0a, 0x70, 0x72,
//     0x65, 0x73, 0x73, 0x20, 0x61, 0x6e, 0x79, 0x20, 0x6b, 0x65, 0x79, 0x20, 0x74, 0x6f, 0x20, 0x74,
//     0x72, 0x79, 0x20, 0x61, 0x67, 0x61, 0x69, 0x6e, 0x20, 0x2e, 0x2e, 0x2e, 0x20, 0x0d, 0x0a, 0x00,


//     // Zero up to 2 last bytes of FAT magic code

//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
// };

// uint16_t DISK_fatTable[DISK_BLOCK_SIZE*0x20] = {
//   0xFFF8, //reserved
//   0xFFFF, //reserved

//   0xFFFF, //INFO.TXT
// };


// uint8_t DISK_rootDirectory[DISK_BLOCK_SIZE*0x20] = {
//       // first entry is volume label
//       'S' , 'H' , 'A' , 'W' , 'A' , 'Z' , 'U' , ' ' , ' ' , ' ' , ' ' , 0x28, 0x00, 0x00, 0x00, 0x00,
//       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//       // second entry is html file
//       // 'I' , 'N' , 'F' , 'O' , ' ' , ' ' , ' ' , ' ' , 'T' , 'X' , 'T' , 0x21, 0x00, 0xC6, 0x52, 0x6D,
//       // 0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 
//       // 0x02, 0x00, //cluster location
//       // 0x00, 0x01, 0x00, 0x00, // info.txt files size (4 Bytes)
// };

// struct fileentry{
//   char shortFilename[8];
//   char filenameExt[3];
//   uint8_t fileAttributes;
//   uint8_t  reserved;
//   uint8_t  createTime_ms;
//   uint16_t createTime;
//   uint16_t createDate;
//   uint16_t accessedDate;
//   uint16_t clusterNumber_High;
//   uint16_t modifiedTime;
//   uint16_t modifiedDate;
//   uint16_t clusterLocation;
//   uint32_t fileSize;
// } __attribute__ ((packed));

// enum filenmode {
//   kFileMode_GB,
//   kFileMode_GBC,
//   kFileMode_SAV,
// };

// #define FILEENTRY_ATTR_READONLY     (1 << 0)
// #define FILEENTRY_ATTR_HIDDEN       (1 << 1)
// #define FILEENTRY_ATTR_SYSTEM       (1 << 2) //means: do not move this file physically (during defrag)!
// #define FILEENTRY_ATTR_VOLUME_LABEL (1 << 3)
// #define FILEENTRY_ATTR_SUBDIR       (1 << 4)
// #define FILEENTRY_ATTR_ARCHIVE      (1 << 5)
// #define FILEENTRY_ATTR_LFN_ENTRY    0x0F

// #define LFN_ENTRY_LAST     0x40
// #define LFN_ENTRY_DELETED  0x80
// #define LFN_ENTRY_MAX_NAME_LEN 13

// const uint8_t *file_info_txt_data_ptr = NULL;

// struct fileentry * DISK_rootDirectory_file = ((struct fileentry *)DISK_rootDirectory)+1;

// struct myfiledesc{
//   uint8_t *buf;
//   size_t bufSize;
//   uint16_t block;
//   uint16_t blockCnt;
// };

// static struct myfiledesc gDynamicFiles[0x100] = {};
// static int gNextIndex = 1;

// uint8_t lfn_checksum(const char *filename){
//   uint8_t ret = filename[0];
//   size_t filenamelen = 11;
//   for (int i = 1; i < filenamelen; i++){
//     ret = (ret >> 1) | (ret << 7);
//     ret += filename[i];
//   }
  
//   return ret;
// }

// int add_file_entry(int entryIDX, int clusterLocation, const char *cartname, uint32_t fileSize, const char *fileExt, bool readonly){
//   char romFileName[0x200] = {};
//   size_t cartnameLen = strlen(cartname);
//   size_t fileExtLen = strlen(fileExt);
//   if (fileExtLen > 3) fileExtLen = 3;

//   size_t romFileNameLen = cartnameLen + fileExtLen + 1;
//   int rom_extra_entries = ((romFileNameLen+1)/13)+1;

//   snprintf(romFileName, sizeof(romFileName), "%s.%s",cartname,fileExt);
//   romFileName[romFileNameLen] = '\0';

//   for (int i=0; i<rom_extra_entries; i++){
//     memset(&DISK_rootDirectory_file[entryIDX+i], 0xFF, sizeof(DISK_rootDirectory_file[entryIDX+i]));
//   }

//   DISK_rootDirectory_file[entryIDX].shortFilename[0] = LFN_ENTRY_LAST | rom_extra_entries;

//   for (int z=(rom_extra_entries-1)*LFN_ENTRY_MAX_NAME_LEN; z < romFileNameLen+1; z++){
//     int fnameIdx = 1+2*(z%LFN_ENTRY_MAX_NAME_LEN);
//     if (fnameIdx >= 0x0B) fnameIdx += 3;
//     if (fnameIdx >= 0x1A) fnameIdx += 2;

//     DISK_rootDirectory_file[entryIDX].shortFilename[fnameIdx] = romFileName[z];
//     DISK_rootDirectory_file[entryIDX].shortFilename[fnameIdx+1] = '\0';
//   }

//   for (int npos=0; npos<(rom_extra_entries-1)*LFN_ENTRY_MAX_NAME_LEN; npos++){
//     int fnameIdx = 1+2*(npos%LFN_ENTRY_MAX_NAME_LEN);
//     if (fnameIdx >= 0x0B) fnameIdx += 3;
//     if (fnameIdx >= 0x1A) fnameIdx += 2;

//     DISK_rootDirectory_file[entryIDX+1+(npos/LFN_ENTRY_MAX_NAME_LEN)].shortFilename[fnameIdx] = romFileName[npos];
//     DISK_rootDirectory_file[entryIDX+1+(npos/LFN_ENTRY_MAX_NAME_LEN)].shortFilename[fnameIdx+1] = '\0';
//   }

//   struct fileentry fe ={
//   .shortFilename = {' ' , ' ' , ' ' , ' ', ' ' , ' ' , ' ' , ' '},
//   .fileAttributes = FILEENTRY_ATTR_SYSTEM | (readonly ? FILEENTRY_ATTR_READONLY : 0),
//   .reserved = 0x00,        
//   .clusterLocation = clusterLocation,
//   .fileSize = fileSize,
//   };

//   fe.filenameExt[0] = fileExt[0];
//   fe.filenameExt[1] = fileExt[2];
//   fe.filenameExt[2] = fileExt[2];
//   if (fe.filenameExt[2] == '\0') fe.filenameExt[2] = ' ';

//   {
//     int len = cartnameLen;
//     if (len > sizeof(fe.shortFilename)){
//       len = sizeof(fe.shortFilename);
//       memcpy(fe.shortFilename, cartname, len);
//       fe.shortFilename[6] = 0x7e;
//       fe.shortFilename[7] = '1';
//     }else{
//       memcpy(fe.shortFilename, cartname, len);
//     }
//   }
//   memcpy(&DISK_rootDirectory_file[entryIDX+rom_extra_entries], &fe, sizeof(fe));


//   uint8_t csum = lfn_checksum(fe.shortFilename);
  
//   for (int i = 0; i<rom_extra_entries; i++){
//     if (i) DISK_rootDirectory_file[entryIDX+i].shortFilename[0] = i;
//     DISK_rootDirectory_file[entryIDX+i].fileAttributes = FILEENTRY_ATTR_LFN_ENTRY;
//     DISK_rootDirectory_file[entryIDX+i].reserved = 0;
//     DISK_rootDirectory_file[entryIDX+i].clusterLocation = 0;
//     DISK_rootDirectory_file[entryIDX+i].createTime_ms = csum;
//   }

//   return rom_extra_entries + 1;
// }

// /*
//   filebuf should be a pointer to a malloced buffer. Ownership will be transfered to this function
// */
// int addDynamicFile(const char *filename, void *filebuf, size_t fileSize, const char *fileSuffix, bool readonly){
//   struct myfiledesc *entry = NULL;
//   uint32_t blocknum = BLOCK_DYNAMIC_FILES;
//   for (size_t i = 0; i < sizeof(gDynamicFiles)/sizeof(*gDynamicFiles); i++){
//     if (gDynamicFiles[i].block){
//       blocknum += gDynamicFiles[i].blockCnt;
//     }else{
//       entry = &gDynamicFiles[i];
//       break;
//     }
//   }
//   if (!entry) return -1;

//   entry->block = blocknum;
//   entry->buf = filebuf;
//   entry->bufSize = fileSize;
//   entry->blockCnt = (fileSize/DISK_BLOCK_SIZE);
//   if (fileSize % DISK_BLOCK_SIZE) entry->blockCnt++;

//   if (entry->blockCnt){
//     for (size_t i = entry->block; i < entry->block+entry->blockCnt-1; i++){
//       DISK_fatTable[i] = i+1;
//     }
//     DISK_fatTable[entry->block+entry->blockCnt-1] = 0xFFFF;
//   }

//   int file_entries = add_file_entry(gNextIndex, blocknum, filename, fileSize, fileSuffix, readonly);
//   gNextIndex += file_entries;

//   return 0; 
// }

// void init_fakefatfs(void){
//   memset(&DISK_fatTable[3], 0, sizeof(DISK_fatTable) - (((uint8_t*)&DISK_fatTable[3]) - (uint8_t*)&DISK_fatTable[0]));
//   memset(&DISK_rootDirectory[0x20], 0, sizeof(DISK_rootDirectory) - (((uint8_t*)&DISK_rootDirectory[0x20])- (uint8_t*)&DISK_rootDirectory[0]));

//   for (size_t i = BLOCK_CART_ROM; i < BLOCK_CART_RAM-1; i++){
//     DISK_fatTable[i] = i+1;
//   }
//   DISK_fatTable[BLOCK_CART_RAM-1] = 0xFFFF;

//   for (size_t i = BLOCK_CART_RAM; i < BLOCK_CART_RAM_END-1; i++){
//     DISK_fatTable[i] = i+1;
//   }
//   DISK_fatTable[BLOCK_CART_RAM_END-1] = 0xFFFF;

//   for (size_t i = 0; i < sizeof(gDynamicFiles)/sizeof(*gDynamicFiles); i++){
//     if (gDynamicFiles[i].block){
//       free(gDynamicFiles[i].buf);
//     }
//     memset(&gDynamicFiles[i], 0, sizeof(gDynamicFiles[i]));
//   }
  

//   //init files
//   {
//     //init info.txt
//     uint32_t fsize = 0;
//     file_info_txt_data_ptr = fakefiles_read_infotxt(&fsize);
//     struct fileentry fe ={
//       .shortFilename = {'I' , 'N' , 'F' , 'O' , ' ' , ' ' , ' ' , ' '},
//       .filenameExt = {'T' , 'X' , 'T'},
//       .fileAttributes = FILEENTRY_ATTR_SYSTEM | FILEENTRY_ATTR_READONLY,
//       .reserved = 0x00,
//       .clusterLocation = 2,
//       .fileSize = fsize,
//     };
//     memcpy(&DISK_rootDirectory_file[INDEX_INFO_TXT], &fe, sizeof(fe));

//     if (cart_is_supported()){
//       bool isColor = false;
//       const char *cartname = cart_get_name(true,&isColor);
//       size_t cartnameLen = strlen(cartname);

//       //init game
//       uint32_t rom_size = 0;
//       uint32_t ram_size = 0;

//       //init ROM
//       rom_size = cart_rom_size();
//       ram_size = cart_ram_size();
      
//       int rom_entries = add_file_entry(INDEX_CART, BLOCK_CART_ROM, cartname, rom_size, isColor ? "gbc" : "gb", true);
//       gNextIndex += rom_entries;
//       if (ram_size){
//         int ram_entries = add_file_entry(INDEX_CART+rom_entries, BLOCK_CART_RAM, cartname, ram_size, "sav", false);
//         gNextIndex += ram_entries;
//       }
//     }
//   }

//   //init camera
//   if (cart_get_type() == CART_TYPE_CAMERA) {
//     char activePhotos[0x1E];
//     memset(activePhotos, 0xFF, sizeof(activePhotos));
//     cart_read_ram(activePhotos, sizeof(activePhotos), 0x11B2);

//     for (int p = 0; p < sizeof(activePhotos); p++) {
//       size_t fileSize = 7286;
//       char filename[sizeof("DEL_IMG_") + 0x1E];
//       if (activePhotos[p] != 0xFF) {
//         // This photo is used
//         snprintf(filename, sizeof(filename), "IMG_%02d", p);
//       } else {
//         // This photo is not used
//         snprintf(filename, sizeof(filename), "DEL_IMG_%02d", p);
//       }

//       addDynamicFile(filename, NULL, fileSize, "bmp", true);
//     }
//   }

//   fatIsInited = true;
// }

// // Invoked when received SCSI_CMD_INQUIRY
// // Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
// void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]){
//   (void) lun;

//   const char vid[] = "Shawazu";
//   const char pid[] = "Mass Storage";
//   const char rev[] = "1.0";

//   memcpy(vendor_id  , vid, strlen(vid));
//   memcpy(product_id , pid, strlen(pid));
//   memcpy(product_rev, rev, strlen(rev));
// }

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun){
  (void) lun;


  if (gCart->getType() == kCartridgeTypeNone){
    fatIsInited = false;

  } else{
    if (!fatIsInited){
      init_fakefatfs();
    }
  }

  // RAM disk is ready until ejected
  if (ejected) {
    // Additional Sense 3A-00 is NOT_FOUND
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return false;
  }

  return fatIsInited;
}

// // Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// // Application update block count and block size
// void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
// {
//   (void) lun;

//   *block_count = DISK_BLOCK_NUM;
//   *block_size  = DISK_BLOCK_SIZE;
// }


// // Invoked when received Start Stop Unit command
// // - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// // - Start = 1 : active mode, if load_eject = 1 : load disk storage
// bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject){
//   (void) lun;
//   (void) power_condition;

//   if ( load_eject )
//   {
//     if (start)
//     {
//       // load disk storage
//     }else
//     {
//       // unload disk storage
//       ejected = true;
//     }
//   }

//   return true;
// }

// // Callback invoked when received READ10 command.
// // Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
// int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize){

//   // out of ramdisk
//   if ( lba >= DISK_BLOCK_NUM) return -1;
//   //printf("lba 0x%x, bufsize %d, offset %d\n",lba, bufsize, offset);
//   //uint8_t const* addr = msc_disk[lba] + offset;
//   //memcpy(buffer, addr, bufsize);

//   uint8_t * addr = 0;
//   if(lba == INDEX_RESERVED){
//     addr = DISK_reservedSection;

//   } else if((lba >= INDEX_FAT_TABLE_1_START && lba - INDEX_FAT_TABLE_1_START < sizeof(DISK_fatTable) / DISK_BLOCK_SIZE) && lba < INDEX_FAT_TABLE_2_START){
//     int idx = ((lba-INDEX_FAT_TABLE_1_START)*DISK_BLOCK_SIZE) % sizeof(DISK_fatTable);
//     addr = &((uint8_t*)DISK_fatTable)[idx];

//   } else if((lba >= INDEX_FAT_TABLE_2_START && lba - INDEX_FAT_TABLE_2_START < sizeof(DISK_fatTable) / DISK_BLOCK_SIZE) && lba < INDEX_ROOT_DIRECTORY){
//     int idx = ((lba-INDEX_FAT_TABLE_2_START)*DISK_BLOCK_SIZE) % sizeof(DISK_fatTable);
//     addr = &((uint8_t*)DISK_fatTable)[idx];

//   }
//   else if(lba >= INDEX_ROOT_DIRECTORY && lba - INDEX_ROOT_DIRECTORY < sizeof(DISK_rootDirectory) / DISK_BLOCK_SIZE && lba < INDEX_DATA_STARTS){
//     int idx = ((lba-INDEX_ROOT_DIRECTORY)*DISK_BLOCK_SIZE) % sizeof(DISK_rootDirectory);
//     addr = &((uint8_t*)DISK_rootDirectory)[idx];

//   }
//   else if(lba >= INDEX_DATA_STARTS)
//   {
//     //printf("lba %d, bufsize %d, offset %d\n",lba, bufsize, offset);
//     //DISK_data is only one section large but the cluster sizes are 8.
//     //So if there was a larger file it would be bad.

//     uint32_t clusterNum    = (lba - INDEX_DATA_STARTS) / DISK_CLUSTER_SIZE;
//     uint32_t clusterOffset = (lba - INDEX_DATA_STARTS) % DISK_CLUSTER_SIZE;
    
//     uint32_t dataIdx = 2 + clusterNum;

//     if (dataIdx == BLOCK_INFO_TXT){
//         if (file_info_txt_data_ptr){
//           memcpy(buffer, file_info_txt_data_ptr, bufsize);
//         }else{
//           memset(buffer,0, bufsize);
//         }
//         return (int32_t) bufsize;
//     }else if (dataIdx >= BLOCK_CART_ROM){
//       int32_t didRead = 0;

//       if (dataIdx >= BLOCK_CART_RAM){
//         if (dataIdx < BLOCK_CART_RAM_END){
//           uint32_t realOffset = offset + clusterOffset * DISK_BLOCK_SIZE + (dataIdx-BLOCK_CART_RAM) * DISK_BLOCK_SIZE * DISK_CLUSTER_SIZE;
//           didRead = fakefiles_read_ram(realOffset, buffer, bufsize);
//         }else{
//           for (size_t i = 0; i < sizeof(gDynamicFiles)/sizeof(*gDynamicFiles); i++){
//             if (gDynamicFiles[i].block >= dataIdx && gDynamicFiles[i].blockCnt < dataIdx){
//               uint32_t realOffset = offset + clusterOffset * DISK_BLOCK_SIZE + (dataIdx-gDynamicFiles[i].block) * DISK_BLOCK_SIZE * DISK_CLUSTER_SIZE;
             
//               char *imgbuf = gbcam_get_image(i);
//               if (realOffset < 7268){
//                 int cpySize = 7268 - realOffset;
//                 if (cpySize > bufsize) cpySize = bufsize;
//                 memcpy(buffer, &imgbuf[realOffset], cpySize);

//                 if (cpySize < bufsize){
//                   memset(&((uint8_t*)buffer)[cpySize], 0, bufsize-cpySize);
//                 }
//               }

//               return (int32_t) bufsize;
//             }
//           }
          
//         }        
//       }else{
//         uint32_t realOffset = offset + clusterOffset * DISK_BLOCK_SIZE + (dataIdx-BLOCK_CART_ROM) * DISK_BLOCK_SIZE * DISK_CLUSTER_SIZE;
//         didRead = fakefiles_read_rom(realOffset, buffer, bufsize);
//       }
//       uint8_t *buf = (uint8_t*)buffer;
//       memset(&buf[didRead],0, bufsize-didRead);
//       return (int32_t) bufsize;
//     }
//   }
//   if(addr != 0){
//     memcpy(buffer, addr, bufsize);
//   }else{
//     memset(buffer,0, bufsize);
//   }
//   return (int32_t) bufsize;
// }

// bool tud_msc_is_writable_cb (uint8_t lun) {
//   (void) lun;


//   return true;
// }

// // Callback invoked when received WRITE10 command.
// // Process data in buffer to disk's storage and return number of written bytes
// int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize){
//   (void) lun;
//   printf("write - lba 0x%x, bufsize%d\n", lba,bufsize);

//   uint32_t clusterNum    = (lba - INDEX_DATA_STARTS) / DISK_CLUSTER_SIZE;
//   uint32_t clusterOffset = (lba - INDEX_DATA_STARTS) % DISK_CLUSTER_SIZE;
  
//   uint32_t dataIdx = 2 + clusterNum;

//   if (dataIdx >= BLOCK_CART_RAM){
//     int32_t didWrite = 0;
//     if (dataIdx < BLOCK_CART_RAM_END){
//       uint32_t realOffset = offset + clusterOffset * DISK_BLOCK_SIZE + (dataIdx-BLOCK_CART_RAM) * DISK_BLOCK_SIZE * DISK_CLUSTER_SIZE;
//       didWrite = fakefiles_write_ram(realOffset, buffer, bufsize);
//     }        
//   }

//   return (int32_t) bufsize;
// }

// // Callback invoked when received an SCSI command not in built-in list below
// // - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// // - READ10 and WRITE10 has their own callbacks
// int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
//   // read10 & write10 has their own callback and MUST not be handled here

//   void const* response = NULL;
//   int32_t resplen = 0;

//   // most scsi handled is input
//   bool in_xfer = true;

//   switch (scsi_cmd[0])
//   {
//     default:
//       // Set Sense = Invalid Command Operation
//       tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

//       // negative means error -> tinyusb could stall and/or response with failed status
//       resplen = -1;
//     break;
//   }

//   // return resplen must not larger than bufsize
//   if ( resplen > bufsize ) resplen = bufsize;

//   if ( response && (resplen > 0) )
//   {
//     if(in_xfer)
//     {
//       memcpy(buffer, response, (size_t) resplen);
//     }else
//     {
//       // SCSI output
//     }
//   }

//   return (int32_t) resplen;
// }