/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/status_screen.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// We define a bare-minimum status screen to satisfy the linker
// and avoid the "undefined reference" errors from the default nice_view widget
// which expects BLE to be active.

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);
    
    // Add a simple label "ESB" to confirm it's working
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "ESB MODE");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    return screen;
}
