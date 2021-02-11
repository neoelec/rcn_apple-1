// SPDX-License-Identifier: GPL-2.0
#include "apple-1.h"

#include "__apple-1-pia.h"
#include "__apple-1-ram.h"
#include "__apple-1-rom.h"

void apple_1_setup(struct apple_1 *board) {
  mcs6502_setup(&board->cpu);

  apple_1_pia_setup(&board->pia);
  apple_1_ram_setup(&board->ram);
  apple_1_rom_setup(&board->rom);
}

void apple_1_construct(struct apple_1 *board) {
  struct e8bit_bus *bus = &board->cpu.bus;

  apple_1_pia_install(&board->pia, bus);
  apple_1_ram_install(&board->ram, bus);
  apple_1_rom_install(&board->rom, bus);
}

void apple_1_destruct(struct apple_1 *board) {
  struct e8bit_bus *bus = &board->cpu.bus;

  apple_1_pia_uninstall(&board->pia, bus);
  apple_1_ram_uninstall(&board->ram, bus);
  apple_1_rom_uninstall(&board->rom, bus);
}
