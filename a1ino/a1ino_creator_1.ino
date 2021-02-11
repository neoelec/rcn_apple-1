// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"
#include "inc/pgm.h"

static struct a1ino_pgm **__creator_1_setup_pgm(size_t *nr_pgm) {
  static struct a1ino_pgm *pgm[1];

  pgm[0] = pgm_integer_basic_get_instance();

  *nr_pgm = ARRAY_SIZE(pgm);

  return pgm;
}

static void __creator_1(struct a1ino *emul) {
  static struct a1ino_pgm **pgm;
  static size_t nr_pgm;

  if (!pgm)
    pgm = __creator_1_setup_pgm(&nr_pgm);

  emul->rom = a1ino_rom_get_instance(pgm, nr_pgm);
}

void a1ino_creator_1(struct a1ino *emul) {
  a1ino_creator_template(emul, __creator_1);
}
