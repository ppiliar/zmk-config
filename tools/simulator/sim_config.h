// Kconfig mirror for the simulator build (forced via -include).
// Central-half build by default; see build.sh for the peripheral variant.
#pragma once

#define CONFIG_ZMK_DISPLAY 1
#define CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM 1
#define CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED 1

// Split: central by default; peripheral build overrides with -DSIM_PERIPHERAL
#ifdef SIM_PERIPHERAL
#define CONFIG_ZMK_SPLIT 1
#define CONFIG_ZMK_SPLIT_ROLE_CENTRAL 0
#else
#define CONFIG_ZMK_SPLIT 1
#define CONFIG_ZMK_SPLIT_ROLE_CENTRAL 1
#endif

#define CONFIG_USB_DEVICE_STACK 1
#define CONFIG_ZMK_BLE 1

#define CONFIG_NICE_VIEW_WIDGET_STATUS 1
// Boot default theme: 0 = light (matches our current configs, which leave this unset)
#ifndef CONFIG_NICE_VIEW_WIDGET_INVERTED
#define CONFIG_NICE_VIEW_WIDGET_INVERTED 0
#endif

#define CONFIG_NICE_VIEW_GEM_ANIMATION 1
#define CONFIG_NICE_VIEW_GEM_ANIMATION_MS 960
#define CONFIG_NICE_VIEW_GEM_WPM_FIXED_RANGE 1
#define CONFIG_NICE_VIEW_GEM_WPM_FIXED_RANGE_MAX 100

// ZMK core display: NO LVGL theme is applied in the firmware build
// (initialize_theme() is gated on CONFIG_LV_USE_THEME_MONO, an undefined
// symbol in the Zephyr 3.5 LVGL integration). LV_USE_THEME_MONO below is kept
// =1 only so lv_theme_mono.c compiles; nothing calls it. CONFIG_ZMK_DISPLAY_INVERT
// is likewise unused; the boot theme comes from CONFIG_NICE_VIEW_WIDGET_INVERTED.
#ifndef CONFIG_ZMK_DISPLAY_INVERT
#define CONFIG_ZMK_DISPLAY_INVERT 0
#endif

// Mirror zephyr's modules/lvgl/include/lv_conf.h (the LVGL config ZMK builds with):
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE <zephyr/kernel.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (k_uptime_get_32())
#define LV_SPRINTF_CUSTOM 1
#define LV_SPRINTF_INCLUDE "stdio.h"
#define lv_snprintf snprintf
#define lv_vsnprintf vsnprintf
#define LV_MEMCPY_MEMSET_STD 1
#define LV_CONF_SUPPRESS_DEFINE_CHECK 1

// LVGL Kconfig mirror (zephyr picks these from lvgl's own Kconfig; we mirror
// the resolved values for LV_COLOR_DEPTH_1 + widget selection)
#define LV_COLOR_DEPTH 1
#define LV_COLOR_DEPTH_1 1
#define LV_MEM_CUSTOM 1
#define LV_USE_THEME_MONO 1
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_BASIC 0
#define LV_USE_LABEL 1
#define LV_USE_IMG 1
#define LV_USE_CANVAS 1
#define LV_USE_ANIMIMG 1
#define LV_USE_ANIMATION 1
#define LV_USE_FLEX 1
#define LV_USE_GRID 0
#define LV_USE_TABVIEW 0
#define LV_USE_WIN 0
#define LV_USE_BTN 0
#define LV_USE_BTNMATRIX 0
#define LV_USE_MSGBOX 0
#define LV_USE_BAR 0
#define LV_USE_SLIDER 0
#define LV_USE_SWITCH 0
#define LV_USE_CHECKBOX 0
#define LV_USE_CHART 0
#define LV_USE_ROLLER 0
#define LV_USE_DROPDOWN 0
#define LV_USE_TEXTAREA 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LIST 0
#define LV_USE_MENU 0
#define LV_USE_SPINBOX 0
#define LV_USE_TABLE 0
#define LV_USE_LED 0
#define LV_USE_SPINNER 0
#define LV_USE_ARC 0
#define LV_USE_CALENDAR 0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN 0
#define LV_USE_METER 0
#define LV_USE_TILEVIEW 0
#define LV_USE_SPAN 0
#define LV_USE_OBJ_REALIGN 0
#define LV_USE_SNAPSHOT 0
#define LV_USE_USER_DATA 1
#define LV_USE_MONKEY 0
#define LV_USE_PER_MONITOR 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_LOG 0
#define LV_USE_ASSERT 0
#define LV_USE_FONT_COMPRESSED 0
#define LV_USE_BIDI 0
#define LV_USE_FLEX 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_Z_VDB_SIZE 100
#define LV_DPI_DEF 161
#define LV_Z_BITS_PER_PIXEL 1
#define LV_Z_MEM_POOL_SIZE 4096

// ZMK font choice (montserrat_14 is the default small font)
#define SIM_LV_FONT_DEFAULT &lv_font_montserrat_14
