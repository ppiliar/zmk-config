// Simulator stub for <zephyr/kernel.h> — native LVGL preview of the gem widgets.
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/sys/slist.h>

// --- atomic ---
typedef long atomic_val_t;
typedef long atomic_t;
static inline atomic_val_t atomic_get(const atomic_t *target) { return *target; }
static inline atomic_val_t atomic_set(atomic_t *target, atomic_val_t value) {
    atomic_val_t old = *target;
    *target = value;
    return old;
}

// --- work queue (immediate synchronous execution in the simulator) ---
struct k_work;
typedef void (*k_work_handler_t)(struct k_work *);

struct k_work {
    k_work_handler_t handler;
};

#define K_WORK_DEFINE(work, handler_fn)                                                            \
    struct k_work work = {.handler = handler_fn};

static inline void k_work_init(struct k_work *work, k_work_handler_t handler) {
    work->handler = handler;
}

// The display work queue: in the simulator, submitted work runs immediately.
static inline void k_work_submit_to_queue_stub(void *q, struct k_work *work) {
    (void)q;
    if (work && work->handler) {
        work->handler(work);
    }
}

#define k_work_submit_to_queue(q, w) k_work_submit_to_queue_stub((q), (w))

// --- sys init ---
// Zephyr's fn signature is `int fn(const struct device *)`; call with NULL.
// (Trailing semicolon from the call site is absorbed by a dummy declaration.)
#define SYS_INIT(fn, level, prio)                                                                  \
    __attribute__((constructor)) static void sim_init_ctor_##fn(void) { (void)fn(NULL); }          \
    static void sim_init_unused_##fn(void)
#define SIM_CONCAT_(a, b) a##b
#define SIM_CONCAT(a, b) SIM_CONCAT_(a, b)

// --- misc ---
#define POST_KERNEL 2
#define K_MSEC(x) ((x) * 1000LL)
// Virtual clock — LVGL's tick comes from here (LV_TICK_CUSTOM=1, as in
// zephyr's modules/lvgl/include/lv_conf.h). The sim advances it explicitly
// via sim_tick_advance() instead of using wall time, so the crystal animation
// renders the same phase on every run and captured frames stay
// byte-comparable. (Defined in the sim main; a frozen tick would stop the
// refresh timer after its first run, so the sim must keep advancing it.)
extern uint32_t sim_tick_ms;
static inline uint32_t k_uptime_get_32(void) { return sim_tick_ms; }
static inline void sim_tick_advance(uint32_t ms) { sim_tick_ms += ms; }
#define IS_ENABLED(config_macro) SIM_Z_IS_ENABLED1(config_macro)
#define SIM_XXXX1 sim_yyyy,
#define SIM_Z_IS_ENABLED1(config_macro) SIM_Z_IS_ENABLED2(SIM_XXXX##config_macro)
#define SIM_Z_IS_ENABLED2(one_or_two_args) SIM_Z_IS_ENABLED3(one_or_two_args 1, 0)
#define SIM_Z_IS_ENABLED3(ignore_this, val, ...) val
