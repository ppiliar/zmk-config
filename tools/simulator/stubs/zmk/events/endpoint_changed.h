#pragma once
#include <zephyr/kernel.h>
#include <zmk/events.h>

struct zmk_endpoint_changed {
    struct zmk_endpoint_instance endpoint;
};
