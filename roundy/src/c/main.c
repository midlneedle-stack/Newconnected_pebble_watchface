#include <pebble.h>

#define CELL_SIZE 4
#define GRID_COLOR     GColorFromHEX(0x555555)
#define ACTIVE_COLOR   GColorFromHEX(0xFFFFFF)
#define INACTIVE_COLOR GColorFromHEX(0x000000)

#define DIGIT_WIDTH 6
#define DIGIT_HEIGHT 12
#define DIGIT_SPACING 1
#define COLON_WIDTH 2

#define MAX_BRIGHTNESS 3
#define ANIM_SPEED_MS 25
#define ANIM_RANDOMNESS 5
#define ARTIFACT_RADIUS 3
#define ARTIFACT_CHANCE 10

static Window *s_main_window;
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

// ========================================================
// ASCII-матрицы цифр (12×6)
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

// двоеточие
static const bool COLON[12][2] = {
  {0,0},{0,0},{0,0},{1,1},{1,1},{0,0},
  {0,0},{1,1},{1,1},{0,0},{0,0},{0,0}
};

static uint8_t pixel_state[12][30];
static uint8_t artifact_state[12][30];
static bool digit_animating[4]; // флаги для 4 цифр

static int prev_h1=-1, prev_h2=-1, prev_m1=-1, prev_m2=-1;

// ========================================================
static void reset_states(void) {
  for (int y=0;y<12;y++)
    for (int x=0;x<30;x++){
      pixel_state[y][x]=0;
      artifact_state[y][x]=0;
    }
  for (int i=0;i<4;i++) digit_animating[i]=false;
}

static void trigger_digit_animation(int index){
  digit_animating[index]=true;
  s_animating=true;
}

// ========================================================
static void animation_step(void *context) {
  bool all_done=true;

  for(int y=0;y<12;y++){
    for(int x=0;x<30;x++){
      if(pixel_state[y][x]<MAX_BRIGHTNESS){
        if(rand()%ANIM_RANDOMNESS==0)
          pixel_state[y][x]++;
        all_done=false;
      }

      // артефакты вокруг
      if(rand()%100<ARTIFACT_CHANCE && artifact_state[y][x]==0)
        artifact_state[y][x]=(rand()%2)+1;
      if(artifact_state[y][x]>0 && rand()%3==0)
        artifact_state[y][x]--;
    }
  }

  layer_mark_dirty(s_canvas_layer);
  if(!all_done)
    s_anim_timer=app_timer_register(ANIM_SPEED_MS,animation_step,NULL);
  else{
    s_animating=false;
    for(int y=0;y<12;y++)
      for(int x=0;x<30;x++)
        artifact_state[y][x]=0;
  }
}

// ========================================================
static void draw_cell(GContext *ctx,int x,int y,bool active,int gx,int gy){
  graphics_context_set_stroke_color(ctx,GRID_COLOR);
  graphics_draw_rect(ctx,GRect(x,y,CELL_SIZE,CELL_SIZE));
  uint32_t color_hex=0x000000;

  if(active){
    color_hex = s_animating ? BRIGHTNESS_HEX[pixel_state[gy][gx]] : 0xFFFFFF;
  } else if(artifact_state[gy][gx]>0){
    color_hex = BRIGHTNESS_HEX[artifact_state[gy][gx]];
  }

  graphics_context_set_fill_color(ctx,GColorFromHEX(color_hex));
  graphics_fill_rect(ctx,GRect(x+1,y+1,CELL_SIZE-2,CELL_SIZE-2),0,GCornerNone);
}

// ========================================================
static void draw_digit(GContext *ctx,GPoint o,int d,int gx_offset,bool animate){
  for(int r=0;r<DIGIT_HEIGHT;r++)
    for(int c=0;c<DIGIT_WIDTH;c++){
      bool active = DIGITS[d][r][c];
      if(animate) draw_cell(ctx,o.x+c*CELL_SIZE,o.y+r*CELL_SIZE,active,gx_offset+c,r);
      else {
        graphics_context_set_stroke_color(ctx,GRID_COLOR);
        graphics_draw_rect(ctx,GRect(o.x+c*CELL_SIZE,o.y+r*CELL_SIZE,CELL_SIZE,CELL_SIZE));
        if(active){
          graphics_context_set_fill_color(ctx,ACTIVE_COLOR);
          graphics_fill_rect(ctx,GRect(o.x+c*CELL_SIZE+1,o.y+r*CELL_SIZE+1,CELL_SIZE-2,CELL_SIZE-2),0,GCornerNone);
        }
      }
    }
}

static void draw_colon(GContext *ctx,GPoint o,int gx_offset){
  for(int r=0;r<DIGIT_HEIGHT;r++)
    for(int c=0;c<COLON_WIDTH;c++)
      draw_cell(ctx,o.x+c*CELL_SIZE,o.y+r*CELL_SIZE,COLON[r][c],gx_offset+c,r);
}

static void draw_full_grid(GContext *ctx,GRect b){
  for(int y=0;y<b.size.h;y+=CELL_SIZE)
    for(int x=0;x<b.size.w;x+=CELL_SIZE){
      graphics_context_set_stroke_color(ctx,GRID_COLOR);
      graphics_draw_rect(ctx,GRect(x,y,CELL_SIZE,CELL_SIZE));
    }
}

// ========================================================
static void canvas_update_proc(Layer *layer,GContext *ctx){
  GRect b=layer_get_bounds(layer);
  draw_full_grid(ctx,b);

  time_t now=time(NULL);
  struct tm *t=localtime(&now);
  int h1=t->tm_hour/10,h2=t->tm_hour%10,m1=t->tm_min/10,m2=t->tm_min%10;

  int tw=(DIGIT_WIDTH*4+3*DIGIT_SPACING+COLON_WIDTH)*CELL_SIZE;
  int th=DIGIT_HEIGHT*CELL_SIZE;
  int sx=((b.size.w-tw)/2/CELL_SIZE)*CELL_SIZE;
  int sy=((b.size.h-th)/2/CELL_SIZE)*CELL_SIZE;

  int gx=0;
  GPoint p=GPoint(sx,sy);
  draw_digit(ctx,p,h1,gx,digit_animating[0]);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  p.x+=(DIGIT_WIDTH+DIGIT_SPACING)*CELL_SIZE;
  draw_digit(ctx,p,h2,gx,digit_animating[1]);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  p.x+=(DIGIT_WIDTH+DIGIT_SPACING)*CELL_SIZE;
  draw_colon(ctx,p,gx);
  gx+=COLON_WIDTH+DIGIT_SPACING;
  p.x+=(COLON_WIDTH+DIGIT_SPACING)*CELL_SIZE;
  draw_digit(ctx,p,m1,gx,digit_animating[2]);
  gx+=DIGIT_WIDTH+DIGIT_SPACING;
  p.x+=(DIGIT_WIDTH+DIGIT_SPACING)*CELL_SIZE;
  draw_digit(ctx,p,m2,gx,digit_animating[3]);
}

// ========================================================
static void trigger_animation(void){
  reset_states();
  for(int i=0;i<4;i++) digit_animating[i]=true;
  s_animating=true;
  animation_step(NULL);
}

// ========================================================
static void tick_handler(struct tm *tick_time,TimeUnits units_changed){
  int h1=tick_time->tm_hour/10,h2=tick_time->tm_hour%10;
  int m1=tick_time->tm_min/10,m2=tick_time->tm_min%10;

  if(h1!=prev_h1){trigger_digit_animation(0);prev_h1=h1;}
  if(h2!=prev_h2){trigger_digit_animation(1);prev_h2=h2;}
  if(m1!=prev_m1){trigger_digit_animation(2);prev_m1=m1;}
  if(m2!=prev_m2){trigger_digit_animation(3);prev_m2=m2;}

  s_animating=true;
  animation_step(NULL);
}

// ========================================================
static void main_window_load(Window *w){
  s_canvas_layer=layer_create(layer_get_bounds(window_get_root_layer(w)));
  layer_set_update_proc(s_canvas_layer,canvas_update_proc);
  layer_add_child(window_get_root_layer(w),s_canvas_layer);
  trigger_animation();
}

static void main_window_unload(Window *w){layer_destroy(s_canvas_layer);}

static void init(void){
  s_main_window=window_create();
  window_set_background_color(s_main_window,INACTIVE_COLOR);
  window_set_window_handlers(s_main_window,(WindowHandlers){.load=main_window_load,.unload=main_window_unload});
  window_stack_push(s_main_window,true);
  tick_timer_service_subscribe(MINUTE_UNIT,tick_handler);
}

static void deinit(void){window_destroy(s_main_window);}
int main(void){init();app_event_loop();deinit();}
