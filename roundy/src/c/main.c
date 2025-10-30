#include <pebble.h>

// ================== Константы ==================
#define CELL_SIZE 4
#define GRID_COLOR     GColorFromHEX(0x555555)
#define ACTIVE_COLOR   GColorFromHEX(0xFFFFFF)
#define INACTIVE_COLOR GColorFromHEX(0x000000)

#define DIGIT_WIDTH 8
#define DIGIT_HEIGHT 18
#define DIGIT_SPACING 0
#define COLON_WIDTH 4
#define GRID_COLS (DIGIT_WIDTH*4 + COLON_WIDTH + DIGIT_SPACING*4)

#define DIGIT_ANIM_STEPS 16          // total frames per entrance animation
#define HALO_PEAK_STEP ((DIGIT_ANIM_STEPS - 1) / 2)
#define HALO_MAX_LEVEL 2

#define DIGIT_THRESHOLD_LOW 30       // % progress when digits reach level 1
#define DIGIT_THRESHOLD_MID 60       // % progress for level 2
#define DIGIT_THRESHOLD_HIGH 85      // % progress for full brightness
#define DIGIT_FILL_BASE 25           // base probability for brightening (%)
#define DIGIT_FILL_STEP 20           // additional probability per missing level
#define DIGIT_FILL_MAX 95            // clamp for probability
#define DIGIT_DELAY_STEPS 5             // max random frame delay per cell

#define MAX_BRIGHTNESS 3
#define ANIM_SPEED_MS 100            // ms between animation frames (tune duration)

static Window *s_main_window;
static Layer *s_grid_layer;
static Layer *s_canvas_layer;
static AppTimer *s_anim_timer;
static bool s_animating = true;

#define BRIGHTNESS_LEVELS 4
static const uint32_t BRIGHTNESS_HEX[BRIGHTNESS_LEVELS] = {
  0x000000,  // OFF
  0x555555,  // LOW
  0xAAAAAA,  // MID
  0xFFFFFF   // FULL
};

// ================== ASCII цифры (18×8) ==================
static const bool DIGITS[10][DIGIT_HEIGHT][DIGIT_WIDTH] = {

  // 0
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 1
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,1,1,1,1,0},
    {0,0,0,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 2
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 3
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 4
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 5
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 6
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 7
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 8
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  },

  // 9
  {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
  }
};

// Двоеточие (18×4)
static const bool COLON[DIGIT_HEIGHT][COLON_WIDTH] = {
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,1,1,0},
  {0,1,1,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,1,1,0},
  {0,1,1,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0}
};

// ================== Состояния ==================
static uint8_t pixel_state[DIGIT_HEIGHT][GRID_COLS];
static uint8_t halo_state[DIGIT_HEIGHT][GRID_COLS];
static uint8_t digit_delay[DIGIT_HEIGHT][GRID_COLS];
static bool active_mask[DIGIT_HEIGHT][GRID_COLS];
static uint8_t distance_map[DIGIT_HEIGHT][GRID_COLS];
static int prev_h1=-1, prev_h2=-1, prev_m1=-1, prev_m2=-1;
static uint8_t anim_frame=0;

// ================== Утилиты ==================
static void reset_states(void) {
  for(int y=0;y<DIGIT_HEIGHT;y++)
    for(int x=0;x<GRID_COLS;x++){
      pixel_state[y][x]=0;
      halo_state[y][x]=0;
      digit_delay[y][x]=0;
      active_mask[y][x]=false;
      distance_map[y][x]=255;
    }
}

static void stamp_digit_mask(int digit,int gx_offset){
  for(int r=0;r<DIGIT_HEIGHT;r++)
    for(int c=0;c<DIGIT_WIDTH;c++)
      if(DIGITS[digit][r][c])
        active_mask[r][gx_offset+c]=true;
}

static void stamp_colon_mask(int gx_offset){
  for(int r=0;r<DIGIT_HEIGHT;r++)
    for(int c=0;c<COLON_WIDTH;c++)
      if(COLON[r][c])
        active_mask[r][gx_offset+c]=true;
}


static void assign_digit_delays(void){
  for(int y=0;y<DIGIT_HEIGHT;y++)
    for(int x=0;x<GRID_COLS;x++)
      digit_delay[y][x] = active_mask[y][x] ? (uint8_t)(rand() % (DIGIT_DELAY_STEPS + 1)) : 0;
}

static void compute_digit_mask(int h1,int h2,int m1,int m2){
  for(int y=0;y<DIGIT_HEIGHT;y++)
    for(int x=0;x<GRID_COLS;x++)
      active_mask[y][x]=false;

  int gx=0;
  stamp_digit_mask(h1,gx);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  stamp_digit_mask(h2,gx);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  stamp_colon_mask(gx);
  gx+=COLON_WIDTH+DIGIT_SPACING;
  stamp_digit_mask(m1,gx);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  stamp_digit_mask(m2,gx);

  bool has_active=false;
  for(int y=0;y<DIGIT_HEIGHT;y++)
    for(int x=0;x<GRID_COLS;x++){
      if(active_mask[y][x]){
        distance_map[y][x]=0;
        has_active=true;
      }else{
        distance_map[y][x]=255;
      }
    }

  if(!has_active)
    return;

  for(int y=0;y<DIGIT_HEIGHT;y++)
    for(int x=0;x<GRID_COLS;x++){
      if(active_mask[y][x]) continue;
      uint8_t best=255;
      for(int yy=0;yy<DIGIT_HEIGHT;yy++)
        for(int xx=0;xx<GRID_COLS;xx++)
          if(active_mask[yy][xx]){
            uint8_t dy = y>yy ? (uint8_t)(y-yy) : (uint8_t)(yy-y);
            uint8_t dx = x>xx ? (uint8_t)(x-xx) : (uint8_t)(xx-x);
            uint8_t dist = dy + dx;
            if(dist<best)
              best=dist;
          }
      distance_map[y][x]=best;
    }
}

// ================== Отрисовка ==================
static void draw_cell(GContext *ctx,int x,int y,bool active,int gx,int gy){
  if(!active) return;
  uint32_t color_hex = s_animating && pixel_state[gy][gx] < BRIGHTNESS_LEVELS
      ? BRIGHTNESS_HEX[pixel_state[gy][gx]] : 0xFFFFFF;
  graphics_context_set_fill_color(ctx, GColorFromHEX(color_hex));
  graphics_fill_rect(ctx, GRect(x+1,y+1,CELL_SIZE-2,CELL_SIZE-2),0,GCornerNone);
}

static void draw_digit(GContext *ctx,GPoint o,int d,int gx_offset){
  for(int r=0;r<DIGIT_HEIGHT;r++)
    for(int c=0;c<DIGIT_WIDTH;c++){
      bool active = DIGITS[d][r][c];
      draw_cell(ctx, o.x+c*CELL_SIZE, o.y+r*CELL_SIZE, active, gx_offset+c, r);
    }
}

static void draw_colon(GContext *ctx,GPoint o,int gx_offset){
  for(int r=0;r<DIGIT_HEIGHT;r++)
    for(int c=0;c<COLON_WIDTH;c++)
      draw_cell(ctx, o.x+c*CELL_SIZE, o.y+r*CELL_SIZE, COLON[r][c], gx_offset+c, r);
}

static uint8_t halo_intensity_for_step(uint8_t step){
  if(DIGIT_ANIM_STEPS <= 1) return 0;
  uint8_t last_step = DIGIT_ANIM_STEPS - 1;
  if(step >= last_step) return 0;
  uint8_t peak = HALO_PEAK_STEP;
  if(peak == 0) peak = 1;
  if(step <= peak)
    return (uint8_t)((step * 100) / peak);
  uint8_t descending_steps = last_step - peak;
  if(descending_steps == 0) return 0;
  uint8_t remaining = last_step - step;
  return (uint8_t)((remaining * 100) / descending_steps);
}

static bool should_spawn_halo(uint8_t distance,uint8_t intensity){
  if(intensity==0 || distance==0 || distance==255)
    return false;
  uint8_t base_chance = 0;
  if(distance<=1)
    base_chance = 40;
  else if(distance==2)
    base_chance = 18;
  else if(distance<=4)
    base_chance = 6;
  if(base_chance==0)
    return false;
  uint8_t chance = (uint8_t)((base_chance * intensity) / 100);
  if(chance==0)
    return false;
  return (rand()%100) < chance;
}

// Статичный слой — сетка
static void grid_update_proc(Layer *layer,GContext *ctx){
  GRect b = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, GRID_COLOR);
  for(int y=0;y<b.size.h;y+=CELL_SIZE)
    for(int x=0;x<b.size.w;x+=CELL_SIZE)
      graphics_draw_rect(ctx, GRect(x,y,CELL_SIZE,CELL_SIZE));
}

static void draw_halo_layer(GContext *ctx,GPoint origin){
  for(int r=0;r<DIGIT_HEIGHT;r++)
    for(int c=0;c<GRID_COLS;c++){
      if(active_mask[r][c]) continue;
      uint8_t level = halo_state[r][c];
      if(level>0){
        uint8_t idx = level < BRIGHTNESS_LEVELS ? level : (BRIGHTNESS_LEVELS - 1);
        graphics_context_set_fill_color(ctx, GColorFromHEX(BRIGHTNESS_HEX[idx]));
        graphics_fill_rect(ctx, GRect(origin.x+c*CELL_SIZE+1, origin.y+r*CELL_SIZE+1, CELL_SIZE-2, CELL_SIZE-2), 0, GCornerNone);
      }
    }
}

// Основной слой — цифры
static void canvas_update_proc(Layer *layer,GContext *ctx){
  GRect b=layer_get_bounds(layer);
  time_t now=time(NULL);
  struct tm *t=localtime(&now);
  int h1=t->tm_hour/10,h2=t->tm_hour%10,m1=t->tm_min/10,m2=t->tm_min%10;

  compute_digit_mask(h1,h2,m1,m2);

  int tw=GRID_COLS*CELL_SIZE;
  int th=DIGIT_HEIGHT*CELL_SIZE;
  int sx=((b.size.w-tw)/2/CELL_SIZE)*CELL_SIZE;
  int sy=((b.size.h-th)/2/CELL_SIZE)*CELL_SIZE;

  GPoint origin=GPoint(sx,sy);
  draw_halo_layer(ctx,origin);

  int gx=0;
  GPoint p=origin;
  draw_digit(ctx,p,h1,gx);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  p.x+=(DIGIT_WIDTH+DIGIT_SPACING)*CELL_SIZE;
  draw_digit(ctx,p,h2,gx);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  p.x+=(DIGIT_WIDTH+DIGIT_SPACING)*CELL_SIZE;
  draw_colon(ctx,p,gx);
  gx+=COLON_WIDTH+DIGIT_SPACING;
  p.x+=(COLON_WIDTH+DIGIT_SPACING)*CELL_SIZE;
  draw_digit(ctx,p,m1,gx);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  p.x+=(DIGIT_WIDTH+DIGIT_SPACING)*CELL_SIZE;
  draw_digit(ctx,p,m2,gx);
}

// ================== Анимация ==================
static uint8_t brightness_from_percent(uint8_t percent){
  if(percent >= DIGIT_THRESHOLD_HIGH) return MAX_BRIGHTNESS;
  if(percent >= DIGIT_THRESHOLD_MID) return MAX_BRIGHTNESS > 1 ? 2 : MAX_BRIGHTNESS;
  if(percent >= DIGIT_THRESHOLD_LOW) return MAX_BRIGHTNESS > 0 ? 1 : 0;
  return 0;
}

static uint8_t target_level_for_step(uint8_t step){
  if(DIGIT_ANIM_STEPS == 0) return MAX_BRIGHTNESS;
  if(step >= DIGIT_ANIM_STEPS-1) return MAX_BRIGHTNESS;
  uint8_t percent = (uint8_t)(((uint32_t)(step+1) * 100) / DIGIT_ANIM_STEPS);
  return brightness_from_percent(percent);
}

static void animation_step(void *context){
  if(anim_frame >= DIGIT_ANIM_STEPS){
    for(int y=0;y<DIGIT_HEIGHT;y++)
      for(int x=0;x<GRID_COLS;x++){
        if(active_mask[y][x])
          pixel_state[y][x]=MAX_BRIGHTNESS;
        else
          halo_state[y][x]=0;
      }
    s_animating=false;
    s_anim_timer=NULL;
    layer_mark_dirty(s_canvas_layer);
    return;
  }

  uint8_t target_level = target_level_for_step(anim_frame);
  bool digits_pending=false;

  for(int y=0;y<DIGIT_HEIGHT;y++)
    for(int x=0;x<GRID_COLS;x++){
      if(!active_mask[y][x]) continue;
      if(anim_frame >= DIGIT_ANIM_STEPS-1){
        pixel_state[y][x]=MAX_BRIGHTNESS;
        continue;
      }

      uint8_t delay = digit_delay[y][x];
      if(anim_frame < delay){
        if(pixel_state[y][x] < MAX_BRIGHTNESS)
          digits_pending=true;
        continue;
      }

      uint8_t current = pixel_state[y][x];
      if(current >= MAX_BRIGHTNESS) continue;

      uint8_t effective_total = (DIGIT_ANIM_STEPS > delay) ? (DIGIT_ANIM_STEPS - delay) : 1;
      if(effective_total == 0) effective_total = 1;
      uint8_t effective_step = anim_frame - delay;
      if(effective_step >= effective_total)
        effective_step = effective_total - 1;
      uint8_t percent = (uint8_t)(((uint32_t)(effective_step+1) * 100) / effective_total);
      uint8_t target = brightness_from_percent(percent);
      if(target > MAX_BRIGHTNESS) target = MAX_BRIGHTNESS;
      if(anim_frame >= DIGIT_ANIM_STEPS-2)
        target = MAX_BRIGHTNESS;

      if(target <= current){
        if(pixel_state[y][x] < MAX_BRIGHTNESS)
          digits_pending=true;
        continue;
      }

      uint8_t deficit = target - current;
      uint8_t chance = DIGIT_FILL_BASE + deficit * DIGIT_FILL_STEP;
      if(chance > DIGIT_FILL_MAX) chance = DIGIT_FILL_MAX;

      if((rand()%100) < chance){
        pixel_state[y][x] = current + 1;
        if(pixel_state[y][x] > MAX_BRIGHTNESS)
          pixel_state[y][x] = MAX_BRIGHTNESS;
      }

      if(pixel_state[y][x] < MAX_BRIGHTNESS)
        digits_pending=true;
    }

  uint8_t halo_intensity = halo_intensity_for_step(anim_frame);
  bool halo_pending=false;
  for(int y=0;y<DIGIT_HEIGHT;y++)
    for(int x=0;x<GRID_COLS;x++){
      if(active_mask[y][x]) continue;
      uint8_t halo=halo_state[y][x];
      if(halo>0){
        halo_state[y][x] = (halo>=HALO_MAX_LEVEL) ? 0 : (halo+1);
        if(halo_state[y][x]>0)
          halo_pending=true;
      }else if(halo_intensity>0 && should_spawn_halo(distance_map[y][x], halo_intensity)){
        halo_state[y][x]=1;
        halo_pending=true;
      }
    }

  anim_frame++;
  bool continue_anim = digits_pending || halo_pending || (anim_frame < DIGIT_ANIM_STEPS);

  layer_mark_dirty(s_canvas_layer);
  if(continue_anim){
    s_anim_timer=app_timer_register(ANIM_SPEED_MS,animation_step,NULL);
  }else{
    s_anim_timer=NULL;
    s_animating=false;
  }
}

static void trigger_animation(void){
  time_t now=time(NULL);
  struct tm *t=localtime(&now);
  int h1=t->tm_hour/10,h2=t->tm_hour%10,m1=t->tm_min/10,m2=t->tm_min%10;

  if(s_anim_timer){
    app_timer_cancel(s_anim_timer);
    s_anim_timer=NULL;
  }

  reset_states();
  compute_digit_mask(h1,h2,m1,m2);
  assign_digit_delays();
  prev_h1=h1;
  prev_h2=h2;
  prev_m1=m1;
  prev_m2=m2;
  anim_frame=0;
  s_animating=true;
  animation_step(NULL);
}

// ================== Службы ==================
static void tick_handler(struct tm *tick_time,TimeUnits units_changed){
  int h1=tick_time->tm_hour/10,h2=tick_time->tm_hour%10;
  int m1=tick_time->tm_min/10,m2=tick_time->tm_min%10;

  if(s_animating){
    prev_h1=h1;
    prev_h2=h2;
    prev_m1=m1;
    prev_m2=m2;
    compute_digit_mask(h1,h2,m1,m2);
    return;
  }

  if(s_anim_timer){
    app_timer_cancel(s_anim_timer);
    s_anim_timer=NULL;
  }

  prev_h1=h1;
  prev_h2=h2;
  prev_m1=m1;
  prev_m2=m2;

  compute_digit_mask(h1,h2,m1,m2);

  for(int y=0;y<DIGIT_HEIGHT;y++)
    for(int x=0;x<GRID_COLS;x++){
      pixel_state[y][x]=MAX_BRIGHTNESS;
      halo_state[y][x]=0;
    }

  s_animating=false;
  anim_frame = DIGIT_ANIM_STEPS;
  layer_mark_dirty(s_canvas_layer);
}

// ================== Окна ==================
static void main_window_load(Window *w){
  Layer *root = window_get_root_layer(w);
  GRect bounds = layer_get_bounds(root);

  s_grid_layer = layer_create(bounds);
  layer_set_update_proc(s_grid_layer, grid_update_proc);
  layer_add_child(root, s_grid_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(root, s_canvas_layer);

  trigger_animation();
}

static void main_window_unload(Window *w){
  layer_destroy(s_canvas_layer);
  layer_destroy(s_grid_layer);
}

static void init(void){
  s_main_window = window_create();
  window_set_background_color(s_main_window, INACTIVE_COLOR);
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void){
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}
