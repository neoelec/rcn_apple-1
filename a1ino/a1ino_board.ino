// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"

static struct apple_1 *__board_get_instance(void) {
  static struct apple_1 board;

  apple_1_setup(&board);
  apple_1_construct(&board);

  return &board;
}

struct apple_1 *a1ino_board_get_instance(void) {
  static struct apple_1 *board;

  if (board)
    return board;

  board = __board_get_instance();

  return board;
}
