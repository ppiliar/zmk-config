#pragma once
#include <zephyr/kernel.h>
#include <zmk/events.h>

struct zmk_split_peripheral_status_changed {
    bool connected;
};
