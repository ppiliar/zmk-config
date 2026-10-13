// Simulator main for the PERIPHERAL half of the split.
// Same sequence as sim_main.c; screen_peripheral.c provides
// zmk_widget_screen_init. The peripheral's struct is identical in layout to
// the central's, so we reuse the central header — the sim only ever handles
// one half per binary.
//
// Build: run.sh peripheral
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

#include "widgets/theme.h"
#include "widgets/screen_peripheral.h"

static bool display_initialized = false;
bool zmk_display_is_initialized(void) { return display_initialized; }

static struct k_work_q {
    int unused;
} sim_display_work_q;

struct k_work_q *zmk_display_work_q(void) { return &sim_display_work_q; }

// Subscription registry (the peripheral widgets register their listeners too).
typedef int (*sim_listener_cb_t)(const zmk_event_t *eh);
sim_listener_cb_t sim_listener_registry[32];
int sim_listener_count = 0;

// Mirroring hardware: initialize_theme() is gated on CONFIG_LV_USE_THEME_MONO,
// an undefined symbol in the Zephyr 3.5 LVGL integration, so the firmware
// applies NO LVGL theme — the sim must not either.

static struct zmk_widget_screen peripheral_widget;

static lv_obj_t *sim_create_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    zmk_widget_screen_init(&peripheral_widget, screen);
    lv_obj_align(zmk_widget_screen_obj(&peripheral_widget), LV_ALIGN_TOP_LEFT, 0, 0);
    lv_scr_load(screen);
    return screen;
}

#define SIM_FB_W 160
#define SIM_FB_H 68
uint8_t sim_fb[SIM_FB_W * SIM_FB_H];

static void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    for (int32_t y = area->y1; y <= area->y2; y++) {
        for (int32_t x = area->x1; x <= area->x2; x++) {
            sim_fb[y * SIM_FB_W + x] = (color_p->full != 0) ? 1 : 0;
            color_p++;
        }
    }
    lv_disp_flush_ready(disp_drv);
}

// Virtual clock + deterministic stepping — see sim_main.c. 32 slices of the
// 30 ms refresh period = 960 ms = one full crystal animation cycle, so dumps
// always land on the same animation phase and stay byte-comparable.
uint32_t sim_tick_ms = 0;

#define SIM_SLICE_MS 30
#define SIM_SLICES_PER_DUMP 32
static void sim_step(uint32_t slices) {
    for (uint32_t i = 0; i < slices; i++) {
        sim_tick_advance(SIM_SLICE_MS);
        lv_task_handler();
    }
}

static void sim_dump(const char *path) {
    extern void sim_png_write(const uint8_t *fb, int w, int h, const char *path);
    extern void sim_png_write_rotated(const uint8_t *fb, int w, int h, const char *path);
    unsigned long sum = 0;
    for (int i = 0; i < SIM_FB_W * SIM_FB_H; i++)
        sum = sum * 31 + sim_fb[i];
    sim_png_write(sim_fb, SIM_FB_W, SIM_FB_H, path);
    // Companion "as seen on the keyboard" view (panel mounted rotated 90 deg).
    char path_view[520];
    snprintf(path_view, sizeof(path_view), "%.*s_view.png", (int)strlen(path) - 4, path);
    sim_png_write_rotated(sim_fb, SIM_FB_W, SIM_FB_H, path_view);
    printf("wrote %s (fbsum=%lu)\n", path, sum);
}

// Fake peripheral state + the API stubs the peripheral listeners call.
static struct {
    bool connected;
    uint8_t battery;
} sim_dev = {.connected = true, .battery = 71};

uint8_t zmk_battery_state_of_charge(void) { return sim_dev.battery; }
bool zmk_usb_is_powered(void) { return false; }
bool zmk_split_bt_peripheral_is_connected(void) { return sim_dev.connected; }

int main(int argc, char **argv) {
    const char *out_prefix = "out/frame";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc)
            out_prefix = argv[++i];
    }

    lv_init();
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[SIM_FB_W * SIM_FB_H];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SIM_FB_W * SIM_FB_H);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SIM_FB_W;
    disp_drv.ver_res = SIM_FB_H;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;
    lv_disp_drv_register(&disp_drv);

    // initialize_theme() is a no-op in this build (see the note above).

    // Sim-only shim: see sim_main.c (chroma-key read outside a refresh).
    {
        extern void _lv_refr_set_disp_refreshing(lv_disp_t * disp);
        _lv_refr_set_disp_refreshing(lv_disp_get_default());
    }

    sim_create_status_screen();
    display_initialized = true;

    sim_step(SIM_SLICES_PER_DUMP);

    char path[512];
    snprintf(path, sizeof(path), "%s_1_boot.png", out_prefix);
    sim_dump(path);

    nvgem_theme_set(!nvgem_theme_is_inverted());
    sim_step(SIM_SLICES_PER_DUMP);
    snprintf(path, sizeof(path), "%s_2_after_toggle.png", out_prefix);
    sim_dump(path);

    nvgem_theme_set(!nvgem_theme_is_inverted());
    sim_step(SIM_SLICES_PER_DUMP);
    snprintf(path, sizeof(path), "%s_3_after_toggle_back.png", out_prefix);
    sim_dump(path);

    printf("done\n");
    return 0;
}
