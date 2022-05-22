// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"

static struct a1ino *emul;

static void __print_banner(void) {
  A1INO_Serial.print(F("\033[2J")); // clear screen
  A1INO_Serial.println(F("Raccoon's Apple 1 Replica"));
  A1INO_Serial.println();
}

static void __select_emulator_type(void) {
  int prod_id, prod_id_max;
  static const char prompt[] PROGMEM = "Select Device Type [1 - %d] : ";

  prod_id_max = a1ino_get_nr_prod();

  a1ino_printf_P(prompt, prod_id_max);
  while (!A1INO_Serial.available()) ;
  prod_id = A1INO_Serial.parseInt();
  A1INO_Serial.println(prod_id);
  A1INO_Serial.println();

  emul = a1ino_factory(prod_id);
}

void setup(void) {
  // put your setup code here, to run once:
  A1INO_Serial.begin(115200);
  delay(1500);

  __print_banner();
  a1ino_print_supported_product();
  __select_emulator_type();

  emul->state_machine = a1ino_state_reset;
}

void loop(void) {
  // put your main code here, to run repeatedly:
  emul->state_machine(emul);
}
