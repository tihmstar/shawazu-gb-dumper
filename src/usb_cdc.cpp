
#include "Cartridge.hpp"
#include "all.h"

#include <tusb.h>
#include <stdlib.h>
#include <stdio.h>

extern Cartridge *gCart;

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void) {
  static char buf[0x400] = {};
  static int bufptr = 0;
  if (tud_cdc_available()) {
    // read data
    char c = tud_cdc_read_char();
    if (c == '\r'){
      tud_cdc_write_char(c);
      c = '\n';
    }
    tud_cdc_write_char(c);
    if (c != '\n'){
      if (c == 0x7f){
        if (bufptr) {
          buf[--bufptr] = '\0';
        }
      }else{
        buf[bufptr++] = c;
      }
    }else{
      buf[bufptr] = '\0';
      bufptr = 0;

      CartridgeType cType = gCart->getType();

      if (strcmp(buf, "name") == 0){
        if (cType!= kCartridgeTypeNone){
          char cartname[0x20] = {};
          gCart->readTitle(cartname, sizeof(cartname), NULL);
          tud_cdc_write_str("Cart: ");
          tud_cdc_write_str(cartname);
          tud_cdc_write_str("\r\n");
        }else{
          tud_cdc_write_str("NO CART CONNECTED!\r\n");
        }
      }else if (strncmp(buf, "r ", 2) == 0){
        const char *s_addr = NULL;
        const char *s_len = NULL;

        uint16_t addr = 0;
        uint16_t len = 0;

        if (!(s_addr = strtok(buf+2, " "))) return;
        if (!(s_len = strtok(NULL, " "))) return;

        if (strncmp(s_addr, "0x", 2) == 0) {
            addr = (uint16_t)strtol(s_addr, NULL, 16);
        }else{
            addr = (uint16_t)strtol(s_addr, NULL, 10);
        }
        
        if (strncmp(s_len, "0x", 2) == 0) {
            len = (uint16_t)strtol(s_len, NULL, 16);
        }else{
            len = (uint16_t)strtol(s_len, NULL, 10);
        }

        while (len > 0){
          uint16_t curlen = len;
          if (curlen > sizeof(buf)) curlen = sizeof(buf);
          uint16_t didread = gCart->read(buf, curlen, addr);
          if (didread < curlen){
            memset(&buf[didread], 0xFF, curlen - didread);
          }
          addr += didread;
          len -= didread;
          tud_cdc_write(buf,didread);
          tud_cdc_write_flush();
          tud_task();
        }
      }else if (strncmp(buf, "w ", 2) == 0){
        const char *s_addr = NULL;
        const char *s_len = NULL;

        uint16_t addr = 0;
        uint16_t len = 0;

        if (!(s_addr = strtok(buf+2, " "))) return;
        if (!(s_len = strtok(NULL, " "))) return;

        if (strncmp(s_addr, "0x", 2) == 0) {
            addr = (uint16_t)strtol(s_addr, NULL, 16);
        }else{
            addr = (uint16_t)strtol(s_addr, NULL, 10);
        }
        
        if (strncmp(s_len, "0x", 2) == 0) {
            len = (uint16_t)strtol(s_len, NULL, 16);
        }else{
            len = (uint16_t)strtol(s_len, NULL, 10);
        }

        uint64_t lastRead = 0;
        int didwrite = 0;
        while (didwrite < len){
          uint16_t curWriteLen = len-didwrite;
          if (curWriteLen > sizeof(buf)) curWriteLen = sizeof(buf);

          lastRead = time_us_64();
          while (true){
            tud_task();
            if (tud_cdc_available()) break;
            if (time_us_64() - lastRead > USEC_PER_SEC*2) goto endWrite;
          }

          curWriteLen = tud_cdc_read(buf, curWriteLen);
          int wres = gCart->writeRaw(buf, curWriteLen,addr+didwrite);
          if (wres > 0){
            didwrite += wres;
          }
          if (wres != curWriteLen) break;
        }
      endWrite:
        snprintf(buf, sizeof(buf), "WROTE: ADDR 0x%04x LEN 0x%x\r\n",addr,didwrite);
        tud_cdc_write_str(buf);
      }
    }
    tud_cdc_write_flush();
  }
}
