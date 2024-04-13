
#include "fakefiles.hpp"

#include "Cartridge.hpp"
#include "CartridgeGB.hpp"
#include "CartridgeGBA.hpp"
#include "CartridgeGBASaveFlash.hpp"
#include <stdio.h>
#include <string.h>

uint8_t gbcam_bmpStart[0x76] = {0x42, 0x4D, 0x76, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00,
    0x28, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00,
    0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF,
    0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00};

const char *fakefiles_read_infotxt(Cartridge *cart, size_t *outSize){
  static char buf[0x100] = {};
  int bufcontent = 0;

  CartridgeType cType = cart->getType();

  if (cType == kCartridgeTypeNone){
    bufcontent = snprintf(buf, sizeof(buf)-1, "No cartridge connected!\n");
  }else{
    cart->readTitle(&buf[0x80], 0x80, NULL);
    bufcontent = snprintf(buf, sizeof(buf)-1, "Title: %s\n", &buf[0x80]);

    int romSize = cart->getROMSize() / 1024;
    if (romSize >= 1024) {
      bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "ROM Size: %d MiB\n", romSize / 1024);
    } else {
      bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "ROM Size: %d KiB\n", romSize);
    }
    bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "RAM Size: %d KiB\n", cart->getRAMSize() / 1024);

    const char *cartType = "Unknown";
    uint8_t cartTypeVal = 0x00;

    switch (cType){
      case kCartridgeTypeGB:
      {
        CartridgeGB *gbcart = (CartridgeGB*)cart;
        bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "ROM Revision: %d\n", gbcart->getROMRevision());
        // Convert cart type to appropriate string
        cartType = "Unknown";
        cartTypeVal = gbcart->getGBTypeRaw();
        switch (cartTypeVal){
          case 0x00: cartType = "ROM ONLY"; break;
          case 0x01: cartType = "MBC1"; break;
          case 0x02: cartType = "MBC1+RAM"; break;
          case 0x03: cartType = "MBC1+RAM+BATTERY"; break;
          case 0x05: cartType = "MBC2"; break;
          case 0x06: cartType = "MBC2+BATTERY"; break;
          case 0x08: cartType = "ROM+RAM"; break;
          case 0x09: cartType = "ROM+RAM+BATTERY"; break;
          case 0x0B: cartType = "MMM01"; break;
          case 0x0C: cartType = "MMM01+RAM"; break;
          case 0x0D: cartType = "MMM01+RAM+BATTERY"; break;
          case 0x0F: cartType = "MBC3+TIMER+BATTERY"; break;
          case 0x10: cartType = "MBC3+TIMER+RAM+BATTERY"; break;
          case 0x11: cartType = "MBC3"; break;
          case 0x12: cartType = "MBC3+RAM"; break;
          case 0x13: cartType = "MBC3+RAM+BATTERY"; break;
          case 0x19: cartType = "MBC5"; break;
          case 0x1A: cartType = "MBC5+RAM"; break;
          case 0x1B: cartType = "MBC5+RAM+BATTERY"; break;
          case 0x1C: cartType = "MBC5+RUMBLE"; break;
          case 0x1D: cartType = "MBC5+RUMBLE+RAM"; break;
          case 0x1E: cartType = "MBC5+RUMBLE+RAM+BATTERY"; break;
          case 0xFC: cartType = "POCKET CAMERA"; break;
          default: break;
        }
        bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "Cartridge Type: %s(%d)\n", cartType, cartTypeVal);
      }
      break;

      case kCartridgeTypeGBA:
      {
        CartridgeGBA *gbacart = (CartridgeGBA*)cart;
        cartType = gbacart->getStorageType();
        bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "Cartridge Type: GBA (%s)\n", cartType);
        switch (gbacart->getSubType()){
        case kGBACartridgeTypeSaveFlash:
        case kGBACartridgeTypeSaveFlashExtended:
          {
            CartridgeGBASaveFlash *flashgbacart = (CartridgeGBASaveFlash*)gbacart;
            uint16_t vidpid = flashgbacart->flashReadVIDandPID();
            const char *vidstr = "Unknown";
            switch (vidpid & 0xff){
              case 0x1F: vidstr = "Atmel"; break;
              case 0x32: vidstr = "Panasonic"; break;
              case 0x62: vidstr = "Sanyo"; break;
              case 0xC2: vidstr = "Macronix"; break;
              case 0xBF: vidstr = "Sanyo or SST"; break;
              default: break;
            }
            bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "Flash VID: 0x%02x (%s)\n", (vidpid & 0xff),vidstr);
            bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "Flash PID: 0x%02x\n", (vidpid >> 8));
          }
          break;
        default:
          break;
        }
      }
      break;

      default:
      {
        bufcontent += snprintf(buf+bufcontent, sizeof(buf)-bufcontent-1, "Cartridge Type: ERROR\n");
      }
      break;
    }
  }

  if (outSize) *outSize = bufcontent;
  return buf;
}

int32_t fakefiles_read_rom(Cartridge *cart, uint32_t offset, void *buf, uint32_t bufSize){
  return cart->readROM(buf, bufSize, offset);
}

int32_t fakefiles_read_ram(Cartridge *cart, uint32_t offset, void *buf, uint32_t bufSize){
  return cart->readRAM(buf, bufSize, offset);
}

int32_t fakefiles_write_ram(Cartridge *cart, uint32_t offset, const void *buf, uint32_t bufSize){
  return cart->writeRAM(buf, bufSize, offset);
}

const void *gbcam_read_image(Cartridge *cart, int which, size_t *bufSize){
  static char start[7286];
  static int last = -1;
  if (which == last) {
    if (bufSize) *bufSize = sizeof(start);
    return start;
  }

  last = which;

  char savBuffer[3584];
  cart->readRAM(savBuffer, sizeof(savBuffer), (which * 0x1000) + 0x2000);
  
  char *buf = start;
  memcpy(buf, gbcam_bmpStart, 0x76); // Write start of BMP file
  buf += 0x76;
  
  int currentByte = 0xD0E;
  
  // 13 vertical blocks of 8 x 8 pixels
  for (uint8_t v = 0; v < 14; v++) {
      
      // 8 Lines
      for (uint8_t l = 0; l < 8; l++) {
          
          // One line
          for (uint8_t x = 0; x < 16; x++) {
              
              // 1st byte stores whether the pixel is white (0) or silver (1)
              // 2nd byte stores whether the pixel is white (0), grey (1, if the bit in the first byte is 0) and black (1, if the bit in the first byte is 1).
              uint8_t pixelsWhiteSilver = savBuffer[currentByte];
              uint8_t pixelsGreyBlack = savBuffer[currentByte+1];
              
              uint8_t eightPixels[4];
              uint8_t epCounter = 0;
              uint8_t tempByte = 0;
              
              for (int8_t p = 7; p >= 0; p--) {
                  // 8 bit BMP colour depth, each nibble is 1 pixel
                  if ((pixelsWhiteSilver & 1<<p) && (pixelsGreyBlack & 1<<p)) {
                      tempByte |= 0x00; // Black
                  }
                  else if (pixelsWhiteSilver & 1<<p) {
                      tempByte |= 0x08; // Silver
                  }
                  else if (pixelsGreyBlack & 1<<p) {
                      tempByte |= 0x07; // Grey
                  }
                  else {
                      tempByte |= 0x0F; // White
                  }
                  
                  // For odd bits, shift the result left by 4 and save the result to our buffer on even bits
                  if (p % 2 == 0) {
                      eightPixels[epCounter] = tempByte;
                      epCounter++;
                      tempByte = 0; // Reset byte
                  }
                  else {
                      tempByte <<= 4;
                  }
              }
              
              memcpy(buf, eightPixels, 4);
              buf += 4;
              currentByte += 0x10;
          }
          
          currentByte -= 0x102;
      }
      
      currentByte -= 0xF0;
  }

  if (bufSize) *bufSize = sizeof(start);
  return start;
}
