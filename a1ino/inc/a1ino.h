#ifndef __A1INO_H__
#define __A1INO_H__

#include "a1ino_cfg.h"

#include "a1ino_board.h"
#include "a1ino_pgm.h"
#include "a1ino_ram.h"
#include "a1ino_rom.h"
#include "a1ino_state.h"
#include "a1ino_tty.h"
#include "a1ino_util.h"

struct a1ino {
  struct apple_1 *board;
  struct a1ino_ram *ram;
  struct a1ino_rom *rom;
  struct a1ino_tty *tty;

  void (*state_machine)(struct a1ino *);
};

#include "a1ino_factory.h"

inline struct mcs6502 *a1ino_get_cpu(struct a1ino *emul) {
  return &emul->board->cpu;
}

inline struct mc6820 *a1ino_get_pia(struct a1ino *emul) {
  return &emul->board->pia;
}

#endif // __A1INO_H__
