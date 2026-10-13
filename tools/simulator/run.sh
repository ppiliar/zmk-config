#!/bin/bash
# Native LVGL simulator for the vendored nice_view_gem shield.
#
# Compiles the actual shield widget sources (unmodified) against the exact
# LVGL revision Zephyr pins, with stub Zephyr/ZMK headers, and renders
# boot -> theme-toggle -> toggle-back frames to PNG. Use it to develop and
# verify display/theme changes without flashing hardware.
#
# Usage: run.sh [central|peripheral] [extra gcc args...]
#   SANITIZE=1 run.sh central   — build with AddressSanitizer
set -e

SIM="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$SIM/../.." && pwd)"
GEM="$REPO/boards/shields/nice_view_gem"

# LVGL revision pinned by Zephyr (v3.5.0+zmk-fixes, see zephyr/west.yml).
# Update this when bumping the ZMK/Zephyr revision.
LVGL_REV=8a6a2d1d29d17d1e4bdc94c243c146a39d635fdd
LVGL="$SIM/lvgl"

if [ ! -d "$LVGL/src" ]; then
    echo "Fetching LVGL $LVGL_REV into $LVGL ..."
    git clone --filter=blob:none --no-checkout https://github.com/lvgl/lvgl "$LVGL"
    git -C "$LVGL" checkout "$LVGL_REV"
fi
# LVGL expects its config next to the library; keep it in sync with the sim's.
if [ ! -f "$LVGL/lv_conf.h" ] || [ "$SIM/sim_config.h" -nt "$LVGL/lv_conf.h" ]; then
    cp "$SIM/sim_config.h" "$LVGL/lv_conf.h"
fi

HALF="${1:-central}"
shift || true

if [ "$HALF" = "peripheral" ]; then
    ROLE=-DSIM_PERIPHERAL
    SRC_WIDGETS="widgets/screen_peripheral.c widgets/animation.c widgets/output.c widgets/battery.c widgets/util.c widgets/theme.c assets/pixel_operator_mono.c"
    SRC_ASSETS="assets/crystal.c assets/images.c"
    MAIN_SRC=sim_main_peripheral.c
else
    ROLE=""
    SRC_WIDGETS="widgets/screen.c widgets/battery.c widgets/output.c widgets/layer.c widgets/profile.c widgets/wpm.c widgets/util.c widgets/theme.c assets/pixel_operator_mono.c"
    SRC_ASSETS="assets/images.c"
    MAIN_SRC=sim_main.c
fi

mkdir -p "$SIM/build" "$SIM/out"
OUT="$SIM/build/sim_$HALF"

# LVGL sources needed by the widgets + our driver glue. Objects are cached in
# build/ and only rebuilt when the source or the sim config changes.
LVGL_SRCS="
src/core/lv_disp.c
src/core/lv_group.c
src/core/lv_indev.c
src/core/lv_indev_scroll.c
src/core/lv_obj.c
src/core/lv_obj_class.c
src/core/lv_obj_draw.c
src/core/lv_obj_pos.c
src/core/lv_obj_scroll.c
src/core/lv_obj_style.c
src/core/lv_obj_style_gen.c
src/core/lv_obj_tree.c
src/core/lv_event.c
src/core/lv_refr.c
src/core/lv_theme.c
src/misc/lv_anim.c
src/misc/lv_anim_timeline.c
src/misc/lv_area.c
src/misc/lv_async.c
src/misc/lv_bidi.c
src/misc/lv_color.c
src/misc/lv_fs.c
src/misc/lv_gc.c
src/misc/lv_ll.c
src/misc/lv_log.c
src/misc/lv_math.c
src/misc/lv_mem.c
src/misc/lv_printf.c
src/misc/lv_style.c
src/misc/lv_style_gen.c
src/misc/lv_templ.c
src/misc/lv_timer.c
src/misc/lv_txt.c
src/misc/lv_txt_ap.c
src/misc/lv_utils.c
src/draw/lv_draw.c
src/draw/lv_draw_arc.c
src/draw/lv_draw_img.c
src/draw/lv_draw_label.c
src/draw/lv_draw_line.c
src/draw/lv_draw_mask.c
src/draw/lv_draw_rect.c
src/draw/lv_draw_transform.c
src/draw/lv_draw_layer.c
src/draw/lv_draw_triangle.c
src/draw/lv_img_buf.c
src/draw/lv_img_cache.c
src/draw/lv_img_decoder.c
src/draw/sw/lv_draw_sw.c
src/draw/sw/lv_draw_sw_arc.c
src/draw/sw/lv_draw_sw_blend.c
src/draw/sw/lv_draw_sw_dither.c
src/draw/sw/lv_draw_sw_gradient.c
src/draw/sw/lv_draw_sw_img.c
src/draw/sw/lv_draw_sw_layer.c
src/draw/sw/lv_draw_sw_letter.c
src/draw/sw/lv_draw_sw_line.c
src/draw/sw/lv_draw_sw_polygon.c
src/draw/sw/lv_draw_sw_rect.c
src/draw/sw/lv_draw_sw_transform.c
src/hal/lv_hal_disp.c
src/hal/lv_hal_indev.c
src/hal/lv_hal_tick.c
src/font/lv_font.c
src/font/lv_font_fmt_txt.c
src/font/lv_font_montserrat_14.c
src/extra/lv_extra.c
src/extra/themes/mono/lv_theme_mono.c
src/extra/themes/basic/lv_theme_basic.c
src/extra/layouts/flex/lv_flex.c
src/extra/widgets/animimg/lv_animimg.c
src/widgets/lv_canvas.c
src/widgets/lv_img.c
src/widgets/lv_label.c
"

SAN_FLAGS=""

# The stub headers are baked into the cached LVGL objects (LV_TICK_CUSTOM pulls
# in zephyr/kernel.h). Wipe the cache when any of them changes.
STUB_SIG=$(find "$SIM/stubs" -type f -exec cat {} + | md5sum | cut -d' ' -f1)
if [ ! -f "$SIM/build/stub_sig" ] || [ "$(cat "$SIM/build/stub_sig" 2>/dev/null)" != "$STUB_SIG" ]; then
    mkdir -p "$SIM/build"
    rm -f "$SIM"/build/lvgl_*.o
    echo "$STUB_SIG" > "$SIM/build/stub_sig"
fi
if [ -n "$SANITIZE" ]; then
    SAN_FLAGS="-fsanitize=address"
fi

LVGL_OBJS=""
for src in $LVGL_SRCS; do
    obj="$SIM/build/lvgl_$(echo "$src" | tr '/' '_').o"
    if [ ! -f "$obj" ] || [ "$LVGL/$src" -nt "$obj" ] || [ "$SIM/sim_config.h" -nt "$obj" ]; then
        gcc -c -g -O1 $SAN_FLAGS -I"$LVGL" -I"$LVGL/src" -I"$SIM/stubs" \
            -include "$SIM/sim_config.h" \
            -o "$obj" "$LVGL/$src" || { echo "FAILED: $src"; exit 1; }
    fi
    LVGL_OBJS="$LVGL_OBJS $obj"
done

gcc -g -O0 -Wall -Wno-unused-function -Wno-unused-variable $SAN_FLAGS \
    -I"$LVGL" \
    -I"$GEM" -I"$GEM/widgets" -I"$GEM/assets" \
    -I"$SIM/stubs" \
    -include "$SIM/sim_config.h" \
    $ROLE \
    "$SIM/$MAIN_SRC" \
    "$SIM/sim_png.c" \
    $(for f in $SRC_WIDGETS $SRC_ASSETS; do echo "$GEM/$f"; done) \
    $LVGL_OBJS \
    -o "$OUT" \
    "$@" -lm

echo "built $OUT"
"$OUT" --out "$SIM/out/$HALF"
