// Simulator stub for the zmk event structs the widgets use.
// The per-event headers in stubs/zmk/events/ define the structs; this header
// just provides the event base type and the accessor the battery widget uses.
#pragma once
#ifndef SIM_ZMK_EVENTS_H
#define SIM_ZMK_EVENTS_H

#include <stdbool.h>
#include <stdint.h>
#include <zmk/endpoints.h>

typedef struct zmk_event_header {
    const void *event;
} zmk_event_t;

// as_* accessor used by widgets/screen.c
static inline const struct zmk_battery_state_changed *
as_zmk_battery_state_changed(const zmk_event_t *eh) {
    (void)eh;
    return NULL;
}

#endif
