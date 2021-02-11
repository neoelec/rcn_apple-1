// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>

#include "emul_8bit.h"

#pragma GCC optimize ("O2")

void e8bit_bus_setup(struct e8bit_bus *bus) {
  bus->root = RB_ROOT;
}

void e8bit_bus_attach(struct e8bit_bus *bus, struct e8bit_bus_dev *dev) {
  struct rb_root *root = &bus->root;
  struct rb_node **p = &root->rb_node;
  struct rb_node *parent = NULL;
  struct e8bit_bus_dev *tmp;

  while (*p) {
    parent = *p;

    tmp = rb_entry(parent, struct e8bit_bus_dev, node);

    if (dev->end < tmp->base)
      p = &(*p)->rb_left;
    else if (dev->base > tmp->end)
      p = &(*p)->rb_right;
    else                        /* maybe a bug */
      return;
  }

  rb_link_node(&dev->node, parent, p);
  rb_insert_color(&dev->node, root);
}

void e8bit_bus_detach(struct e8bit_bus *bus, struct e8bit_bus_dev *dev) {
  struct rb_root *root = &bus->root;

  rb_erase(&dev->node, root);
}

static inline struct e8bit_bus_dev *__bus_find_dev(struct e8bit_bus *bus,
    uint16_t addr) {
  struct rb_root *root = &bus->root;
  struct rb_node *n = root->rb_node;
  struct e8bit_bus_dev *dev;

  while (n) {
    dev = rb_entry(n, struct e8bit_bus_dev, node);

    if (addr < dev->base)
      n = n->rb_left;
    else if (addr > dev->end)
      n = n->rb_right;
    else
      return dev;
  }

  return NULL;
}

struct e8bit_bus_dev *e8bit_bus_read(struct e8bit_bus *bus,
    uint16_t addr, uint8_t *val) {
  struct e8bit_bus_dev *dev;

  dev = __bus_find_dev(bus, addr);
  if (dev && dev->read) {
    *val = dev->read(dev, addr);

    return dev;
  }

  *val = 0x00;

  return NULL;
}

struct e8bit_bus_dev *e8bit_bus_write(struct e8bit_bus *bus,
    uint16_t addr, uint8_t val) {
  struct e8bit_bus_dev *dev;

  dev = __bus_find_dev(bus, addr);
  if (dev && dev->write) {
    dev->write(dev, addr, val);

    return dev;
  }

  return NULL;
}
