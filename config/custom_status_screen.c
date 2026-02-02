/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/display/status_screen.h>
#include <zmk/battery.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Simple status screen to display Battery % and Active Layer
// without relying on the nice_view built-in widget which depends on BLE.

static lv_obj_t *layer_label;
static lv_obj_t *battery_label;
static lv_timer_t *refresh_timer;

static int custom_status_screen_init(void) {
    printk("custom_status_screen: init\n");
    LOG_INF("Custom status screen module init");
#if DT_HAS_CHOSEN(zephyr_display)
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (device_is_ready(display)) {
        printk("custom_status_screen: display ready in init\n");
        LOG_INF("Display device ready in init");
    } else {
        printk("custom_status_screen: display NOT ready in init\n");
        LOG_ERR("Display device not ready in init");
    }
#else
    printk("custom_status_screen: no zephyr_display chosen in init\n");
    LOG_ERR("No zephyr_display chosen in init");
#endif
    return 0;
}

SYS_INIT(custom_status_screen_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

// --- Update Functions ---

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_SHOW_LAYER)
#include "layer_display_state.h"

/*
 * Avoid a hard link-time dependency on ZMK's keymap implementation.
 * We can derive layer names directly from the keymap devicetree (display-name/label).
 *
 * This prevents "undefined reference to zmk_keymap_layer_name" on targets/configs
 * where the keymap module is not built/linked (e.g. dongle/minimal builds).
 */
#define DT_DRV_COMPAT zmk_keymap
#if DT_HAS_COMPAT_STATUS_OKAY(zmk_keymap)
#define _LAYER_NAME(node_id) DT_PROP_OR(node_id, display_name, DT_PROP_OR(node_id, label, ""))
static const char *layer_names[] = {
    DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_INST(0, zmk_keymap), _LAYER_NAME, (, ))};

static const char *layer_name_from_dt(uint8_t layer) {
    if (layer < ARRAY_SIZE(layer_names)) {
        const char *name = layer_names[layer];
        if (name != NULL && name[0] != '\0') {
            return name;
        }
    }
    return NULL;
}
#else
static const char *layer_name_from_dt(uint8_t layer) {
    ARG_UNUSED(layer);
    return NULL;
}
#endif

static void update_layer_status() {
    if (layer_label == NULL) return;

    uint8_t layer = zmk_layer_display_get();
    const char *name = layer_name_from_dt(layer);
    if (name != NULL) {
        lv_label_set_text(layer_label, name);
        return;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "Layer %u", layer);
    lv_label_set_text(layer_label, buf);
}
#endif

static void update_battery_status() {
    if (battery_label == NULL) return;

    uint8_t level = zmk_battery_state_of_charge();
    char buf[16];
    snprintf(buf, sizeof(buf), "BAT: %u%%", level);
    lv_label_set_text(battery_label, buf);
}

static void refresh_timer_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);
#if IS_ENABLED(CONFIG_ZMK_DISPLAY_SHOW_LAYER)
    update_layer_status();
#endif
    update_battery_status();
}

// --- Screen Setup ---

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    printk("custom_status_screen: screen create\n");

#if DT_HAS_CHOSEN(zephyr_display)
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (device_is_ready(display)) {
        display_blanking_off(display);
        printk("custom_status_screen: display unblanked\n");
        LOG_INF("Display ready, unblanked");
    } else {
        printk("custom_status_screen: display NOT ready in screen\n");
        LOG_ERR("Display device not ready");
    }
#else
    printk("custom_status_screen: no zephyr_display chosen in screen\n");
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
