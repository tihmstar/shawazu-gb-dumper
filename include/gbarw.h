#ifndef GBARW_H
#define GBARW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void gba_rw_init (void);
void gba_rw_cleanup(void);

int gba_read_word (uint32_t addr);
int gba_write_word(uint32_t addr, uint16_t data);

int gba_read (uint32_t addr, void *buf, int size);
int gba_write(uint32_t addr, const void *buf, int size);

#ifdef __cplusplus
}
#endif

#endif // GBRW_H