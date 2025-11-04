#pragma once

#include <pebble.h>

#include "roundy_layout.h"

typedef struct {
  uint8_t width;
  uint8_t rows[ROUNDY_DIGIT_HEIGHT];
} RoundyGlyph;

enum {
  ROUNDY_GLYPH_ZERO = 0,
  ROUNDY_GLYPH_ONE,
  ROUNDY_GLYPH_TWO,
  ROUNDY_GLYPH_THREE,
  ROUNDY_GLYPH_FOUR,
  ROUNDY_GLYPH_FIVE,
  ROUNDY_GLYPH_SIX,
  ROUNDY_GLYPH_SEVEN,
  ROUNDY_GLYPH_EIGHT,
  ROUNDY_GLYPH_NINE,
  ROUNDY_GLYPH_COLON,
  ROUNDY_GLYPH_COUNT
};

extern const RoundyGlyph ROUNDY_GLYPHS[ROUNDY_GLYPH_COUNT];
