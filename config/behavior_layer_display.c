#define DT_DRV_COMPAT zmk_behavior_layer_display

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <drivers/behavior.h>

#include "layer_display_state.h"

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_layer_display_pressed(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);
    zmk_layer_display_set((uint8_t)binding->param1);
    return 0;
}

static int behavior_layer_display_released(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return 0;
}

static const struct behavior_driver_api behavior_layer_display_driver_api = {
    .binding_pressed = behavior_layer_display_pressed,
    .binding_released = behavior_layer_display_released,
};

static int behavior_layer_display_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

BEHAVIOR_DT_INST_DEFINE(0, behavior_layer_display_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_layer_display_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
