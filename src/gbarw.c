#include "../include/gbarw.h"
#include <hardware/timer.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <stdio.h>

#include "gba_read.pio.h"
#include "gba_sram_read.pio.h"
#include "gba_sram_write.pio.h"
#include "gba_eeprom_write.pio.h"


#define USEC_PER_SEC 1000000

#define GBARW_PIO      pio0
#define GBARW_SM        0

#define DMA_CHANNEL_READ         (0) /* bit plane content DMA channel */
#define DMA_CHANNEL_READ_MASK    (1u<<DMA_CHANNEL_READ)
#define DMA_CHANNEL_WRITE        (1) /* bit plane content DMA channel */
#define DMA_CHANNEL_WRITE_MASK   (1u<<DMA_CHANNEL_WRITE)

#define MODE_WRITE        1
#define MODE_READ         2
#define MODE_READ_SRAM    3
#define MODE_WRITE_EEPROM 4
#define MODE_READ_EEPROM  5

static bool gIsInited = false;
static int gActiveMode = 0;
static int gRead_pio_pc = 0;
static int gRead_sram_pio_pc = 0;
static int gWrite_pio_pc = 0;
static int gWrite_eeprom_pio_pc = 0;


#pragma mark private
static int internal_deinit(){
  if (gWrite_pio_pc){
    pio_remove_program(GBARW_PIO, &gba_sram_write_program, gWrite_pio_pc);
    gWrite_pio_pc = 0;
  }
  if (gRead_sram_pio_pc){
    pio_remove_program(GBARW_PIO, &gba_sram_read_program, gRead_sram_pio_pc);
    gRead_sram_pio_pc = 0;
  }
  if (gRead_pio_pc){
    pio_remove_program(GBARW_PIO, &gba_read_program, gRead_pio_pc);
    gRead_pio_pc = 0;
  }
  if (gWrite_eeprom_pio_pc){
    pio_remove_program(GBARW_PIO, &gba_eeprom_write_program, gWrite_eeprom_pio_pc);
    gWrite_eeprom_pio_pc = 0;
  }
}
static int internal_init(int mode){
  if (!gIsInited) return -1;
  if (gActiveMode != mode){
      gActiveMode = mode;
      pio_sm_set_enabled(GBARW_PIO, GBARW_SM, false);
      pio_sm_clear_fifos(GBARW_PIO, GBARW_SM);
      pio_sm_restart(GBARW_PIO, GBARW_SM);
      pio_sm_clkdiv_restart(GBARW_PIO, GBARW_SM);
      if (mode == MODE_READ || mode == MODE_READ_EEPROM){
          if (!gRead_pio_pc){
            internal_deinit();
            gRead_pio_pc = pio_add_program(GBARW_PIO, &gba_read_program);
          }

          gba_read_program_init(GBARW_PIO, GBARW_SM, gRead_pio_pc, mode == MODE_READ_EEPROM);
          gpio_init(PIN_NWR);
          gpio_set_dir(PIN_NWR, GPIO_OUT);
          gpio_put(PIN_NWR, 1);

          gpio_init(PIN_CS2);
          gpio_set_dir(PIN_CS2, GPIO_OUT);
          gpio_put(PIN_CS2, 1);

          if (mode == MODE_READ_EEPROM){
            gpio_init(PIN_A23);
            gpio_set_dir(PIN_A23, GPIO_OUT);
            gpio_put(PIN_A23, 1);
          }

      }else if (mode == MODE_READ_SRAM){
          if (!gRead_sram_pio_pc){
            internal_deinit();
            gRead_sram_pio_pc = pio_add_program(GBARW_PIO, &gba_sram_read_program);
          }

          gba_sram_read_program_init(GBARW_PIO, GBARW_SM, gRead_sram_pio_pc);

          gpio_init(PIN_NWR);
          gpio_set_dir(PIN_NWR, GPIO_OUT);
          gpio_put(PIN_NWR, 1);
      }else if (mode == MODE_WRITE_EEPROM){
          if (!gWrite_eeprom_pio_pc){
            internal_deinit();
            gWrite_eeprom_pio_pc = pio_add_program(GBARW_PIO, &gba_eeprom_write_program);
          }

          gba_eeprom_write_program_init(GBARW_PIO, GBARW_SM, gWrite_eeprom_pio_pc);
          gpio_init(PIN_CS2);
          gpio_set_dir(PIN_CS2, GPIO_OUT);
          gpio_put(PIN_CS2, 1);

          gpio_init(PIN_A23);
          gpio_set_dir(PIN_A23, GPIO_OUT);
          gpio_put(PIN_A23, 0);

      }else if (mode == MODE_WRITE){
        if (!gWrite_pio_pc){
          internal_deinit();
          gWrite_pio_pc = pio_add_program(GBARW_PIO, &gba_sram_write_program);
        }

        gba_sram_write_program_init(GBARW_PIO, GBARW_SM, gWrite_pio_pc);        
      }
      pio_sm_set_enabled(GBARW_PIO, GBARW_SM, true);   
  }    
  return 0;
}

#pragma mark public
void gba_rw_init (void){
  dma_claim_mask(DMA_CHANNEL_READ_MASK | DMA_CHANNEL_WRITE_MASK);
  for (int i=0; i<29; i++){
    gpio_set_function(i, GPIO_FUNC_PIO0);
  }

  gRead_pio_pc = pio_add_program(GBARW_PIO, &gba_read_program);
  gIsInited = true;
  gActiveMode = 0;
  internal_init(MODE_READ);
}

void gba_rw_cleanup(void){
  if (!gIsInited) return;
  pio_sm_set_enabled(GBARW_PIO, GBARW_SM, false);
  pio_sm_clear_fifos(GBARW_PIO, GBARW_SM);
  pio_sm_restart(GBARW_PIO, GBARW_SM);
  pio_sm_clkdiv_restart(GBARW_PIO, GBARW_SM);
  dma_unclaim_mask(DMA_CHANNEL_READ_MASK | DMA_CHANNEL_WRITE_MASK);
  internal_deinit();
  gIsInited = false;
}

static void internal_pio_read_dma(void *buf, size_t bufSize) {
  dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL_READ); /* get default configuration */
  channel_config_set_dreq(&channel_config, pio_get_dreq(GBARW_PIO, GBARW_SM, false)); /* configure data request. true: sending data to the PIO state machine */
  bufSize >>= 1;
  channel_config_set_transfer_data_size(&channel_config, DMA_SIZE_16);
  channel_config_set_read_increment(&channel_config, false);
  channel_config_set_write_increment(&channel_config, true);
  dma_channel_configure(DMA_CHANNEL_READ,
                        &channel_config,
                        buf, //write address
                        &GBARW_PIO->rxf[GBARW_SM], /* read address: read from PIO FIFO */
                        bufSize,
                        true); /* start */
}

static void internal_pio_write_dma(const void *buf, size_t bufSize) {
  dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL_WRITE); /* get default configuration */
  channel_config_set_dreq(&channel_config, pio_get_dreq(GBARW_PIO, GBARW_SM, true)); /* configure data request. true: sending data to the PIO state machine */
  bufSize >>= 1;
  channel_config_set_transfer_data_size(&channel_config, DMA_SIZE_16);
  channel_config_set_read_increment(&channel_config, true);
  channel_config_set_write_increment(&channel_config, false);
  dma_channel_configure(DMA_CHANNEL_WRITE,
                        &channel_config,
                        &GBARW_PIO->txf[GBARW_SM], /* write address: write to PIO FIFO */
                        buf, //read address
                        bufSize,
                        true); /* start */
}


static int popPicoSM(){
  uint64_t time = time_us_64();
  while (pio_sm_is_rx_fifo_empty(GBARW_PIO, GBARW_SM)){
     if (time_us_64() - time > USEC_PER_SEC*2) return -2;
  }
  uint32_t val = pio_sm_get(GBARW_PIO, GBARW_SM);
  return val;
}

static int gba_read_rom(uint32_t addr, void *buf, uint32_t size){
  int err = 0;
  err = internal_init(addr >= GBA_EEPROM_READ_ADDRESS ? MODE_READ_EEPROM : MODE_READ);
  if (err) return err;
  if (!size) return 0;

  uint8_t *ptr = (uint8_t*)buf;
  int didRead = 0;
  int needsRawReads = 0;

  while (!pio_sm_is_rx_fifo_empty(GBARW_PIO, GBARW_SM)) 
    pio_sm_get(GBARW_PIO, GBARW_SM);

  needsRawReads = size / 2;
  if (size & 1) needsRawReads++;

  if (addr & 1){
    if ((size & 1) == 0) needsRawReads++;

    pio_sm_put(GBARW_PIO, GBARW_SM, addr>>1);
    pio_sm_put(GBARW_PIO, GBARW_SM, needsRawReads);

    int v = popPicoSM();
    if (v < 0) return v;
    uint8_t *pv = (uint8_t*)&v;
    *ptr++ = pv[1]; didRead++;

  }else{
    uint32_t needsRead = (size & ~1);

    if (needsRead > 3){
      internal_pio_read_dma(ptr, needsRead);
      ptr += needsRead;
      didRead += needsRead;
    }

    pio_sm_put(GBARW_PIO, GBARW_SM, addr>>1);
    pio_sm_put(GBARW_PIO, GBARW_SM, needsRawReads);
  }
  
  if (didRead > 3){
    dma_channel_wait_for_finish_blocking(DMA_CHANNEL_READ);
  }

  while (didRead < size){
    int v = popPicoSM();
    if (v < 0) return v;
    uint8_t *pv = (uint8_t*)&v;
    *ptr++ = *pv++; didRead++;
    if (didRead < size){
      *ptr++ = *pv++; didRead++;
    }
  }

  return didRead;
}

static int gba_sram_read_byte_internal(uint16_t addr){
  pio_sm_put(GBARW_PIO, GBARW_SM, addr);
  uint64_t time = time_us_64();
  while (pio_sm_is_rx_fifo_empty(GBARW_PIO, GBARW_SM)){
     if (time_us_64() - time > USEC_PER_SEC*2) return -2;
  }
  uint8_t val = pio_sm_get(GBARW_PIO, GBARW_SM);
  return val;
}

static int gba_read_sram(uint16_t addr, void *buf, int size){
  int err = 0;
  err = internal_init(MODE_READ_SRAM);
  if (err) return err;

  uint8_t *dst = (uint8_t*)buf;
  int didread = 0;
  for (; didread < size; didread++){
    int data = gba_sram_read_byte_internal(addr + didread);
    if (data < 0) break;
    dst[didread] = (uint8_t)data;
  }
  return didread;
}


static int gba_write_sram_byte_internal(uint16_t addr, uint8_t data){
  uint64_t time = time_us_64();
  while (pio_sm_is_tx_fifo_full(GBARW_PIO, GBARW_SM)){
     if (time_us_64() - time > USEC_PER_SEC*2) return -2;
  }
  uint32_t val = (((uint32_t)data) << 16) | addr;
  pio_sm_put(GBARW_PIO, GBARW_SM, val);
  time = time_us_64();
  while (pio_sm_is_rx_fifo_empty(GBARW_PIO, GBARW_SM)){
    if (time_us_64() - time > USEC_PER_SEC*2) return -3;
  }
  uint8_t rval = pio_sm_get(GBARW_PIO, GBARW_SM);
  return rval == 8 ? 0 : -4;
}

static int gba_write_sram(uint16_t addr, const void *buf, int size){
  int err = 0;
  err = internal_init(MODE_WRITE);
  if (err) return err;
  const uint8_t *src = (uint8_t*)buf;
  int didwrite = 0;

  for (; didwrite < size; didwrite++){
    int err = gba_write_sram_byte_internal(addr + didwrite, src[didwrite]);
    if (err < 0) break;
  }
  return didwrite;
}

static int gba_write_rom(uint32_t addr, const void *buf, int size){
  int err = 0;
  err = internal_init(MODE_WRITE_EEPROM);
  if (err) return err;

  const uint8_t *ptr = (uint8_t*)buf;
  int didWrite = 0;
  int needsWriteRaw = 0;

  while (!pio_sm_is_rx_fifo_empty(GBARW_PIO, GBARW_SM)) 
    pio_sm_get(GBARW_PIO, GBARW_SM);

  gpio_put(PIN_A23, 1);

  needsWriteRaw = size / 2;
  if (needsWriteRaw){
    pio_sm_put(GBARW_PIO, GBARW_SM, addr);
    pio_sm_put(GBARW_PIO, GBARW_SM, needsWriteRaw-1);

    didWrite = needsWriteRaw<<1;
    internal_pio_write_dma(ptr, didWrite);

    dma_channel_wait_for_finish_blocking(DMA_CHANNEL_WRITE);

    int v = popPicoSM();

    if (v != 16){
      return -16;
    }
  }
  
  gpio_put(PIN_A23, 0);

  return didWrite;
}

int gba_read(uint32_t addr, void *buf, uint32_t size){
  if (addr >= GBA_SAVEGAME_MAP_ADDRESS){
    return gba_read_sram((uint16_t)addr, buf, size);
  }else{
    return gba_read_rom(addr & 0x3FFFFFF, buf, size);
  }

}

int gba_write(uint32_t addr, const void *buf, uint32_t size){
  if (addr >= GBA_SAVEGAME_MAP_ADDRESS){
    return gba_write_sram((uint16_t)addr, buf, size);
  }else{
    return gba_write_rom(addr & 0x3FFFFFF, buf, size);
  }
}

int gba_read_byte(uint32_t addr){
  if (addr >= GBA_SAVEGAME_MAP_ADDRESS){
    int err = 0;
    err = internal_init(MODE_READ_SRAM);
    if (err) return err;
    return gba_sram_read_byte_internal(addr);
  }else{
    uint8_t byte = 0xFF;
    gba_read_rom(addr & 0x3FFFFFF, &byte, 1);
    return byte;
  }
}

int gba_write_byte(uint32_t addr, uint8_t data){
  if (addr >= GBA_SAVEGAME_MAP_ADDRESS){
    int err = 0;
    err = internal_init(MODE_WRITE);
    if (err) return err;
    return gba_write_sram_byte_internal((uint16_t)addr, data);
  }else{
    return -1;
  }
}