// SPDX-License-Identifier: GPL-2.0
#include "mc6820.h"

void mc6820_setup(struct mc6820 *pia, uint16_t io_base) {
  e8bit_io_setup(&pia->io, io_base, MC6820_IO_SIZE, pia->io_area);
}

void mc6820_install(struct mc6820 *pia, struct e8bit_bus *bus) {
  e8bit_bus_attach(bus, &pia->io.dev);
}

void mc6820_uninstall(struct mc6820 *pia, struct e8bit_bus *bus) {
  e8bit_bus_detach(bus, &pia->io.dev);
}
