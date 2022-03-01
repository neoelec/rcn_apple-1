// SPDX-License-Identifier: GPL-2.0
#include "emul_8bit.h"

static uint8_t e8bit_ram_read(const struct e8bit_bus_dev *dev,
    uint16_t addr) {
  struct e8bit_ram *ram = container_of(dev, struct e8bit_ram, dev);
  size_t offset = (size_t)(addr - dev->base);

  return ram->mem[offset];
}

static void e8bit_ram_write(const struct e8bit_bus_dev *dev,
    uint16_t addr, uint8_t data) {
  struct e8bit_ram *ram = container_of(dev, struct e8bit_ram, dev);
  size_t offset = (size_t)(addr - dev->base);

  ram->mem[offset] = data;
}

void e8bit_ram_setup(struct e8bit_ram *ram,
    uint16_t base, uint16_t size, uint8_t *mem) {
  struct e8bit_bus_dev *dev = &ram->dev;

  dev->base = base;
  dev->end = base + (size - 1);
  dev->read = e8bit_ram_read;
  dev->write = e8bit_ram_write;
  dev->type = E_E8BIT_BUS_TYPE_RAM;

  ram->mem = mem;
}
