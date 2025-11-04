#pragma once

#include <pebble.h>

typedef struct RoundyBackgroundLayer RoundyBackgroundLayer;

RoundyBackgroundLayer *roundy_background_layer_create(GRect frame);
void roundy_background_layer_destroy(RoundyBackgroundLayer *layer);
Layer *roundy_background_layer_get_layer(RoundyBackgroundLayer *layer);
void roundy_background_layer_mark_dirty(RoundyBackgroundLayer *layer);
