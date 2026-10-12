#define DT_DRV_COMPAT zmk_behavior_nvgem

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>

#include "../widgets/theme.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int on_nvgem_binding_pressed(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event) {
    bool target;
    switch (binding->param1) {
    case NVGEM_PARAM_INVERTED:
        target = true;
        break;
    case NVGEM_PARAM_NORMAL:
        target = false;
        break;
    default:
        // Toggle (only reachable when invoked without the central-side
        // conversion, e.g. via Studio replay).
        target = !nvgem_theme_is_inverted();
        break;
    }
    nvgem_theme_set(target);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_nvgem_binding_released(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

// Rewrites a toggle into the absolute target state on the central, so the
// value forwarded to (and replayed by) peripherals is idempotent and a half
// that rebooted re-converges on the next press.
static int nvgem_binding_convert_central_state_dependent_params(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    if (binding->param1 == 0) {
        binding->param1 =
            nvgem_theme_is_inverted() ? NVGEM_PARAM_NORMAL : NVGEM_PARAM_INVERTED;
    }
    return 0;
}

static const struct behavior_driver_api behavior_nvgem_driver_api = {
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
    .binding_convert_central_state_dependent_params =
        nvgem_binding_convert_central_state_dependent_params,
    .binding_pressed = on_nvgem_binding_pressed,
    .binding_released = on_nvgem_binding_released,
};

#define NVGEM_INST(n)                                                                              \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_nvgem_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NVGEM_INST)
