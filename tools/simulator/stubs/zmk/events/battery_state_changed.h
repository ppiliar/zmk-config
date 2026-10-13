#pragma once
#include <zephyr/kernel.h>
#include <zmk/events.h>

struct zmk_battery_state_changed {
    uint8_t state_of_charge;
};
