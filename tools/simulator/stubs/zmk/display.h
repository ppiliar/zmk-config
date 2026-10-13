// Simulator stub for <zmk/display.h> — the real macro from zmk/app/include/zmk/display.h,
// with the work submission routed to the simulator's immediate-execution queue and
// listener registration done via constructor attributes (ZMK uses linker sections).
#pragma once

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zmk/events.h>

struct k_work_q;
struct k_work_q *zmk_display_work_q(void);

// Provided by sim_main.c
typedef int (*sim_listener_cb_t)(const zmk_event_t *eh);
extern sim_listener_cb_t sim_listener_registry[];
extern int sim_listener_count;

#define SIM_ZMK_EV_EVENT_BUBBLE 1

bool zmk_display_is_initialized(void);

// Faithful to the real macro except: no mutex (single-threaded sim) and
// registration via constructor instead of linker section.
#define ZMK_DISPLAY_WIDGET_LISTENER(listener, state_type, cb, state_func)                          \
    static state_type __##listener##_state;                                                        \
    static state_type listener##_get_local_state() { return __##listener##_state; };               \
    static void listener##_work_cb(struct k_work *work) {                                          \
        (void)work;                                                                                \
        cb(listener##_get_local_state());                                                          \
    };                                                                                             \
    K_WORK_DEFINE(listener##_work, listener##_work_cb);                                            \
    static void listener##_refresh_state(const zmk_event_t *eh) {                                  \
        (void)eh;                                                                                  \
        __##listener##_state = state_func(eh);                                                     \
    };                                                                                             \
    static void listener##_init() {                                                                \
        listener##_refresh_state(NULL);                                                            \
        listener##_work_cb(NULL);                                                                  \
    }                                                                                              \
    static int listener##_cb(const zmk_event_t *eh) {                                              \
        if (zmk_display_is_initialized()) {                                                        \
            listener##_refresh_state(eh);                                                          \
            k_work_submit_to_queue(zmk_display_work_q(), &listener##_work);                        \
        }                                                                                          \
        return SIM_ZMK_EV_EVENT_BUBBLE;                                                            \
    }                                                                                              \
    __attribute__((constructor)) static void listener##_register(void) {                           \
        sim_listener_registry[sim_listener_count++] = listener##_cb;                               \
    }

#define ZMK_SUBSCRIPTION(mod, ev_type)                                                             \
    static void *sim_sub_unused_##mod##_##ev_type
