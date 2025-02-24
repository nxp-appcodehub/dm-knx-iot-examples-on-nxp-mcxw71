/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <oc_clock.h>
#include "ot_platform_common.h"
#include "openthread/platform/time.h"
#include "openthread/platform/alarm-milli.h"

void
oc_clock_init(void)
{
	otPlatAlarmInit();
}

oc_clock_time_t
oc_clock_time(void)
{
  return otPlatAlarmMilliGetNow();
}

unsigned long
oc_clock_seconds(void)
{
  return otPlatTimeGet();
}