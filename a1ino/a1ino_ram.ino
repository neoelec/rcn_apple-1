// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"

#if defined(CFG_A1INO_RAM) && (A1INO_RAM_1000_SIZE > 0x0)

#define A1INO_RAM_1000_BASE   0x1000

static struct a1ino_ram *__ram_get_instance(uint16_t size) {
  static struct a1ino_ram ram;

  ram.size = min(size, (uint16_t)A1INO_RAM_1000_SIZE);

  e8bit_ram_setup(&ram.ram_1000, A1INO_RAM_1000_BASE, ram.size, ram.mem);

  return &ram;
}

struct a1ino_ram *a1ino_ram_get_instance(uint16_t size) {
  static struct a1ino_ram *ram;

  if (ram)
    return ram;

  ram = __ram_get_instance(size);

  return ram;
}

void a1ino_ram_install(struct a1ino *emul) {
  struct a1ino_ram *ram = emul->ram;
  struct mcs6502 *cpu = a1ino_get_cpu(emul);

  e8bit_bus_attach(&cpu->bus, &ram->ram_1000.dev);
}

void a1ino_ram_uninstall(struct a1ino *emul) {
  struct a1ino_ram *ram = emul->ram;
  struct mcs6502 *cpu = a1ino_get_cpu(emul);

  e8bit_bus_detach(&cpu->bus, &ram->ram_1000.dev);
}

#endif // CFG_A1INO_RAM
