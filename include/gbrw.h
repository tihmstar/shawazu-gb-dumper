#ifndef GBRW_H
#define GBRW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void gb_rw_init (void);
void gb_rw_cleanup(void);

int gb_read_byte (uint16_t addr);
int gb_write_byte(uint16_t addr, uint8_t data);

int gb_read (uint16_t addr, void *buf, int size);
int gb_write(uint16_t addr, const void *buf, int size);

#ifdef __cplusplus
}
#endif

#endif // GBRW_H