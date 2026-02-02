#include "layer_display_state.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

static atomic_t layer_display_value = ATOMIC_INIT(0);
static void (*update_cb)(void);
static struct k_work layer_display_work;

static void layer_display_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    if (update_cb != NULL) {
        update_cb();
    }
}

void zmk_layer_display_register_update_cb(void (*cb)(void)) {
    update_cb = cb;
    k_work_init(&layer_display_work, layer_display_work_handler);
}

void zmk_layer_display_set(uint8_t layer) {
    atomic_set(&layer_display_value, layer);
    if (update_cb != NULL) {
        k_work_submit(&layer_display_work);
    }
}

uint8_t zmk_layer_display_get(void) {
    return (uint8_t)atomic_get(&layer_display_value);
}
