// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>

#include "emul_8bit.h"

#pragma GCC optimize ("O2")

void e8bit_notify_setup(struct e8bit_notify *notify) {
  INIT_LIST_HEAD(&notify->head);
}

void e8bit_notify_attach_notify(struct e8bit_notify *notify, 
    struct e8bit_notify_handle *handle) {
  list_add(&handle->list, &notify->head);
}

void e8bit_notify_detach(struct e8bit_notify *notify,
    struct e8bit_notify_handle *handle) {
  notify = notify;

  list_del(&handle->list);
}

void e8bit_notify_handle(struct e8bit_notify *notify,
    unsigned int i, void *d) {
  struct e8bit_notify_handle *handle;

  list_for_each_entry(handle, &notify->head, list) {
    if (handle->call)
      handle->call(handle, i, d);
  }
}
