// SPDX-License-Identifier: GPL-2.0
#include "apple-1.h"

#define APPLE_1_RAM_0000_BASE   0x0000

void apple_1_ram_setup(struct apple_1_ram *ram) {
  e8bit_ram_setup(&ram->ram_0000,
      APPLE_1_RAM_0000_BASE, APPLE_1_RAM_0000_SIZE, ram->ram_0000_mem);
}

void apple_1_ram_install(struct apple_1_ram *ram, struct e8bit_bus *bus) {
  e8bit_bus_attach(bus, &ram->ram_0000.dev);
}

void apple_1_ram_uninstall(struct apple_1_ram *ram, struct e8bit_bus *bus) {
  e8bit_bus_detach(bus, &ram->ram_0000.dev);
}
