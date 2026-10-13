#pragma once
#include <zephyr/kernel.h>
#include <zmk/events.h>

struct zmk_layer_state_changed {
    uint8_t layer;
    bool state;
    int64_t timestamp;
};
