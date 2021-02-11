// SPDX-License-Identifier: GPL-2.0
#include "emul_8bit.h"

static uint8_t e8bit_rom_read(const struct e8bit_bus_dev *dev, uint16_t addr) {
  struct e8bit_rom *rom = container_of(dev, struct e8bit_rom, dev);
  size_t offset = (size_t)(addr - dev->base);

  return pgm_read_byte(&rom->mem[offset]);
}

void e8bit_rom_setup(struct e8bit_rom *rom,
    uint16_t base, uint16_t size, const uint8_t *mem) {
  rom->dev.base = base;
  rom->dev.end = base + (size - 1);
  rom->dev.read = e8bit_rom_read;
  rom->dev.type = E_E8BIT_BUS_TYPE_ROM;
  rom->mem = mem;
}
