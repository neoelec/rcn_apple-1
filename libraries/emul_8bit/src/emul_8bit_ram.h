#ifndef __EMUL_8BIT_RAM_H__
#define __EMUL_8BIT_RAM_H__

#include <stdint.h>

#include "emul_8bit_bus.h"

struct e8bit_ram {
  struct e8bit_bus_dev dev;
  uint8_t *mem;
};

#ifdef __cplusplus
extern "C" {
#endif

extern void e8bit_ram_setup(struct e8bit_ram *ram, uint16_t base, uint16_t size, uint8_t *mem);

#ifdef __cplusplus
}
#endif

#endif /* __EMUL_8BIT_RAM_H__ */
