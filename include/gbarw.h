#ifndef GBARW_H
#define GBARW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GBA_SAVEGAME_MAP_ADDRESS 0x1000000

void gba_rw_init (void);
void gba_rw_cleanup(void);

int gba_read (uint32_t addr, void *buf, uint32_t size);
int gba_write(uint32_t addr, const void *buf, uint32_t size);

int gba_read_byte(uint32_t addr);
int gba_write_byte(uint32_t addr, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif // GBRW_H