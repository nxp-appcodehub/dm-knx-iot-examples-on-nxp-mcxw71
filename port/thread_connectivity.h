/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef THREAD_CONNECTIVITY_H
#define THREAD_CONNECTIVITY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function returns the OpenThread instance.
 * @return pointer to otInstance
 *
 */
otInstance * otGetOtInstance(void);

#ifdef __cplusplus
}
#endif

#endif // THREAD_CONNECTIVITY_H