#include <pebble.h>

#define CELL_SIZE 4
#define GRID_COLOR     GColorFromHEX(0x555555)
#define ACTIVE_COLOR   GColorFromHEX(0xFFFFFF)
#define INACTIVE_COLOR GColorFromHEX(0x000000)

#define DIGIT_WIDTH 6
#define DIGIT_HEIGHT 12
#define DIGIT_SPACING 1
#define COLON_WIDTH 2

static Window *s_main_window;
static Layer *s_canvas_layer;
static bool s_colon_visible = true;

// ========================================================
// Полная матрица 12x6 для каждой цифры
// ========================================================
static const bool DIGITS[10][12][6] = {
  // 0
  {
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1}
  },
  // 1
  {
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,1,1,1,1},
    {0,0,1,1,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1}
  },
  // 2
  {
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,1,0,0,0,0},
    {1,1,0,0,0,0},
    {1,1,0,0,0,0},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1}
  },
  // 3
  {
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1}
  },
  // 4
  {
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1}
  },
  // 5
  {
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,1,0,0,0,0},
    {1,1,0,0,0,0},
    {1,1,0,0,0,0},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1}
  },
  // 6
  {
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,1,0,0,0,0},
    {1,1,0,0,0,0},
    {1,1,0,0,0,0},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1}
  },
  // 7
  {
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1}
  },
  // 8
  {
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1}
  },
  // 9
  {
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {0,0,0,0,1,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1}
  }
};

// ========================================================
// Узкое двоеточие (12x2)
// ========================================================
static const bool COLON[12][2] = {
  {0,0},
  {0,0},
  {0,0},
  {1,1},
  {1,1},
  {0,0},
  {0,0},
  {1,1},
  {1,1},
  {0,0},
  {0,0},
  {0,0}
};

// ========================================================
static void draw_cell(GContext *ctx, int x, int y, bool active) {
  graphics_context_set_stroke_color(ctx, GRID_COLOR);
  graphics_draw_rect(ctx, GRect(x, y, CELL_SIZE, CELL_SIZE));
  graphics_context_set_fill_color(ctx, active ? ACTIVE_COLOR : INACTIVE_COLOR);
  graphics_fill_rect(ctx, GRect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2), 0, GCornerNone);
}

static void draw_full_grid(GContext *ctx, GRect b) {
  for (int y = 0; y < b.size.h; y += CELL_SIZE)
    for (int x = 0; x < b.size.w; x += CELL_SIZE)
      draw_cell(ctx, x, y, false);
}

static void draw_digit(GContext *ctx, GPoint o, int d) {
  for (int r = 0; r < DIGIT_HEIGHT; r++)
    for (int c = 0; c < DIGIT_WIDTH; c++)
      draw_cell(ctx, o.x + c * CELL_SIZE, o.y + r * CELL_SIZE, DIGITS[d][r][c]);
}

static void draw_colon(GContext *ctx, GPoint o, bool visible) {
  if (!visible) return;
  for (int r = 0; r < DIGIT_HEIGHT; r++)
    for (int c = 0; c < COLON_WIDTH; c++)
      draw_cell(ctx, o.x + c * CELL_SIZE, o.y + r * CELL_SIZE, COLON[r][c]);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  draw_full_grid(ctx, b);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int h1 = t->tm_hour / 10, h2 = t->tm_hour % 10;
  int m1 = t->tm_min / 10, m2 = t->tm_min % 10;

  int tw = (DIGIT_WIDTH * 4 + 3 * DIGIT_SPACING + COLON_WIDTH) * CELL_SIZE;
  int th = DIGIT_HEIGHT * CELL_SIZE;
  int sx = ((b.size.w - tw) / 2 / CELL_SIZE) * CELL_SIZE;
  int sy = ((b.size.h - th) / 2 / CELL_SIZE) * CELL_SIZE;

  GPoint p = GPoint(sx, sy);
  draw_digit(ctx, p, h1);
  p.x += (DIGIT_WIDTH + DIGIT_SPACING) * CELL_SIZE;
  draw_digit(ctx, p, h2);
  p.x += (DIGIT_WIDTH + DIGIT_SPACING) * CELL_SIZE;
  draw_colon(ctx, p, s_colon_visible);
  p.x += (COLON_WIDTH + DIGIT_SPACING) * CELL_SIZE;
  draw_digit(ctx, p, m1);
  p.x += (DIGIT_WIDTH + DIGIT_SPACING) * CELL_SIZE;
  draw_digit(ctx, p, m2);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_colon_visible = !s_colon_visible;
  layer_mark_dirty(s_canvas_layer);
}

static void main_window_load(Window *window) {
  s_canvas_layer = layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, INACTIVE_COLOR);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
