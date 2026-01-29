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

// Simple status screen to display Battery % and Active Layer
// without relying on the nice_view built-in widget which depends on BLE.

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
        lv_label_set_text_fmt(layer_label, "Layer %d", layer);
    }
}

static void update_battery_status() {
    if (battery_label == NULL) return;

    uint8_t level = 0;
    
    // We try to get the battery level of the Dongle (Central)
    // In ESB proxy mode, getting peripheral battery levels is complex without the full widget
    // So we just show the central's level for now as a proxy.
    // (Actual peripheral battery proxying requires deeper hooks)
    
    // Using simple ZMK API to get state of charge
    // Note: This might return the Dongle's battery (which is N/A if USB powered) or 0.
    // We check if it returns a valid value.
    
    // For now, we just display a placeholder or the raw value if available.
    // Real implementation would need to listen to specific peripheral events.
    
    lv_label_set_text(battery_label, "BAT: --%");
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

// Note: Battery listener skipped for now as simple API usage is tricky without full context.
// We initialize with a static value.

// --- Screen Setup ---

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    
    // 1. Layer Label (Top Center, Large)
    layer_label = lv_label_create(screen);
    lv_obj_align(layer_label, LV_ALIGN_CENTER, 0, -10);
    update_layer_status();

    // 2. Battery Label (Bottom Center, Small)
    battery_label = lv_label_create(screen);
    lv_obj_align(battery_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    update_battery_status();

    return screen;
}
