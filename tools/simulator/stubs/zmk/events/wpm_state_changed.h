#pragma once
#include <zephyr/kernel.h>
#include <zmk/events.h>

struct zmk_wpm_state_changed {
    int state;
};
