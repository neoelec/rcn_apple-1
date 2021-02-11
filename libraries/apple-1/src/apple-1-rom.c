
// SPDX-License-Identifier: GPL-2.0
#include "apple-1.h"

static const uint8_t __woz_monitor_rom_ff00[] PROGMEM = {
#include "woz-monitor_ff00.h"
};

#define WOZ_MONITOR_ROM_BASE    0xFF00
#define WOZ_MONITOR_ROM_SIZE    sizeof(__woz_monitor_rom_ff00)

void apple_1_rom_setup(struct apple_1_rom *rom) {
  e8bit_rom_setup(&rom->rom_ff00,
      WOZ_MONITOR_ROM_BASE, WOZ_MONITOR_ROM_SIZE, __woz_monitor_rom_ff00);
}

void apple_1_rom_install(struct apple_1_rom *rom, struct e8bit_bus *bus) {
  e8bit_bus_attach(bus, &rom->rom_ff00.dev);
}

void apple_1_rom_uninstall(struct apple_1_rom *rom, struct e8bit_bus *bus) {
  e8bit_bus_detach(bus, &rom->rom_ff00.dev);
}
