#ifndef __A1INO_ROM_H__
#define __A1INO_ROM_H__

#include "a1ino_pgm.h"

struct a1ino_rom {
  struct a1ino_pgm **pgm;
  size_t nr_pgm;
};

extern struct a1ino_rom *a1ino_rom_get_instance(struct a1ino_pgm **pgm, size_t nr_pgm);

extern void a1ino_rom_install(struct a1ino *emul);
extern void a1ino_rom_uninstall(struct a1ino *emul);

#endif // __A1INO_ROM_H__
