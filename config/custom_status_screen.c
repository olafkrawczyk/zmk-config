/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/status_screen.h>
#include <zmk/battery.h>
#include <zmk/keymap.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static lv_obj_t *layer_label;
static lv_obj_t *battery_label;

// --- Update Functions ---

static void update_layer_status() {
    if (layer_label == NULL) return;

    uint8_t layer = zmk_keymap_highest_layer_active();
    const char *name = zmk_keymap_layer_name(layer);

    if (name != NULL) {
        lv_label_set_text(layer_label, name);
    } else {
        lv_label_set_text_fmt(layer_label, "L:%d", layer);
    }
}

static void update_battery_status() {
    if (battery_label == NULL) return;

    uint8_t level = zmk_battery_state_of_charge();
    lv_label_set_text_fmt(battery_label, "%d%%", level);
}

// --- Event Listeners ---

int layer_listener(const zmk_event_t *eh) {
    if (zmk_event_check(eh, ZMK_EVENT_LAYER_STATE_CHANGED)) {
        update_layer_status();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(custom_layer_status, layer_listener);
ZMK_SUBSCRIPTION(custom_layer_status, zmk_layer_state_changed);

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
    
    // Style for high visibility on monochrome
    static lv_style_t style_base;
    lv_style_init(&style_base);
    lv_style_set_text_color(&style_base, lv_color_black());
    lv_style_set_bg_color(&style_base, lv_color_white());
    
    lv_obj_add_style(screen, &style_base, LV_PART_MAIN);

    // 1. Layer Label (Top)
    layer_label = lv_label_create(screen);
    lv_obj_align(layer_label, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(layer_label, &lv_font_montserrat_14, 0);
    update_layer_status();

    // 2. Battery Label (Bottom)
    battery_label = lv_label_create(screen);
    lv_obj_align(battery_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_14, 0);
    update_battery_status();

    return screen;
}
