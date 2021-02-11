#ifndef __APPLE_1_H__
#define __APPLE_1_H__

#include <stdint.h>

#include <emul_8bit.h>
#include <mcs6502.h>
#include <mc6820.h>

#include "apple-1-ram.h"
#include "apple-1-rom.h"

struct apple_1 {
  struct mcs6502 cpu;

  struct apple_1_ram ram;
  struct apple_1_rom rom;

  struct mc6820 pia;
};

#ifdef __cplusplus
extern "C" {
#endif

void apple_1_setup(struct apple_1 *board);
void apple_1_construct(struct apple_1 *board);
void apple_1_destruct(struct apple_1 *board);

#ifdef __cplusplus
}
#endif
#endif /* __APPLE_1_H__ */
