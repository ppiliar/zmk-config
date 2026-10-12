#pragma once

#include <stdbool.h>
#include <stdint.h>

// Runtime-switchable light/dark theme for the gem status screens. The flag is
// per-session RAM state only; the boot default comes from
// CONFIG_NICE_VIEW_WIDGET_INVERTED.

// Inverted theme (dark background) is param 1, normal theme (light
// background) is param 2; 0 means toggle and is converted to an absolute
// value by the central before invocation.
#define NVGEM_PARAM_INVERTED 1
#define NVGEM_PARAM_NORMAL 2

bool nvgem_theme_is_inverted(void);

// Sets the theme. Safe to call from any context; flips the flag immediately
// and queues the palette patch + redraw on the display work queue. No-op if
// the theme is already set to the requested value.
void nvgem_theme_set(bool inverted);

// Rewrites the 8-byte palette at the head of an indexed-1bit image map.
// opposite_authored is true for images whose upstream normal palette has
// index 0 = background color swapped (bt_unbonded).
void nvgem_apply_palette(uint8_t *map, bool opposite_authored, bool inverted);

// Re-applies each asset file's palettes for the current theme.
void nvgem_images_apply(void);

// Peripheral-only (crystal frames); weak no-op default on the central.
void nvgem_crystal_apply(void);

// Defined per half in screen.c / screen_peripheral.c; redraws the local
// screen with the cached widget state.
void nvgem_theme_request_redraw(void);
