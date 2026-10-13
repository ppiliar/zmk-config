// Simulator stub for <zmk/endpoints.h> — faithful to endpoints_types.h of zmk v0.3.
#pragma once

#include <stdbool.h>
#include <stdint.h>

enum zmk_transport { ZMK_TRANSPORT_USB, ZMK_TRANSPORT_BLE };

struct zmk_transport_usb_data {};
struct zmk_transport_ble_data {
    int profile_index;
};

struct zmk_endpoint_instance {
    enum zmk_transport transport;
    union {
        struct zmk_transport_usb_data usb;
        struct zmk_transport_ble_data ble;
    };
};

struct zmk_endpoint_instance zmk_endpoints_selected(void);
