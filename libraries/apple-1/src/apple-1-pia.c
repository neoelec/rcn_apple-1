// SPDX-License-Identifier: GPL-2.0
#include "apple-1.h"

#define APPLE_1_PIA_BASE    0xD000
#define APPLE_1_PIA_SIZE    0x0100

static e8bit_bus_read_t __e8bit_io_read;
static e8bit_bus_write_t __e8bit_io_write;

/* A4 : CS0
 * A1 : RS1
 * A0 : RS0
 */
static uint8_t apple_1_pia_io_read(const struct e8bit_bus_dev *dev, uint16_t addr) {
  return __e8bit_io_read(dev, addr & 0xD003);
}

static void apple_1_pia_io_write(const struct e8bit_bus_dev *dev, uint16_t addr, uint8_t data) {
  __e8bit_io_write(dev, addr & 0xD003, data);
}

void apple_1_pia_setup(struct mc6820 *pia) {
  mc6820_setup(pia, APPLE_1_PIA_BASE);

  pia->io.dev.end = pia->io.dev.base + (APPLE_1_PIA_SIZE - 0x1);

  __e8bit_io_read = pia->io.dev.read;
  pia->io.dev.read = apple_1_pia_io_read;

  __e8bit_io_write = pia->io.dev.write;
  pia->io.dev.write = apple_1_pia_io_write;
}

void apple_1_pia_install(struct mc6820 *pia, struct e8bit_bus *bus) {
  mc6820_install(pia, bus);
}

void apple_1_pia_uninstall(struct mc6820 *pia, struct e8bit_bus *bus) {
  mc6820_uninstall(pia, bus);
}
