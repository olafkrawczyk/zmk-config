#include "layer_display_state.h"

#include <zephyr/sys/atomic.h>

static atomic_t layer_display_value = ATOMIC_INIT(0);

void zmk_layer_display_set(uint8_t layer) {
    atomic_set(&layer_display_value, layer);
}

uint8_t zmk_layer_display_get(void) {
    return (uint8_t)atomic_get(&layer_display_value);
}
