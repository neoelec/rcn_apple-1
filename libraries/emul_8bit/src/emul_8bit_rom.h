#ifndef __EMUL_8BIT_ROM_H__
#define __EMUL_8BIT_ROM_H__

#include <stdint.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#ifdef PROGMEM
#undef PROGMEM
#endif
#define PROGMEM
#ifndef pgm_read_byte
#define pgm_read_byte(__ptr)    *((uint8_t *)(__ptr))
#endif
#endif /* __AVR_ARCH__ */

#include "emul_8bit_bus.h"

struct e8bit_rom {
  struct e8bit_bus_dev dev;
  const uint8_t *mem;
};

#ifdef __cplusplus
extern "C" {
#endif

extern void e8bit_rom_setup(struct e8bit_rom *rom, uint16_t base, uint16_t size, const uint8_t *mem);

#ifdef __cplusplus
}
#endif

#endif /* __EMUL_8BIT_ROM_H__ */
