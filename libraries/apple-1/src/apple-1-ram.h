#ifndef __APPLE_1_RAM_H__
#define __APPLE_1_RAM_H__

#include <emul_8bit.h>

#define APPLE_1_RAM_0000_SIZE   0x1000

struct apple_1_ram {
  struct e8bit_ram ram_0000;
  uint8_t ram_0000_mem[APPLE_1_RAM_0000_SIZE];
};

#endif /* __APPLE_1_RAM_H__ */
