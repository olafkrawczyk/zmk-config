#pragma once

#include <stdint.h>

void zmk_layer_display_set(uint8_t layer);
uint8_t zmk_layer_display_get(void);
void zmk_layer_display_register_update_cb(void (*cb)(void));
