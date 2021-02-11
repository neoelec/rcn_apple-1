#ifndef __EMUL_8BIT_H__
#define __EMUL_8BIT_H__

#ifndef container_of
#define container_of(__ptr, __type, __member) \
  ((__type *) ((char *) (__ptr) - offsetof(__type, __member)))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(__a)   (sizeof(__a) / sizeof((__a)[0]))
#endif

#include <list.h>

#include "emul_8bit_bus.h"
#include "emul_8bit_io.h"
#include "emul_8bit_ram.h"
#include "emul_8bit_rom.h"

#include "emul_8bit_hook.h"

#include "emul_8bit_notify.h"

#endif /* __EMUL_8BIT_H__ */
