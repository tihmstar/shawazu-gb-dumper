#include "../include/gbrw.h"
#include <hardware/timer.h>
#include <hardware/clocks.h>

#include "all.h"
#include "gb_read.pio.h"
#include "gb_write.pio.h"


#define USEC_PER_SEC 1000000

#define GBRW_PIO       pio0
#define GBRW_SM        0

#ifdef REV2_LAYOUT
#define PIN_CS         25
#else
#define PIN_CS         28
#endif

#define PIN_RST        24


#define MODE_READ 1
#define MODE_WRITE 2

static bool gIsInited = false;
static bool gModeIsRead = false;
static int gRead_pio_pc = 0;
static int gWrite_pio_pc = 0;


#pragma mark private
static int internal_init(bool isRead){
  if (!gIsInited) return -1;
  if (gModeIsRead != isRead){
      gModeIsRead = isRead;
      pio_sm_set_enabled(GBRW_PIO, GBRW_SM, false);
      pio_sm_clear_fifos(GBRW_PIO, GBRW_SM);
      pio_sm_restart(GBRW_PIO, GBRW_SM);
      pio_sm_clkdiv_restart(GBRW_PIO, GBRW_SM);
      if (isRead){
          gb_read_program_init(GBRW_PIO, GBRW_SM, gRead_pio_pc);
      }else{
          gb_write_program_init(GBRW_PIO, GBRW_SM, gWrite_pio_pc);        
      }
      pio_sm_set_enabled(GBRW_PIO, GBRW_SM, true);   
  }    
  return 0;
}

#pragma mark public
void gb_rw_init (void){
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_put(PIN_CS, 1);

  gpio_init(PIN_RST);
  gpio_set_dir(PIN_RST, GPIO_OUT);
  gpio_put(PIN_RST, 1);

  for (int i=0; i<29; i++){
    if (i == PIN_CS) continue;
    if (i == PIN_RST) continue;
    gpio_set_function(i, GPIO_FUNC_PIO0);
  }

  gRead_pio_pc = pio_add_program(GBRW_PIO, &gb_read_program);
  gWrite_pio_pc = pio_add_program(GBRW_PIO, &gb_write_program);
  gIsInited = true;
  gModeIsRead = false;
  internal_init(true);
}

void gb_rw_cleanup(void){
  if (!gIsInited) return;
  pio_sm_set_enabled(GBRW_PIO, GBRW_SM, false);
  pio_sm_clear_fifos(GBRW_PIO, GBRW_SM);
  pio_sm_restart(GBRW_PIO, GBRW_SM);
  pio_sm_clkdiv_restart(GBRW_PIO, GBRW_SM);
  pio_remove_program(GBRW_PIO, &gb_read_program, gRead_pio_pc);
  pio_remove_program(GBRW_PIO, &gb_write_program, gWrite_pio_pc);
  gIsInited = false;
}

int gb_read_byte (uint16_t addr){
  int err = 0;
  err = internal_init(true);
  if (err) return err;

  if (addr >= 0xA000) gpio_put(PIN_CS, 0);
  else gpio_put(PIN_CS, 1);

  pio_sm_put(GBRW_PIO, GBRW_SM, addr);
  uint64_t time = time_us_64();
  while (pio_sm_is_rx_fifo_empty(GBRW_PIO, GBRW_SM)){
     if (time_us_64() - time > USEC_PER_SEC*2) return -2;
  }
  uint8_t val = pio_sm_get(GBRW_PIO, GBRW_SM);

  return val;
}

int gb_write_byte(uint16_t addr, uint8_t data){
  int err = 0;
  err = internal_init(false);
  if (err) return err;

  if (addr >= 0xA000) gpio_put(PIN_CS, 0);
  else gpio_put(PIN_CS, 1);

  uint64_t time = time_us_64();
  while (pio_sm_is_tx_fifo_full(GBRW_PIO, GBRW_SM)){
     if (time_us_64() - time > USEC_PER_SEC*2) return -2;
  }
  uint32_t val = (((uint32_t)data) << 16) | addr;
  pio_sm_put(GBRW_PIO, GBRW_SM, val);
  time = time_us_64();
  while (pio_sm_is_rx_fifo_empty(GBRW_PIO, GBRW_SM)){
    if (time_us_64() - time > USEC_PER_SEC*2) return -3;
  }
  uint8_t rval = pio_sm_get(GBRW_PIO, GBRW_SM);
  return rval == 8 ? 0 : -4;
}

int gb_read(uint16_t addr, void *buf, int size){
  uint8_t *dst = (uint8_t*)buf;
  int didread = 0;
  for (; didread < size; didread++){
    int data = gb_read_byte(addr + didread);
    if (data < 0) break;
    dst[didread] = (uint8_t)data;
  }
  return didread;
}

int gb_write(uint16_t addr, const void *buf, int size){
  const uint8_t *src = (uint8_t*)buf;
  int didwrite = 0;

  for (; didwrite < size; didwrite++){
    int err = gb_write_byte(addr + didwrite, src[didwrite]);
    if (err < 0) break;
  }
  return didwrite;
}