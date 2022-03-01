// SPDX-License-Identifier: GPL-2.0
#include "emul_8bit.h"

void e8bit_io_attach_dev(struct e8bit_io *io, struct e8bit_io_dev *dev) {
  dev->io = io;

  list_add(&dev->list, &io->head);
}

void e8bit_io_detach_dev(struct e8bit_io *io, struct e8bit_io_dev *dev) {
  io = io;

  list_del(&dev->list);
}

static void __e8bit_io_read(struct list_head *head,
    uint8_t *mem, uint16_t offset) {
  struct e8bit_io_dev *dev;

  list_for_each_entry(dev, head, list) {
    if (dev->read)
      dev->read(dev, mem, offset);
  }
}

static uint8_t e8bit_io_read(const struct e8bit_bus_dev *dev, uint16_t addr) {
  struct e8bit_io *io = container_of(dev, struct e8bit_io, dev);
  uint16_t offset = addr - dev->base;

  __e8bit_io_read(&io->head, io->mem, offset);

  return io->mem[offset];
}

static void __e8bit_io_write(struct list_head *head,
    uint8_t *mem, uint16_t offset) {
  struct e8bit_io_dev *dev;

  list_for_each_entry(dev, head, list) {
    if (dev->write)
      dev->write(dev, mem, offset);
  }
}

static void e8bit_io_write(const struct e8bit_bus_dev *dev,
    uint16_t addr, uint8_t data) {
  struct e8bit_io *io = container_of(dev, struct e8bit_io, dev);
  uint16_t offset = addr - dev->base;

  io->mem[offset] = data;

  __e8bit_io_write(&io->head, io->mem, offset);
}

void e8bit_io_setup(struct e8bit_io *io,
    uint16_t base, uint16_t size, uint8_t *mem) {
  struct e8bit_bus_dev *dev = &io->dev;

  dev->base = base;
  dev->end = base + (size - 1);
  dev->read = e8bit_io_read;
  dev->write = e8bit_io_write;
  dev->type = E_E8BIT_BUS_TYPE_IO;

  INIT_LIST_HEAD(&io->head);
  io->mem = mem;
}
