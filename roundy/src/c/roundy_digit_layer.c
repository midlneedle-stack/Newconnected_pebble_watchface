#include "roundy_digit_layer.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "roundy_glyphs.h"
#include "roundy_layout.h"
#include "roundy_palette.h"
#include <math.h>

/* Animation tuning */
#define DIAG_FRAME_MS 16 /* target frame interval in ms (approx 60Hz -> 16ms) */
#define DIAG_DURATION_MS 480 /* total animation duration in ms (gradual reveal) */
/* initial delay before starting the first animation frame (user requested value) */
#define DIAG_START_DELAY_MS 240
#define ROUNDY_GLYPH_DURATION 1.0f
#define ROUNDY_GLYPH_STAGGER 1.0f

/* digits + colon */
#define ROUNDY_ANIMATED_GLYPH_COUNT (ROUNDY_DIGIT_COUNT + 1)

/* choose animation color based on progress: three steps
 * 0.0 - 0.333: #555555
 * 0.333 - 0.666: #AAAAAA
 * 0.666 - 1.0: #FFFFFF
 */
static GColor prv_anim_color(float p) {
  if (p < (1.0f / 3.0f)) {
    return PBL_IF_COLOR_ELSE(GColorFromRGB(0x55, 0x55, 0x55), GColorBlack);
  } else if (p < (2.0f / 3.0f)) {
    return PBL_IF_COLOR_ELSE(GColorFromRGB(0xAA, 0xAA, 0xAA), GColorBlack);
  } else {
    return PBL_IF_COLOR_ELSE(GColorFromRGB(0xFF, 0xFF, 0xFF), GColorWhite);
  }
}

typedef struct {
  int16_t digits[ROUNDY_DIGIT_COUNT];
  int16_t prev_digits[ROUNDY_DIGIT_COUNT];
  bool use_24h_time;
  /* animation state */
  AppTimer *anim_timer;
  float anim_time;
  float glyph_start[ROUNDY_ANIMATED_GLYPH_COUNT];
  bool glyph_active[ROUNDY_ANIMATED_GLYPH_COUNT];
  bool diag_mode_active;
  float diag_time;
} RoundyDigitLayerState;

struct RoundyDigitLayer {
  Layer *layer;
  RoundyDigitLayerState *state;
};

static void prv_diag_anim_timer(void *ctx);

static inline RoundyDigitLayerState *prv_get_state(RoundyDigitLayer *layer) {
  return layer ? layer->state : NULL;
}

static float prv_glyph_progress(const RoundyDigitLayerState *state, int glyph_index) {
  if (!state->glyph_active[glyph_index]) {
    return 1.0f;
  }
  const float elapsed = state->anim_time - state->glyph_start[glyph_index];
  if (elapsed <= 0.0f) {
    return 0.0f;
  }
  if (elapsed >= ROUNDY_GLYPH_DURATION) {
    return 1.0f;
  }
  return elapsed / ROUNDY_GLYPH_DURATION;
}

static bool prv_any_glyph_active(const RoundyDigitLayerState *state) {
  for (int i = 0; i < ROUNDY_ANIMATED_GLYPH_COUNT; ++i) {
    if (state->glyph_active[i]) {
      return true;
    }
  }
  return false;
}

static void prv_configure_glyph_animation(RoundyDigitLayerState *state, const bool mask[]) {
  if (!state || !mask) {
    return;
  }

  state->diag_mode_active = false;
  state->diag_time = 0.0f;
  state->anim_time = 0.0f;
  float next_start = 0.0f;
  for (int i = 0; i < ROUNDY_ANIMATED_GLYPH_COUNT; ++i) {
    if (mask[i]) {
      state->glyph_active[i] = true;
      state->glyph_start[i] = next_start;
      next_start += ROUNDY_GLYPH_STAGGER;
    } else {
      state->glyph_active[i] = false;
      state->glyph_start[i] = 0.0f;
    }
  }
}

static void prv_start_glyph_animation(RoundyDigitLayer *rdl, const bool mask[],
                                      uint32_t initial_delay_ms) {
  if (!rdl || !rdl->layer) {
    return;
  }
  RoundyDigitLayerState *state = layer_get_data(rdl->layer);
  if (!state) {
    return;
  }

  if (state->anim_timer) {
    app_timer_cancel(state->anim_timer);
    state->anim_timer = NULL;
  }

  prv_configure_glyph_animation(state, mask);

  if (!prv_any_glyph_active(state)) {
    layer_mark_dirty(rdl->layer);
    return;
  }

  state->anim_timer =
      app_timer_register(initial_delay_ms, prv_diag_anim_timer, rdl->layer);
  layer_mark_dirty(rdl->layer);
}

static float prv_diag_glyph_progress(float time, int glyph_index) {
  const float start = (float)glyph_index * ROUNDY_GLYPH_STAGGER;
  const float local = time - start;
  if (local <= 0.0f) {
    return 0.0f;
  }
  float progress = local / ROUNDY_GLYPH_DURATION;
  if (progress >= 1.0f) {
    return 1.0f;
  }
  return progress;
}

static inline int prv_digit_index_for_glyph(int glyph_index) {
  if (glyph_index < 0) {
    return -1;
  }
  if (glyph_index <= 1) {
    return glyph_index;
  }
  if (glyph_index >= 3) {
    return glyph_index - 1;
  }
  return -1; /* colon */
}

/* Draw a single digit cell. `progress` interpolates the diagonal from the
 * original '\' (progress == 0) to '/' (progress == 1). */
static void prv_draw_digit_cell(GContext *ctx, int cell_col, int cell_row,
                                float progress) {
  const GRect frame = roundy_cell_frame(cell_col, cell_row);
  graphics_fill_rect(ctx, frame, 0, GCornerNone);

  const int origin_x = frame.origin.x;
  const int origin_y = frame.origin.y;
  for (int idx = 0; idx < ROUNDY_CELL_SIZE; ++idx) {
    /* interpolate between '\' and '/' diagonals */
    const int from_x = idx;
    const int to_x = (ROUNDY_CELL_SIZE - 1 - idx);
    const int x =
        origin_x + (int)roundf(((1.0f - progress) * from_x) + (progress * to_x));
    const int y = origin_y + idx;
    graphics_draw_pixel(ctx, GPoint(x, y));
  }
}

static void prv_draw_glyph(GContext *ctx, const RoundyGlyph *glyph, int cell_col,
                           int cell_row, float progress, GColor base_stroke) {
  if (!glyph) {
    return;
  }

  const int max_diag = (glyph->width - 1) + (ROUNDY_DIGIT_HEIGHT - 1);
  const float total = progress * (float)(max_diag + 1);

  for (int row = 0; row < ROUNDY_DIGIT_HEIGHT; ++row) {
    const uint8_t mask = glyph->rows[row];
    if (!mask) {
      continue;
    }

  for (int col = 0; col < glyph->width; ++col) {
      if (mask & (1 << (glyph->width - 1 - col))) {
        const int diag_index = row + col;
        float cell_progress = total - (float)diag_index;
        if (cell_progress <= 0.0f) {
          continue;
        }
        if (cell_progress > 1.0f) {
          cell_progress = 1.0f;
        }
        const GColor stroke =
            (cell_progress >= 1.0f) ? base_stroke : prv_anim_color(cell_progress);
        graphics_context_set_stroke_color(ctx, stroke);
        prv_draw_digit_cell(ctx, cell_col + col, cell_row + row, cell_progress);
      }
    }
  }
  graphics_context_set_stroke_color(ctx, base_stroke);
}

static void prv_draw_digit(GContext *ctx, int16_t digit, int cell_col,
                           int cell_row, float progress, GColor base_stroke) {
  if (digit < ROUNDY_GLYPH_ZERO || digit > ROUNDY_GLYPH_NINE) {
    return;
  }

  prv_draw_glyph(ctx, &ROUNDY_GLYPHS[digit], cell_col, cell_row, progress,
                 base_stroke);
}

static void prv_draw_colon(GContext *ctx, int cell_col, int cell_row,
                           float progress, GColor base_stroke) {
  prv_draw_glyph(ctx, &ROUNDY_GLYPHS[ROUNDY_GLYPH_COLON], cell_col, cell_row,
                 progress, base_stroke);
}

static void prv_draw_glyph_slot(GContext *ctx, RoundyDigitLayerState *state,
                                int glyph_index, int cell_col, int cell_row,
                                GColor base_stroke) {
  const bool animating = state->glyph_active[glyph_index];
  float progress = prv_glyph_progress(state, glyph_index);
  if (!animating) {
    if (glyph_index == 2) {
      prv_draw_colon(ctx, cell_col, cell_row, 1.0f, base_stroke);
    } else {
      const int digit_idx = prv_digit_index_for_glyph(glyph_index);
      if (digit_idx >= 0) {
        prv_draw_digit(ctx, state->digits[digit_idx], cell_col, cell_row, 1.0f,
                       base_stroke);
      }
    }
    return;
  }

  if (glyph_index == 2) {
    /* colon only animates in one direction */
    prv_draw_colon(ctx, cell_col, cell_row, progress, base_stroke);
    return;
  }

  const int digit_idx = prv_digit_index_for_glyph(glyph_index);
  if (digit_idx < 0) {
    return;
  }

  const int16_t new_digit = state->digits[digit_idx];
  const int16_t old_digit = state->prev_digits[digit_idx];

  const bool has_old = (old_digit >= ROUNDY_GLYPH_ZERO &&
                        old_digit <= ROUNDY_GLYPH_NINE);
  const bool has_new = (new_digit >= ROUNDY_GLYPH_ZERO &&
                        new_digit <= ROUNDY_GLYPH_NINE);

  const float phase = progress * 2.0f; /* 0-2 */

  if (has_old) {
    const float exit_phase = phase;
    if (exit_phase < 1.0f) {
      float exit_progress = 1.0f - exit_phase;
      if (exit_progress < 0.0f) {
        exit_progress = 0.0f;
      } else if (exit_progress > 1.0f) {
        exit_progress = 1.0f;
      }
      prv_draw_digit(ctx, old_digit, cell_col, cell_row, exit_progress,
                     base_stroke);
    }
  }

  if (has_new) {
    const float enter_start = has_old ? 1.0f : 0.0f;
    if (phase >= enter_start) {
      float enter_phase = phase - enter_start;
      if (enter_phase < 0.0f) {
        enter_phase = 0.0f;
      } else if (enter_phase > 1.0f) {
        enter_phase = 1.0f;
      }
      prv_draw_digit(ctx, new_digit, cell_col, cell_row, enter_phase,
                     base_stroke);
    }
  }
}

static void prv_digit_layer_update_proc(Layer *layer, GContext *ctx) {
  RoundyDigitLayerState *state = layer_get_data(layer);
  if (!state) {
    return;
  }

  graphics_context_set_fill_color(ctx, roundy_palette_digit_fill());
  const int cell_row = ROUNDY_DIGIT_START_ROW;

  if (state->diag_mode_active) {
    const float diag_total = (ROUNDY_ANIMATED_GLYPH_COUNT - 1) * ROUNDY_GLYPH_STAGGER + ROUNDY_GLYPH_DURATION;
    float color_phase = state->diag_time / diag_total;
    if (color_phase > 1.0f) {
      color_phase = 1.0f;
    } else if (color_phase < 0.0f) {
      color_phase = 0.0f;
    }
    const GColor base_stroke = roundy_palette_digit_stroke();
    graphics_context_set_stroke_color(ctx, prv_anim_color(color_phase));

    int cell_col = ROUNDY_DIGIT_START_COL;
    float glyph_progress = prv_diag_glyph_progress(state->diag_time, 0);
    if (state->digits[0] >= ROUNDY_GLYPH_ZERO &&
        state->digits[0] <= ROUNDY_GLYPH_NINE) {
      prv_draw_digit(ctx, state->digits[0], cell_col, cell_row, glyph_progress,
                     base_stroke);
    }
    cell_col += ROUNDY_DIGIT_WIDTH + ROUNDY_DIGIT_GAP;

    glyph_progress = prv_diag_glyph_progress(state->diag_time, 1);
    if (state->digits[1] >= ROUNDY_GLYPH_ZERO &&
        state->digits[1] <= ROUNDY_GLYPH_NINE) {
      prv_draw_digit(ctx, state->digits[1], cell_col, cell_row, glyph_progress,
                     base_stroke);
    }
    cell_col += ROUNDY_DIGIT_WIDTH + ROUNDY_DIGIT_GAP;

    glyph_progress = prv_diag_glyph_progress(state->diag_time, 2);
    prv_draw_colon(ctx, cell_col, cell_row, glyph_progress, base_stroke);
    cell_col += ROUNDY_DIGIT_COLON_WIDTH + ROUNDY_DIGIT_GAP;

    glyph_progress = prv_diag_glyph_progress(state->diag_time, 3);
    if (state->digits[2] >= ROUNDY_GLYPH_ZERO &&
        state->digits[2] <= ROUNDY_GLYPH_NINE) {
      prv_draw_digit(ctx, state->digits[2], cell_col, cell_row, glyph_progress,
                     base_stroke);
    }
    cell_col += ROUNDY_DIGIT_WIDTH + ROUNDY_DIGIT_GAP;

    glyph_progress = prv_diag_glyph_progress(state->diag_time, 4);
    if (state->digits[3] >= ROUNDY_GLYPH_ZERO &&
        state->digits[3] <= ROUNDY_GLYPH_NINE) {
      prv_draw_digit(ctx, state->digits[3], cell_col, cell_row, glyph_progress,
                     base_stroke);
    }
    return;
  }

  const GColor base_stroke = roundy_palette_digit_stroke();
  graphics_context_set_stroke_color(ctx, base_stroke);

  int cell_col = ROUNDY_DIGIT_START_COL;
  for (int glyph_index = 0; glyph_index < ROUNDY_ANIMATED_GLYPH_COUNT;
       ++glyph_index) {
    prv_draw_glyph_slot(ctx, state, glyph_index, cell_col, cell_row,
                        base_stroke);
    switch (glyph_index) {
      case 0:
      case 1:
      case 3:
        cell_col += ROUNDY_DIGIT_WIDTH + ROUNDY_DIGIT_GAP;
        break;
      case 2:
        cell_col += ROUNDY_DIGIT_COLON_WIDTH + ROUNDY_DIGIT_GAP;
        break;
      default:
        break;
    }
  }
}

RoundyDigitLayer *roundy_digit_layer_create(GRect frame) {
  RoundyDigitLayer *layer = calloc(1, sizeof(*layer));
  if (!layer) {
    return NULL;
  }

  layer->layer = layer_create_with_data(frame, sizeof(RoundyDigitLayerState));
  if (!layer->layer) {
    free(layer);
    return NULL;
  }

  layer->state = layer_get_data(layer->layer);
  layer->state->use_24h_time = clock_is_24h_style();
  layer->state->anim_time = ROUNDY_GLYPH_DURATION;
  layer->state->diag_mode_active = false;
  layer->state->diag_time = (ROUNDY_ANIMATED_GLYPH_COUNT - 1) * ROUNDY_GLYPH_STAGGER + ROUNDY_GLYPH_DURATION;
  for (int i = 0; i < ROUNDY_ANIMATED_GLYPH_COUNT; ++i) {
    layer->state->glyph_active[i] = false;
    layer->state->glyph_start[i] = 0.0f;
  }
  for (int i = 0; i < ROUNDY_DIGIT_COUNT; ++i) {
    layer->state->digits[i] = -1;
    layer->state->prev_digits[i] = -1;
  }

  layer_set_update_proc(layer->layer, prv_digit_layer_update_proc);
  return layer;
}

void roundy_digit_layer_destroy(RoundyDigitLayer *layer) {
  if (!layer) {
    return;
  }
  if (layer->layer) {
    /* cancel any running animation timer stored in layer data */
    RoundyDigitLayerState *state = layer_get_data(layer->layer);
    if (state && state->anim_timer) {
      app_timer_cancel(state->anim_timer);
      state->anim_timer = NULL;
    }
    layer_destroy(layer->layer);
  }
  free(layer);
}

Layer *roundy_digit_layer_get_layer(RoundyDigitLayer *layer) {
  return layer ? layer->layer : NULL;
}

/* Animation timer callback: ctx is the Layer* whose data is RoundyDigitLayerState */
static void prv_diag_anim_timer(void *ctx) {
  Layer *layer = (Layer *)ctx;
  if (!layer) {
    return;
  }
  RoundyDigitLayerState *state = layer_get_data(layer);
  if (!state) {
    return;
  }
  const int FRAME_MS = DIAG_FRAME_MS;
  const int DURATION_MS = DIAG_DURATION_MS;
  const float delta = (float)FRAME_MS / (float)DURATION_MS;

  if (state->diag_mode_active) {
    state->diag_time += delta;
    const float diag_total = (ROUNDY_ANIMATED_GLYPH_COUNT - 1) * ROUNDY_GLYPH_STAGGER + ROUNDY_GLYPH_DURATION;
    if (state->diag_time >= diag_total) {
      state->diag_time = diag_total;
      state->diag_mode_active = false;
      state->anim_timer = NULL;
    } else {
      state->anim_timer =
          app_timer_register(FRAME_MS, prv_diag_anim_timer, layer);
    }
    layer_mark_dirty(layer);
    return;
  }

  state->anim_time += delta;
  bool still_active = false;
  for (int i = 0; i < ROUNDY_ANIMATED_GLYPH_COUNT; ++i) {
    if (!state->glyph_active[i]) {
      continue;
    }
    const float elapsed = state->anim_time - state->glyph_start[i];
    if (elapsed >= ROUNDY_GLYPH_DURATION) {
      state->glyph_active[i] = false;
      continue;
    }
    still_active = true;
  }

  if (still_active) {
    state->anim_timer = app_timer_register(FRAME_MS, prv_diag_anim_timer, layer);
  } else {
    state->anim_timer = NULL;
  }

  layer_mark_dirty(layer);
}

void roundy_digit_layer_start_diag_flip(RoundyDigitLayer *rdl) {
  if (!rdl || !rdl->layer) {
    return;
  }
  RoundyDigitLayerState *state = layer_get_data(rdl->layer);
  if (!state) {
    return;
  }

  if (state->anim_timer) {
    app_timer_cancel(state->anim_timer);
    state->anim_timer = NULL;
  }

  state->diag_mode_active = true;
  state->diag_time = 0.0f;
  state->anim_time = 0.0f;
  for (int i = 0; i < ROUNDY_ANIMATED_GLYPH_COUNT; ++i) {
    state->glyph_active[i] = false;
  }

  state->anim_timer =
      app_timer_register(DIAG_START_DELAY_MS, prv_diag_anim_timer, rdl->layer);
  layer_mark_dirty(rdl->layer);
}

void roundy_digit_layer_set_time(RoundyDigitLayer *layer, const struct tm *time_info) {
  RoundyDigitLayerState *state = prv_get_state(layer);
  if (!state || !time_info) {
    return;
  }

  static const int glyph_index_map[ROUNDY_DIGIT_COUNT] = {0, 1, 3, 4};
  bool glyph_mask[ROUNDY_ANIMATED_GLYPH_COUNT] = {false};

  int16_t old_digits[ROUNDY_DIGIT_COUNT];
  for (int i = 0; i < ROUNDY_DIGIT_COUNT; ++i) {
    old_digits[i] = state->digits[i];
  }

  const bool use_24h = clock_is_24h_style();
  int hour = time_info->tm_hour;
  if (!use_24h) {
    hour %= 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  int16_t new_digits[ROUNDY_DIGIT_COUNT] = {
    hour / 10,
    hour % 10,
    time_info->tm_min / 10,
    time_info->tm_min % 10,
  };

  if (!use_24h && hour < 10) {
    new_digits[0] = -1;
  }

  bool changed = (state->use_24h_time != use_24h);
  bool all_old_blank = true;
  for (int i = 0; i < ROUNDY_DIGIT_COUNT; ++i) {
    if (state->digits[i] != new_digits[i]) {
      changed = true;
      glyph_mask[glyph_index_map[i]] = true;
    }
    if (old_digits[i] >= 0) {
      all_old_blank = false;
    }
    state->prev_digits[i] = old_digits[i];
    state->digits[i] = new_digits[i];
  }

  if (state->use_24h_time != use_24h) {
    state->use_24h_time = use_24h;
  }

  glyph_mask[2] = glyph_mask[1] || glyph_mask[3];
  bool any_glyph = false;
  for (int i = 0; i < ROUNDY_ANIMATED_GLYPH_COUNT; ++i) {
    if (glyph_mask[i]) {
      any_glyph = true;
      break;
    }
  }

  if (changed && layer->layer) {
    if (any_glyph && !all_old_blank && !state->diag_mode_active) {
      prv_start_glyph_animation(layer, glyph_mask, DIAG_FRAME_MS);
    }
    layer_mark_dirty(layer->layer);
  }
}

void roundy_digit_layer_refresh_time(RoundyDigitLayer *layer) {
  time_t now = time(NULL);
  struct tm *time_info = localtime(&now);
  if (!time_info) {
    return;
  }
  roundy_digit_layer_set_time(layer, time_info);
}

void roundy_digit_layer_force_redraw(RoundyDigitLayer *layer) {
  if (layer && layer->layer) {
    layer_mark_dirty(layer->layer);
  }
}
