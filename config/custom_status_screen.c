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
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static lv_obj_t *layer_label;
static lv_obj_t *battery_label;

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

int layer_listener(const zmk_event_t *eh) {
    update_layer_status();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(custom_layer_status, layer_listener);
ZMK_SUBSCRIPTION(custom_layer_status, zmk_layer_state_changed);

int battery_listener(const zmk_event_t *eh) {
    update_battery_status();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(custom_battery_status, battery_listener);
ZMK_SUBSCRIPTION(custom_battery_status, zmk_battery_state_changed);

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    
    // Set screen background to white (Sharp Memory displays are usually white when unset)
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // Layer Label
    layer_label = lv_label_create(screen);
    lv_obj_set_style_text_color(layer_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(layer_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(layer_label, LV_ALIGN_CENTER, 0, -10);
    update_layer_status();

    // Battery Label
    battery_label = lv_label_create(screen);
    lv_obj_set_style_text_color(battery_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(battery_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    update_battery_status();

    return screen;
}
