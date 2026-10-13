#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <string.h>

#include <lvgl.h>

#include <zmk/display.h>

#include "theme.h"

static atomic_t theme_inverted;

bool nvgem_theme_is_inverted(void) { return atomic_get(&theme_inverted) == 1; }

// lvgl.h -> lv_draw.h -> lv_img_cache.h
static const uint8_t PAL_WB[8] = {0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff};
static const uint8_t PAL_BW[8] = {0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff};

void nvgem_apply_palette(uint8_t *map, bool opposite_authored, bool inverted) {
    memcpy(map, (inverted ^ opposite_authored) ? PAL_BW : PAL_WB, 8);
}

__attribute__((weak)) void nvgem_crystal_apply(void) {}

static void theme_apply_work_cb(struct k_work *_work) {
    nvgem_images_apply();
    nvgem_crystal_apply();
    lv_img_cache_invalidate_src(NULL);

    // ZMK applies no LVGL theme in this build: initialize_theme() is gated on
    // CONFIG_LV_USE_THEME_MONO, an undefined symbol in the Zephyr 3.5 LVGL
    // integration, so lv_theme_mono_init must not be called here either — it
    // would impose card styling (border + pad) on the widget containers at the
    // first toggle, shifting every tile. The widget objs carry explicit
    // background styles instead (pinned at init, re-pinned by the redraw
    // below), which is all the theme handling this widget needs.
    nvgem_theme_request_redraw();
}
static K_WORK_DEFINE(theme_apply_work, theme_apply_work_cb);

void nvgem_theme_set(bool inverted) {
    atomic_val_t target = inverted ? 1 : 0;
    if (atomic_get(&theme_inverted) == target) {
        return;
    }
    atomic_set(&theme_inverted, target);
    k_work_submit_to_queue(zmk_display_work_q(), &theme_apply_work);
}

static int nvgem_theme_init(const struct device *_dev) {
    atomic_set(&theme_inverted, IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? 1 : 0);
    nvgem_images_apply();
    nvgem_crystal_apply();
    return 0;
}

SYS_INIT(nvgem_theme_init, POST_KERNEL, 50);
