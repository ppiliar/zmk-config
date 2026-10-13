#pragma once
#include <zephyr/kernel.h>
#include <zmk/events.h>

struct zmk_ble_active_profile_changed {
    uint8_t index;
    void *profile;
};
