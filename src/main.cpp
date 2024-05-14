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
  /*
    First thing, init mini voltage to 3v3
  */
  gpio_init(MINI_VOLTAGE_CTRL_PIN);
  gpio_set_dir(MINI_VOLTAGE_CTRL_PIN, GPIO_OUT);
  gpio_put(MINI_VOLTAGE_CTRL_PIN, 0);

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