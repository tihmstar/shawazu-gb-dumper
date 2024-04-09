#include <stdio.h>
#include <stdlib.h>
#include <tusb.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "gbrw.h"
#include "gbarw.h"
#include "all.h"

#include "Cartridge.hpp"

Cartridge *gCart = NULL;

void cdc_task(void);


int main() {
  {
    static Cartridge cartHolder;
    gCart = &cartHolder;
  }
  stdio_init_all();

  tusb_init();

  while (1){
    tud_task(); // tinyusb device task
    cdc_task();
  }

  return 0;
}