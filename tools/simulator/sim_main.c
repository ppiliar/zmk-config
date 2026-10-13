// Simulator main: reproduces the exact ZMK display init sequence, then toggles
// the theme and dumps frames to PNG. Compiles the *actual* vendored widget
// sources, unmodified, against stub headers (see stubs/ and sim_config.h).
//
// Build: run.sh [central|peripheral]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

#include "widgets/theme.h"

static bool display_initialized = false;
bool zmk_display_is_initialized(void) { return display_initialized; }

static struct k_work_q {
    int unused;
} sim_display_work_q;

struct k_work_q *zmk_display_work_q(void) { return &sim_display_work_q; }

// ---------------------------------------------------------------------------
// Subscription registry — the stub ZMK_DISPLAY_WIDGET_LISTENER macro (in
// stubs/zmk/display.h) registers each listener's _cb through these.
// ---------------------------------------------------------------------------
typedef int (*sim_listener_cb_t)(const zmk_event_t *eh);
sim_listener_cb_t sim_listener_registry[32];
int sim_listener_count = 0;

static void sim_raise_event(void) {
    for (int i = 0; i < sim_listener_count; i++) {
        sim_listener_registry[i](NULL);
    }
}

#include "widgets/screen.h"

static struct zmk_widget_screen central_widget;

// ---------------------------------------------------------------------------
// The real ZMK display init sequence, in order:
//   initialize_theme() -> zmk_display_status_screen() -> lv_scr_load
// Mirroring hardware: initialize_theme() is gated on CONFIG_LV_USE_THEME_MONO,
// an undefined symbol in the Zephyr 3.5 LVGL integration, so the firmware
// applies NO LVGL theme — the sim must not either. (Applying it here once
// produced a sim-only border/pad shift that hardware never shows.)
// ---------------------------------------------------------------------------

// Mirrors custom_status_screen.c (allocation + widget init only).
static lv_obj_t *sim_create_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    zmk_widget_screen_init(&central_widget, screen);
    lv_obj_align(zmk_widget_screen_obj(&central_widget), LV_ALIGN_TOP_LEFT, 0, 0);
    lv_scr_load(screen);
    return screen;
}

// ---------------------------------------------------------------------------
// PNG dump: 1bpp framebuffer -> PNG via bundled minimal encoder (sim_png.c).
// ---------------------------------------------------------------------------
#define SIM_FB_W 160
#define SIM_FB_H 68
uint8_t sim_fb[SIM_FB_W * SIM_FB_H];

// Verbose flag: SIM_VERBOSE=1 env var (run.sh passes extra args to gcc, so
// command-line flags would not reach the binary).
static int sim_verbose = 0;

__attribute__((constructor)) static void sim_verbose_from_env(void) {
    sim_verbose = getenv("SIM_VERBOSE") != NULL;
}

static void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    if (sim_verbose) {
        printf("[flush] area (%d,%d)-(%d,%d)\n", area->x1, area->y1, area->x2, area->y2);
    }
    // LV_COLOR_DEPTH=1: each lv_color_t holds one bit (0/1) in .full.
    for (int32_t y = area->y1; y <= area->y2; y++) {
        for (int32_t x = area->x1; x <= area->x2; x++) {
            sim_fb[y * SIM_FB_W + x] = (color_p->full != 0) ? 1 : 0;
            color_p++;
        }
    }
    lv_disp_flush_ready(disp_drv);
}

// Virtual clock advanced by sim_step(); LVGL's tick reads this (see
// stubs/zephyr/kernel.h).
uint32_t sim_tick_ms = 0;

// Advance the virtual clock in LVGL refresh-timer slices (30 ms period) so the
// timer actually fires and pending invalidations render. 32 slices = 960 ms =
// exactly one crystal animation cycle (16 frames x 60 ms), so dumps separated
// by this amount show the animation at the same phase and stay byte-comparable.
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

// ---------------------------------------------------------------------------
// Fake device state (kept in sync with what the listeners read) + the ZMK
// API stubs the widget listeners call.
// ---------------------------------------------------------------------------
static struct {
    uint8_t battery;
    bool usb_present;
    bool ble_connected;
    bool ble_bonded;
    uint8_t profile;
    uint8_t layer;
    const char *layer_label;
    uint8_t wpm[10];
} sim_dev = {
    .battery = 84,
    .usb_present = false,
    .ble_connected = true,
    .ble_bonded = true,
    .profile = 0,
    .layer = 0,
    .layer_label = "QWERTY",
    .wpm = {0, 0, 0, 12, 34, 45, 56, 48, 40, 38},
};

uint8_t zmk_battery_state_of_charge(void) { return sim_dev.battery; }
bool zmk_usb_is_powered(void) { return sim_dev.usb_present; }
uint8_t zmk_ble_active_profile_index(void) { return sim_dev.profile; }
bool zmk_ble_active_profile_is_connected(void) { return sim_dev.ble_connected; }
bool zmk_ble_active_profile_is_open(void) { return !sim_dev.ble_bonded; }
uint8_t zmk_keymap_highest_layer_active(void) { return sim_dev.layer; }
const char *zmk_keymap_layer_name(uint8_t layer) {
    return layer == 0 ? sim_dev.layer_label : "EXTRA 1";
}
uint8_t zmk_wpm_get_state(void) { return sim_dev.wpm[9]; }

struct zmk_endpoint_instance zmk_endpoints_selected(void) {
    struct zmk_endpoint_instance e = {.transport = ZMK_TRANSPORT_BLE,
                                      .ble = {.profile_index = sim_dev.profile}};
    return e;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    const char *out_prefix = "out/frame";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc)
            out_prefix = argv[++i];
    }

    // --- LVGL core init (mirrors zephyr lvgl.c + zmk display init) ---
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

    // --- ZMK boot: initialize_theme() (no-op here, see above), then the status
// screen ---

    // Sim-only shim: lv_canvas_transform's rgb_no_aa() reads
    // _lv_refr_get_disp_refreshing()->driver->color_chroma_key, which is only
    // valid inside a refresh. On hardware a refresh has always run by the time
    // the first widget draw happens; the chroma key is irrelevant for
    // TRUE_COLOR canvases. Point it at our display so the sim can draw.
    {
        extern void _lv_refr_set_disp_refreshing(lv_disp_t * disp);
        _lv_refr_set_disp_refreshing(lv_disp_get_default());
    }

    sim_create_status_screen();
    display_initialized = true;

    // First frame: fire the event listeners that populate the widgets.
    sim_raise_event();
    sim_step(SIM_SLICES_PER_DUMP);

    char path[512];
    snprintf(path, sizeof(path), "%s_1_boot.png", out_prefix);
    sim_dump(path);

    // --- Simulate a keybind press: theme toggle ---
    nvgem_theme_set(!nvgem_theme_is_inverted());
    sim_step(SIM_SLICES_PER_DUMP);
    snprintf(path, sizeof(path), "%s_2_after_toggle.png", out_prefix);
    sim_dump(path);

    // Toggle back: must return to a pixel-identical frame.
    nvgem_theme_set(!nvgem_theme_is_inverted());
    sim_step(SIM_SLICES_PER_DUMP);
    snprintf(path, sizeof(path), "%s_3_after_toggle_back.png", out_prefix);
    sim_dump(path);

    printf("done\n");
    return 0;
}
