#pragma once

#include <pebble.h>

enum {
  ROUNDY_GRID_COLS = 24,
  ROUNDY_GRID_ROWS = 28,
  ROUNDY_CELL_SIZE = 6,
  ROUNDY_DIGIT_WIDTH = 4,
  ROUNDY_DIGIT_HEIGHT = 9,
  ROUNDY_DIGIT_COLON_WIDTH = 2,
  ROUNDY_DIGIT_COUNT = 4,
  ROUNDY_DIGIT_GAP = 1,
  ROUNDY_DIGIT_START_COL = 1,
  ROUNDY_DIGIT_START_ROW = 10,
};

static inline GPoint roundy_cell_origin(int cell_col, int cell_row) {
  return GPoint(cell_col * ROUNDY_CELL_SIZE, cell_row * ROUNDY_CELL_SIZE);
}

static inline GRect roundy_cell_frame(int cell_col, int cell_row) {
  return GRect(cell_col * ROUNDY_CELL_SIZE, cell_row * ROUNDY_CELL_SIZE,
               ROUNDY_CELL_SIZE, ROUNDY_CELL_SIZE);
}
