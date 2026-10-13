#pragma once
#include <zephyr/kernel.h>
#include <zmk/events.h>

struct zmk_usb_conn_state_changed {
    int conn_state;
};
