/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "port/oc_random.h"
#include <openthread/platform/entropy.h>
#include "platform-k32w1.h"
#include <string.h>

void
oc_random_init(void)
{
	K32WRandomInit();
}

unsigned int
oc_random_value(void)
{
    unsigned int random_value = 0;
    uint8_t random_bytes[4] = {0};
    otPlatEntropyGet(random_bytes, sizeof(random_bytes));
    memcpy(&random_value, &random_bytes[0], sizeof(random_bytes));
    return random_value;
}

void oc_random_destroy(void)
{

}
