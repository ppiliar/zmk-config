// Simulator stub for <zmk/keymap.h>
#pragma once
#include <stdint.h>
#include <stdbool.h>

uint8_t zmk_keymap_highest_layer_active(void);
const char *zmk_keymap_layer_name(uint8_t layer);
