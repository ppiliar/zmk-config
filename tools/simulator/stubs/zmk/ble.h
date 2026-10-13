// Simulator stub for <zmk/ble.h>
#pragma once
#include <stdbool.h>
#include <stdint.h>

uint8_t zmk_ble_active_profile_index(void);
bool zmk_ble_active_profile_is_connected(void);
bool zmk_ble_active_profile_is_open(void);
