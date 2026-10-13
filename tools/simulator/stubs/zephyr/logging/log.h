// Simulator stub for <zephyr/logging/log.h>
#pragma once
#include <stdio.h>

#define LOG_MODULE_DECLARE(name, level) static int sim_log_unused_##name = 0
#define LOG_MODULE_REGISTER(name, level)                                                           \
    static int sim_log_unused_##name = 0
#define LOG_DBG(...)                                                                               \
    do {                                                                                           \
    } while (0)
#define LOG_INF(...)                                                                               \
    do {                                                                                           \
    } while (0)
#define LOG_WRN(...)                                                                               \
    do {                                                                                           \
    } while (0)
#define LOG_ERR(...)                                                                               \
    do {                                                                                           \
    } while (0)
