/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OC_CONFIG_H
#define OC_CONFIG_H

#include <stdint.h>

typedef uint64_t oc_clock_time_t;

#define OC_CLOCK_CONF_TICKS_PER_SECOND 1000
#define OC_MAX_NUM_DEVICES (1)
#define OC_MAX_APP_RESOURCES (4)
#define OC_MAX_NUM_CONCURRENT_REQUESTS (5)
#define OC_MAX_NUM_REP_OBJECTS (150)
#define OC_MAX_OBSERVE_SIZE 512
#define OC_MAX_NUM_ENDPOINTS (20)
#define OC_REQUEST_HISTORY
#define OC_DYNAMIC_ALLOCATION

#define OC_BLOCK_WISE

#endif
