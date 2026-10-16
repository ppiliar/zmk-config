#define DT_DRV_COMPAT zmk_behavior_display_theme

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/behavior.h>
#include <zmk/display.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Params: 0 = toggle, 1 = force light, 2 = force dark.
#define DISP_THEME_PARAM_INVERTED 1
#define DISP_THEME_PARAM_NORMAL 2

// RAM-only state: a reboot returns to the CONFIG_ZMK_DISPLAY_INVERT boot
// default. No flash writes, so toggling costs no battery.
static atomic_t theme_inverted;

static void theme_apply_work_cb(struct k_work *work) {
    // Runs on the display work queue: the only context where LVGL calls are
    // safe. Stock status widgets are bare lv_labels that set no colors, so
    // they inherit these props from the active screen.
    lv_obj_t *scr = lv_scr_act();
    lv_color_t bg = atomic_get(&theme_inverted) ? lv_color_white() : lv_color_black();
    lv_color_t fg = atomic_get(&theme_inverted) ? lv_color_black() : lv_color_white();
    lv_obj_set_style_bg_color(scr, bg, LV_PART_MAIN);
    lv_obj_set_style_text_color(scr, fg, LV_PART_MAIN);
    lv_obj_invalidate(scr);
}

static K_WORK_DEFINE(theme_apply_work, theme_apply_work_cb);

bool disp_theme_is_inverted(void) { return atomic_get(&theme_inverted); }

void disp_theme_set(bool inverted) {
    if (atomic_set(&theme_inverted, inverted) == (atomic_val_t)inverted) {
        return;
    }
    k_work_submit_to_queue(zmk_display_work_q(), &theme_apply_work);
}

static int on_display_theme_binding_pressed(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    bool target;
    switch (binding->param1) {
    case DISP_THEME_PARAM_INVERTED:
        target = true;
        break;
    case DISP_THEME_PARAM_NORMAL:
        target = false;
        break;
    default:
        // Toggle (only reachable when invoked without the central-side
        // conversion, e.g. via Studio replay).
        target = !disp_theme_is_inverted();
        break;
    }
    disp_theme_set(target);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_display_theme_binding_released(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

// Rewrites a toggle into the absolute target state on the central, so the
// value forwarded to (and replayed by) peripherals is idempotent and a half
// that rebooted re-converges on the next press.
static int display_theme_binding_convert_central_state_dependent_params(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    if (binding->param1 == 0) {
        binding->param1 =
            disp_theme_is_inverted() ? DISP_THEME_PARAM_NORMAL : DISP_THEME_PARAM_INVERTED;
    }
    return 0;
}

static const struct behavior_driver_api behavior_display_theme_driver_api = {
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
    .binding_convert_central_state_dependent_params =
        display_theme_binding_convert_central_state_dependent_params,
    .binding_pressed = on_display_theme_binding_pressed,
    .binding_released = on_display_theme_binding_released,
};

static int display_theme_init(const struct device *dev) {
    atomic_set(&theme_inverted, IS_ENABLED(CONFIG_ZMK_DISPLAY_INVERT));
    return 0;
}

#define DISP_THEME_INST(n)                                                                         \
    BEHAVIOR_DT_INST_DEFINE(n, display_theme_init, NULL, NULL, NULL, POST_KERNEL,                  \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_display_theme_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DISP_THEME_INST)
