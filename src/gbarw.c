#include "../include/gbrw.h"
#include <hardware/timer.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <stdio.h>

#include "gba_read.pio.h"
// #include "gba_write.pio.h"


#define USEC_PER_SEC 1000000

#define GBARW_PIO      pio0
#define GBARW_SM        0

#define DMA_CHANNEL         (0) /* bit plane content DMA channel */
#define DMA_CHANNEL_MASK    (1u<<DMA_CHANNEL)

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
      pio_sm_set_enabled(GBARW_PIO, GBARW_SM, false);
      pio_sm_clear_fifos(GBARW_PIO, GBARW_SM);
      pio_sm_restart(GBARW_PIO, GBARW_SM);
      pio_sm_clkdiv_restart(GBARW_PIO, GBARW_SM);
      if (isRead){
          gba_read_program_init(GBARW_PIO, GBARW_SM, gRead_pio_pc);
          gpio_init(PIN_NWR);
          gpio_set_dir(PIN_NWR, GPIO_OUT);
          gpio_put(PIN_NWR, 1);

          gpio_init(PIN_CS2);
          gpio_set_dir(PIN_CS2, GPIO_OUT);
          gpio_put(PIN_CS2, 1);
      }else{
          // gba_write_program_init(GBARW_PIO, GBARW_SM, gWrite_pio_pc);        
      }
      pio_sm_set_enabled(GBARW_PIO, GBARW_SM, true);   
  }    
  return 0;
}

#pragma mark public
void gba_rw_init (void){
  dma_claim_mask(DMA_CHANNEL_MASK);
  for (int i=0; i<29; i++){
    gpio_set_function(i, GPIO_FUNC_PIO0);
  }

  gRead_pio_pc = pio_add_program(GBARW_PIO, &gba_read_program);
  // gWrite_pio_pc = pio_add_program(GBARW_PIO, &gba_write_program);
  gIsInited = true;
  gModeIsRead = false;
  internal_init(true);
}

void gba_rw_cleanup(void){
  if (!gIsInited) return;
  pio_sm_set_enabled(GBARW_PIO, GBARW_SM, false);
  pio_sm_clear_fifos(GBARW_PIO, GBARW_SM);
  pio_sm_restart(GBARW_PIO, GBARW_SM);
  pio_sm_clkdiv_restart(GBARW_PIO, GBARW_SM);
  dma_unclaim_mask(DMA_CHANNEL_MASK);
  pio_remove_program(GBARW_PIO, &gba_read_program, gRead_pio_pc);
  // pio_remove_program(GBARW_PIO, &gba_write_program, gWrite_pio_pc);
  gIsInited = false;
}

static void internal_pio_read_dma(void *buf, size_t bufSize, bool is8BitBus) {
  dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL); /* get default configuration */
  channel_config_set_dreq(&channel_config, pio_get_dreq(GBARW_PIO, GBARW_SM, false)); /* configure data request. true: sending data to the PIO state machine */
  if (!is8BitBus) 
    bufSize >>= 1;
  channel_config_set_transfer_data_size(&channel_config, !is8BitBus ? DMA_SIZE_16 : DMA_SIZE_8);
  channel_config_set_read_increment(&channel_config, false);
  channel_config_set_write_increment(&channel_config, true);
  dma_channel_configure(DMA_CHANNEL,
                        &channel_config,
                        buf, //write address
                        &GBARW_PIO->rxf[GBARW_SM], /* read address: read from PIO FIFO */
                        bufSize,
                        true); /* start */
}

int popPicoSM(){
  uint64_t time = time_us_64();
  while (pio_sm_is_rx_fifo_empty(GBARW_PIO, GBARW_SM)){
     if (time_us_64() - time > USEC_PER_SEC*2) return -2;
  }
  uint32_t val = pio_sm_get(GBARW_PIO, GBARW_SM);
  return val;
}

int gba_read_internal(uint32_t addr, void *buf, uint32_t size, bool is8BitBus){
  int err = 0;
  err = internal_init(true);
  if (err) return err;
  if (!size) return 0;

  uint8_t *ptr = (uint8_t*)buf;
  int didRead = 0;
  int needsRawReads = 0;

  while (!pio_sm_is_rx_fifo_empty(GBARW_PIO, GBARW_SM)) 
    pio_sm_get(GBARW_PIO, GBARW_SM);

  if (!is8BitBus){
    /*
      ROM mem access is 16bit data bus
    */
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
        internal_pio_read_dma(ptr, needsRead, is8BitBus);
        ptr += needsRead;
        didRead += needsRead;
      }

      pio_sm_put(GBARW_PIO, GBARW_SM, addr>>1);
      pio_sm_put(GBARW_PIO, GBARW_SM, needsRawReads);
    }
    
    if (didRead > 3){
      dma_channel_wait_for_finish_blocking(DMA_CHANNEL);
    }

  }else{
    //TODO
    needsRawReads = size;
    pio_sm_put(GBARW_PIO, GBARW_SM, addr);
    pio_sm_put(GBARW_PIO, GBARW_SM, needsRawReads);
  }

  while (didRead < size){
    int v = popPicoSM();
    if (v < 0) return v;
    uint8_t *pv = (uint8_t*)&v;
    *ptr++ = *pv++; didRead++;
    if (didRead < size && !is8BitBus){
      *ptr++ = *pv++; didRead++;
    }
  }

  return didRead;
}

int gba_read(uint32_t addr, void *buf, uint32_t size){
  return gba_read_internal(addr & 0xFFFFFF, buf, size, addr >= 0x1000000);
}