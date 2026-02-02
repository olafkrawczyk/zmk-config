#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>
#include <zmk/split/central.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *layer_display_behavior;
static uint8_t last_sent_layer = 0xFF;

static int send_layer_to_left(uint8_t layer) {
    if (layer_display_behavior == NULL) {
        return -ENODEV;
    }

    if (layer == last_sent_layer) {
        return 0;
    }

    struct zmk_behavior_binding binding = {
        .behavior_dev = "LAYER_DISPLAY",
        .param1 = layer,
        .param2 = 0,
    };

    struct zmk_behavior_binding_event event = {0};

    int err = zmk_split_central_invoke_behavior(CONFIG_ZMK_LAYER_DISPLAY_LEFT_SOURCE, &binding,
                                                event, true);
    if (err == 0) {
        last_sent_layer = layer;
    }

    return err;
}

static int layer_display_relay_listener(const zmk_event_t *eh) {
    if (as_zmk_layer_state_changed(eh) == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    uint8_t layer = zmk_keymap_highest_layer_active();
    int err = send_layer_to_left(layer);
    if (err != 0) {
        LOG_WRN("Layer relay failed: %d", err);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_display_relay, layer_display_relay_listener);
ZMK_SUBSCRIPTION(layer_display_relay, zmk_layer_state_changed);

static int layer_display_relay_init(void) {
    layer_display_behavior = device_get_binding("LAYER_DISPLAY");
    if (layer_display_behavior == NULL) {
        LOG_ERR("Layer display behavior not found");
        return -ENODEV;
    }

    uint8_t layer = zmk_keymap_highest_layer_active();
    (void)send_layer_to_left(layer);

    return 0;
}

SYS_INIT(layer_display_relay_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
