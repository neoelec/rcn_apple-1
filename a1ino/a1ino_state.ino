// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"

void a1ino_state_reset(struct a1ino *emul) {
  mcs6502_reset(&emul->board->cpu);

  a1ino_tty_reset(emul);

  emul->state_machine = a1ino_state_run;
}

void a1ino_state_run(struct a1ino *emul) {
  mcs6502_exec(&emul->board->cpu, 128);

  a1ino_tty_run(emul);
}
