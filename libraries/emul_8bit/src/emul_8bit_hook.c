// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>

#include "emul_8bit.h"

#pragma GCC optimize ("O2")

void e8bit_hook_setup(struct e8bit_hook *hook) {
  hash_init(hook->htbl);
}

void e8bit_hook_attach(struct e8bit_hook *hook,
    struct e8bit_hook_handle *handle) {
  INIT_HLIST_NODE(&handle->node);

  hash_add(hook->htbl, &handle->node, handle->ptr & 0x0F);
}

void e8bit_hook_detach(struct e8bit_hook *hook,
    struct e8bit_hook_handle *handle) {
  hook = hook;

  hash_del(&handle->node);
}

int e8bit_hook_handle(struct e8bit_hook *hook, uint16_t addr, void *d) {
  struct e8bit_hook_handle *handle;
  int ret = EMUL_8BIT_HOOK_CONTINUE;

  hash_for_each_possible(hook->htbl, handle, node, addr & 0x0F) {
    if ((addr == handle->ptr) && handle->call)
      ret |= handle->call(handle, d);
  }

  return ret;
}
