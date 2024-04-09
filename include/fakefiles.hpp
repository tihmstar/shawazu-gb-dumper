#ifndef FAKEFILES_HPP
#define FAKEFILES_HPP

#include "Cartridge.hpp"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *fakefiles_read_infotxt(Cartridge *cart, size_t *outSize);

int32_t fakefiles_read_rom(Cartridge *cart, uint32_t offset, void *buf, uint32_t bufSize);
int32_t fakefiles_read_ram(Cartridge *cart, uint32_t offset, void *buf, uint32_t bufSize);
int32_t fakefiles_write_ram(Cartridge *cart, uint32_t offset, const void *buf, uint32_t bufSize);

const void *gbcam_read_image(Cartridge *cart, int which, size_t *bufSize);

#ifdef __cplusplus
}
#endif

#endif // FAKEFILES_HPP