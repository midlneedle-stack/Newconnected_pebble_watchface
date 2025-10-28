#include <pebble.h>

#define CELL_SIZE   4
#define GRID_COLS   36
#define GRID_ROWS   42

#define COLOR_BG       GColorWhite
#define COLOR_STROKE   GColorFromRGB(0xAA,0xAA,0xAA)
#define COLOR_ACTIVE   GColorBlack
#define COLOR_INACTIVE GColorWhite

static Window *s_main_window;
static Layer  *s_draw_layer;
static char s_time_str[6]; // "HHMM"

// --------- Шаблоны цифр (10x6). Линии везде 2-ячейковой толщины ---------
// 1 имеет фактическую ширину 4, но матрица 6 — лишние столбцы нули.
static const uint8_t DIGITS[10][10][6] = {
/* 0 */ {
  {1,1,1,1,1,1}, // top 2 строки
  {1,1,1,1,1,1},
  {1,1,0,0,1,1}, // боковые 2-ячейковые стойки
  {1,1,0,0,1,1},
  {1,1,0,0,1,1},
  {1,1,0,0,1,1},
  {1,1,0,0,1,1},
  {1,1,0,0,1,1},
  {1,1,1,1,1,1}, // bottom 2 строки
  {1,1,1,1,1,1}
},
/* 1 */ {
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0}
},
/* 2 */ {
  {1,1,1,1,1,1}, // top 2
  {1,1,1,1,1,1},
  {0,0,0,0,1,1}, // верхняя правая стойка
  {0,0,0,0,1,1},
  {1,1,1,1,1,1}, // середина 2
  {1,1,1,1,1,1},
  {1,1,0,0,0,0}, // нижняя левая стойка
  {1,1,0,0,0,0},
  {1,1,1,1,1,1}, // bottom 2
  {1,1,1,1,1,1}
},
/* 3 */ {
  {1,1,1,1,1,1},
  {1,1,1,1,1,1},
  {0,0,0,0,1,1}, // правая стойка (верх)
  {0,0,0,0,1,1},
  {1,1,1,1,1,1}, // середина
  {1,1,1,1,1,1},
  {0,0,0,0,1,1}, // правая стойка (низ)
  {0,0,0,0,1,1},
  {1,1,1,1,1,1}, // низ
  {1,1,1,1,1,1}
},
/* 4 */ {
  {1,1,0,0,1,1}, // две стойки
  {1,1,0,0,1,1},
  {1,1,0,0,1,1},
  {1,1,1,1,1,1}, // середина 2 строки
  {1,1,1,1,1,1},
  {0,0,0,0,1,1}, // правая стойка вниз
  {0,0,0,0,1,1},
  {0,0,0,0,1,1},
  {0,0,0,0,1,1},
  {0,0,0,0,1,1}
},
/* 5 */ {
  {1,1,1,1,1,1}, // top
  {1,1,1,1,1,1},
  {1,1,0,0,0,0}, // левая стойка (верх)
  {1,1,0,0,0,0},
  {1,1,1,1,1,1}, // середина
  {1,1,1,1,1,1},
  {0,0,0,0,1,1}, // правая стойка (низ)
  {0,0,0,0,1,1},
  {1,1,1,1,1,1}, // низ
  {1,1,1,1,1,1}
},
/* 6 */ {
  {1,1,1,1,1,1}, // top
  {1,1,1,1,1,1},
  {1,1,0,0,0,0}, // левая стойка (долго)
  {1,1,0,0,0,0},
  {1,1,1,1,1,1}, // середина
  {1,1,1,1,1,1},
  {1,1,0,0,1,1}, // обе стойки (низ)
  {1,1,0,0,1,1},
  {1,1,1,1,1,1}, // низ
  {1,1,1,1,1,1}
},
/* 7 */ {
  {1,1,1,1,1,1}, // top
  {1,1,1,1,1,1},
  {0,0,0,0,1,1}, // правая стойка вниз
  {0,0,0,0,1,1},
  {0,0,0,0,1,1},
  {0,0,0,0,1,1},
  {0,0,0,0,1,1},
  {0,0,0,0,1,1},
  {0,0,0,0,1,1},
  {0,0,0,0,1,1}
},
/* 8 */ {
  {1,1,1,1,1,1}, // top
  {1,1,1,1,1,1},
  {1,1,0,0,1,1}, // обе стойки
  {1,1,0,0,1,1},
  {1,1,1,1,1,1}, // середина
  {1,1,1,1,1,1},
  {1,1,0,0,1,1}, // обе стойки
  {1,1,0,0,1,1},
  {1,1,1,1,1,1}, // низ
  {1,1,1,1,1,1}
},
/* 9 */ {
  {1,1,1,1,1,1}, // top
  {1,1,1,1,1,1},
  {1,1,0,0,1,1}, // обе стойки (верх)
  {1,1,0,0,1,1},
  {1,1,1,1,1,1}, // середина
  {1,1,1,1,1,1},
  {0,0,0,0,1,1}, // правая стойка вниз
  {0,0,0,0,1,1},
  {1,1,1,1,1,1}, // низ
  {1,1,1,1,1,1}
}
};

// Двоеточие: две точки 2x2, между ними 2 ячейки
static const uint8_t COLON_PATTERN[10][2] = {
  {0,0},{0,0},{1,1},{1,1},{0,0},{0,0},{1,1},{1,1},{0,0},{0,0}
};

// ---- рисуем ячейку: белая/чёрная заливка + серый внутренний 1px-обвод ----
static void draw_cell(GContext *ctx, int x, int y, GColor fill) {
  GRect rect = GRect(x, y, CELL_SIZE, CELL_SIZE);
  graphics_context_set_fill_color(ctx, fill);
  graphics_fill_rect(ctx, rect, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, COLOR_STROKE);
  graphics_draw_rect(ctx, rect);
}

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(s_time_str, sizeof(s_time_str), "%H%M", t);
  if (s_draw_layer) layer_mark_dirty(s_draw_layer);
}

static void draw_layer_update_proc(Layer *layer, GContext *ctx) {
  // фон
  graphics_context_set_fill_color(ctx, COLOR_BG);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  // сетка (белые ячейки с серым stroke)
  for (int y = 0; y < GRID_ROWS; y++)
    for (int x = 0; x < GRID_COLS; x++)
      draw_cell(ctx, x * CELL_SIZE, y * CELL_SIZE, COLOR_INACTIVE);

  // параметры набора
  const int spacing = 1;   // 1 ячейка между всеми символами
  const int digit_h = 10;
  const int colon_w = 2;

  // ширины символов (1 — 4, остальные — 6)
  int w0 = (s_time_str[0]=='1') ? 4 : 6;
  int w1 = (s_time_str[1]=='1') ? 4 : 6;
  int w2 = (s_time_str[2]=='1') ? 4 : 6; // не используется (двоеточие)
  int w3 = (s_time_str[3]=='1') ? 4 : 6;

  int total_cols = w0 + spacing + w1 + spacing + colon_w + spacing + w3 + spacing + 6; // последняя цифра = 6 или 4?
  // Точнее: вычислим динамически:
  int w2d = 6; // ширина последней цифры (второй M)
  // Пересчёт аккуратный:
  total_cols = w0 + spacing + w1 + spacing + colon_w + spacing + ((s_time_str[2]=='1')?4:6) + spacing + ((s_time_str[3]=='1')?4:6);
  // Но у нас формат HHMM: s_time_str[2] = 'M' десятки, s_time_str[3] = 'M' единицы

  int widths[4] = {
    (s_time_str[0]=='1')?4:6,
    (s_time_str[1]=='1')?4:6,
    (s_time_str[2]=='1')?4:6,
    (s_time_str[3]=='1')?4:6
  };

  total_cols = widths[0] + spacing + widths[1] + spacing + colon_w + spacing + widths[2] + spacing + widths[3];

  int start_col = (GRID_COLS - total_cols) / 2;
  int start_row = (GRID_ROWS - digit_h) / 2;

  // рисуем HH:MM
  int col = start_col;
  char seq[5] = { s_time_str[0], s_time_str[1], ':', s_time_str[2], s_time_str[3] };

  for (int i = 0; i < 5; i++) {
    if (seq[i] == ':') {
      // двоеточие — всегда видно (без мигания)
      for (int r = 0; r < 10; r++)
        for (int c = 0; c < 2; c++)
          if (COLON_PATTERN[r][c])
            draw_cell(ctx, (col + c) * CELL_SIZE, (start_row + r) * CELL_SIZE, COLOR_ACTIVE);
      col += colon_w + spacing;
    } else {
      int d = seq[i] - '0';
      int w = (d == 1) ? 4 : 6;
      for (int r = 0; r < 10; r++)
        for (int c = 0; c < w; c++)
          if (DIGITS[d][r][c])
            draw_cell(ctx, (col + c) * CELL_SIZE, (start_row + r) * CELL_SIZE, COLOR_ACTIVE);
      col += w + spacing;
    }
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) update_time();
}

static void main_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_draw_layer = layer_create(bounds);
  layer_set_update_proc(s_draw_layer, draw_layer_update_proc);
  layer_add_child(root, s_draw_layer);
  update_time();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_draw_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, COLOR_BG);
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
