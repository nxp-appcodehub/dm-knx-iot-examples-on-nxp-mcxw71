/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* KNX commands added through the OpenThread Shell framework */
#include "oc_main.h"
#include "oc_knx_dev.h"
#include "oc_knx_fp.h"
#include "oc_core_res.h"

#include "common/code_utils.hpp"
#include <openthread/cli.h>
#include <openthread/platform/misc.h>

#include "thread_connectivity.h"
#include "fsl_debug_console.h"

#define TX_BUFFER_SIZE 256 /* Length of the send buffer */
#define EOL_CHARS "\r\n"   /* End of Line Characters */
#define EOL_CHARS_LEN 2    /* Length of EOL */

/* static variables */
static char sTxBuffer[TX_BUFFER_SIZE + 1]; /* Transmit Buffer */

static otError knx_got(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        error = OT_ERROR_INVALID_ARGS;
        goto exit;
    }

    if (!strcmp("add", aArgs[0]))
    {
        int index = 0;
        oc_group_object_table_t entry = {0};
        oc_string_t href;
        uint32_t *ga_values = NULL;

        /* Check that command has minimum number of parameters and "ga" token is in place */
        if ((aArgsLength < 6) && (strcmp("ga", aArgs[4])))
        {
            PRINT("knx_got add *id* *uri_path* *cflags* ga *group_addresses*\r\n");
            error = OT_ERROR_INVALID_ARGS;
            goto exit;
        }

        if (oc_is_device_in_runtime(0) == false)
        {
            PRINT("Not in loaded state or need to set IA and/or IID before.\r\n");
            error = OT_ERROR_INVALID_STATE;
            goto exit;
        }

        /* knx_got add *id* *uri_path* *cflags* ga *group_addresses* */
        entry.id = atoi(aArgs[1]);

        if(oc_core_find_index_in_group_object_table_from_id(entry.id) != -1)
        {
            PRINT("Group Object Table ID already in use!\r\n");
            error = OT_ERROR_INVALID_ARGS;
            goto exit;
        }
  
        index = find_empty_slot_in_group_object_table(entry.id);

        if (oc_core_find_group_object_table_number_group_entries(index))
        {
            PRINT("Index already in use!\r\n");
            error = OT_ERROR_INVALID_ARGS;
            goto exit;
        }

        /* Create string for URI path */
        oc_new_string(&href, aArgs[2], strlen(aArgs[2]));

        /* Add CFLAGS */
        entry.cflags = atoi(aArgs[3]);

        /* Size of ga array is equal to subtracting from the command's number
         * of parameters the number of already used parameters
         * aArgs[4] needs to be set to "ga" string as a indicator
         * where the group addresses are entered */
        uint8_t ga_len = aArgsLength - 5;

        ga_values = (uint32_t *)malloc(ga_len * sizeof(uint32_t));
        /* Add group addresses */
        for (int i = 0; i < ga_len; i++)
        {
            ga_values[i] = atoi(aArgs[5 + i]);
        }

        entry.ga_len = ga_len;

        entry.href = href;
        entry.ga = ga_values;

        oc_core_set_group_object_table(index, entry);
        oc_dump_group_object_table_entry(index);
        oc_register_group_multicasts();

        /* Free the allocated memory */
        free((void *)ga_values);
        oc_free_string(&href);
    }
    else if (!strcmp("show", aArgs[0]))
    {
        PRINT("Showing Group Object Table\n");
        for (int i = 0; i < GOT_MAX_ENTRIES; i++)
        {
            oc_print_group_object_table_entry(i);
        }
    }
    else if (!strcmp("remove", aArgs[0]))
    {
        int index;

        if (aArgsLength < 2)
        {
            PRINT("knx_got remove *id*\r\n");
            error = OT_ERROR_INVALID_ARGS;
            goto exit;
        }

        index = oc_core_find_index_in_group_object_table_from_id(atoi(aArgs[1]));

        if(index != -1)
        {
            oc_delete_group_object_table_entry(index);
        }
        else
        {
            PRINT("Group Object Table ID not found!\r\n");
            error = OT_ERROR_INVALID_ARGS;
            goto exit;
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

static otError knx_ia(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        oc_device_info_t *device = oc_core_get_device_info(0);
        PRINT("Device individual address: %lu\r\n", device->ia);
    }
    else if (aArgsLength == 1)
    {
        oc_core_set_and_store_device_ia(0, atoi(aArgs[0]));
    }
    else
    {
        PRINT("'knx_ia' outputs device individual address, 'knx_ia *addr*' sets device individual address\r\n");
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

static otError knx_iid(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        PRINT("Device installation id: %ld\r\n", (uint32_t)oc_core_get_device_iid(0));
    }
    else if (aArgsLength == 1)
    {
        oc_core_set_and_store_device_iid(0, atol(aArgs[0]));
    }
    else
    {
        otCliOutputFormat("'knx_iid' outputs device installation id, 'knx_iid *iid*' sets device installation id\r\n");
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

static otError knx_fid(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        oc_device_info_t *pDevInfo = oc_core_get_device_info(0);
        PRINT("Fabric identifier: %ld\r\n", (uint32_t)pDevInfo->fid);
    }
    else if (aArgsLength == 1)
    {
        uint64_t temp = atol(aArgs[0]);
        oc_core_set_device_fid(0, temp);
        oc_storage_write(KNX_STORAGE_FID, (uint8_t *)&temp, sizeof(temp));
    }
    else
    {
        PRINT("'knx_fid' outputs fabric identifier, 'knx_fid *fid*' sets fabric identifier\r\n");
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

static otError knx_factoryreset(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    /* Call KNX storage reset for device 0 (this device) and code 2 (Factory Reset to default state) */
    oc_knx_device_storage_reset(0, 2);

    /* Call OpenThread Factory Reset API that erases persistent OT stack data and resets the board */
    otInstanceFactoryReset(otGetOtInstance());

    return OT_ERROR_NONE;
}

static otError knx_pm(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        PRINT("Device in programming mode: %s", oc_knx_device_in_programming_mode(0) ? "TRUE" : "FALSE");
    }
    else if (aArgsLength == 1)
    {
        int mode = atoi(aArgs[0]);
        if (mode == 1)
        {
            oc_knx_device_set_programming_mode(0, true);
        }
        else if (mode == 0)
        {
            oc_knx_device_set_programming_mode(0, false);
        }
        else
        {
            PRINT("Invalid argument\r\n");
            error = OT_ERROR_INVALID_ARGS;
        }
    }
    else
    {
        PRINT("'knx_pm' outputs programming mode status, 'knx_pm 1' sets mode true, 'knx_pm 0' sets mode false\r\n");
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

static const otCliCommand sExtensionCommands[] = {
    {"knx_got", knx_got},
    {"knx_ia", knx_ia},
    {"knx_iid", knx_iid},
    {"knx_fid", knx_fid},
    {"knx_factoryreset", knx_factoryreset},
    {"knx_pm", knx_pm},
};

void otCliKNXSetUserCommands(void)
{
    IgnoreError(otCliSetUserCommands(sExtensionCommands, OT_ARRAY_LENGTH(sExtensionCommands), NULL));
}

void formated_print(const char *aFormat, va_list ap)
{
    int len = 0;
    memset(sTxBuffer, 0, TX_BUFFER_SIZE + 1);
    len = vsnprintf(sTxBuffer, TX_BUFFER_SIZE - EOL_CHARS_LEN, aFormat, ap);
    //otEXPECT(len >= 0);
    memcpy(sTxBuffer + len, EOL_CHARS, EOL_CHARS_LEN);
    len += EOL_CHARS_LEN;

/* gDebugConsoleEnable_d=1 enables usage of UART0 on MCXW71 while keeping it to 0
   keeps the logging output to same interface as OpenThread CLI */
#if (gDebugConsoleEnable_d == 0)
    otCliOutputFormat(sTxBuffer);
#else
    PRINTF(sTxBuffer);
#endif
}

int __wrap_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    formated_print(format, args);

    va_end(args);
    return 0;
}

int __wrap_puts(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    formated_print(format, args);

    va_end(args);
    return 0;
}
