/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/display/status_screen.h>
#include <zmk/battery.h>
#include <zmk/keymap.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Simple status screen to display Battery % and Active Layer
// without relying on the nice_view built-in widget which depends on BLE.

static lv_obj_t *layer_label;
static lv_obj_t *battery_label;
static lv_timer_t *refresh_timer;

// --- Update Functions ---

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_SHOW_LAYER)
#include "layer_display_state.h"

static void update_layer_status() {
    if (layer_label == NULL) return;

    uint8_t layer = zmk_layer_display_get();
    const char *name = zmk_keymap_layer_name(layer);

    if (name != NULL) {
        lv_label_set_text(layer_label, name);
    } else {
        lv_label_set_text_fmt(layer_label, "Layer %d", layer);
    }
}
#endif

static void update_battery_status() {
    if (battery_label == NULL) return;

    uint8_t level = zmk_battery_state_of_charge();
    lv_label_set_text_fmt(battery_label, "BAT: %d%%", level);
}

static void refresh_timer_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);
#if IS_ENABLED(CONFIG_ZMK_DISPLAY_SHOW_LAYER)
    update_layer_status();
#endif
    update_battery_status();
}

// --- Event Listeners ---

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_SHOW_LAYER)
int layer_listener(const zmk_event_t *eh) {
    if (zmk_event_check(eh, ZMK_EVENT_LAYER_STATE_CHANGED)) {
        update_layer_status();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(custom_layer_status, layer_listener);
ZMK_SUBSCRIPTION(custom_layer_status, zmk_layer_state_changed);
#endif

int battery_listener(const zmk_event_t *eh) {
    if (zmk_event_check(eh, ZMK_EVENT_BATTERY_STATE_CHANGED)) {
        update_battery_status();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(custom_battery_status, battery_listener);
ZMK_SUBSCRIPTION(custom_battery_status, zmk_battery_state_changed);

// --- Screen Setup ---

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

#if DT_HAS_CHOSEN(zephyr_display)
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (device_is_ready(display)) {
        display_blanking_off(display);
        LOG_INF("Display ready, unblanked");
    } else {
        LOG_ERR("Display device not ready");
    }
#else
    LOG_ERR("No zephyr_display chosen");
#endif

    LOG_INF("Custom status screen init");
    
    // 1. Layer Label (Top Center, Large)
#if IS_ENABLED(CONFIG_ZMK_DISPLAY_SHOW_LAYER)
    layer_label = lv_label_create(screen);
    lv_obj_align(layer_label, LV_ALIGN_CENTER, 0, -10);
    update_layer_status();
#else
    layer_label = NULL;
#endif

    // 2. Battery Label (Bottom Center, Small)
    battery_label = lv_label_create(screen);
    lv_obj_align(battery_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    update_battery_status();

    if (refresh_timer == NULL) {
        refresh_timer = lv_timer_create(refresh_timer_cb, 2000, NULL);
    }

    return screen;
}
