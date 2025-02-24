/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <oc_endpoint.h>
#include <oc_connectivity.h>
#include <oc_log.h>
#include <oc_buffer.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "fsl_os_abstraction.h"

#include "openthread-system.h"
#include "ot_platform_common.h"

#include <openthread-core-config.h>
#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/error.h>
#include <openthread/udp.h>
#include <openthread/tasklet.h>

#include "knx_shell.h"

#include <assert.h>
#include <string.h>


#ifndef OT_MAIN_TASK_PRIORITY
#define OT_MAIN_TASK_PRIORITY 5
#endif

#ifndef OT_MAIN_TASK_SIZE
#define OT_MAIN_TASK_SIZE ((configSTACK_DEPTH_TYPE)4*1024 / sizeof(portSTACK_TYPE))
#endif

#define COAP_UNSECURED_PORT 5683

static otInstance       *sInstance      = NULL;
static TaskHandle_t      sMainTask      = NULL;
static SemaphoreHandle_t sMainStackLock = NULL;

static otUdpSocket mSocket;
static OSA_MUTEX_HANDLE_DEFINE(mThreadMutexId);

// Task handles
static TaskHandle_t xOpenThreadTaskHandle = NULL;

extern void otAppCliInit(otInstance *aInstance);
extern void otSysRunIdleTask(void);
extern void knx_appSpecificInit(void);

void appOtLockOtTask(bool bLockState);

static void appOtInit()
{
    otSysInit(0, NULL);

    knx_appSpecificInit();

    sInstance = otInstanceInitSingle();
    
    assert(sInstance);

    /* Init the CLI */
    otAppCliInit(sInstance);

    /* Add KNX specific shell commands */
    otCliKNXSetUserCommands();
}

static void mainloop(void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);
    appOtInit();
    
    otSysProcessDrivers(sInstance);
    while (!otSysPseudoResetWasRequested())
    {
        /* Aqquired the task mutex lock and release after OT processing is done */
        appOtLockOtTask(true);
        otTaskletsProcess(sInstance);
        otSysProcessDrivers(sInstance);
        appOtLockOtTask(false);

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    otInstanceFinalize(sInstance);
    vTaskDelete(NULL);
}

void appOtLockOtTask(bool bLockState)
{
    if (bLockState)
    {
        /* Aqquired the task mutex lock */
        xSemaphoreTakeRecursive(sMainStackLock, portMAX_DELAY);
    }
    else
    {
        /* Release the task mutex lock */
        xSemaphoreGiveRecursive(sMainStackLock);
    }
}

void appOtStart(int argc, char *argv[])
{
    // Store the parent task handle
    xOpenThreadTaskHandle = xTaskGetCurrentTaskHandle();

    sMainStackLock = xSemaphoreCreateRecursiveMutex();
    assert(sMainStackLock != NULL);

    xTaskCreate(mainloop, "ot", OT_MAIN_TASK_SIZE, NULL, OT_MAIN_TASK_PRIORITY, &sMainTask);
}

void otTaskletsSignalPending(otInstance *aInstance)
{
    (void)aInstance;

    if (sMainTask != NULL)
    {
        xTaskNotifyGive(sMainTask);
    }
}

void otSysEventSignalPending(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (sMainTask != NULL)
    {
        vTaskNotifyGiveFromISR(sMainTask, &xHigherPriorityTaskWoken);
        /* Context switch needed? */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void otSysRunIdleTask(void)
{
    otPlatSaveSettingsIdle();

    /* Apply reset if it was requested, better to do that after filesystem writes */
    otPlatResetIdle();
}

otInstance *otGetOtInstance(void)
{
	return sInstance;
}

/* KNX-IoT OpenThread integration */

void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    oc_message_t *message = oc_allocate_message();
    (void)aContext;

    if (!message)
    {
        PRINT("Failed to allocate OC message\r\n");
        return;
    }

    message->length = otMessageRead(aMessage, otMessageGetOffset(aMessage), message->data, otMessageGetLength(aMessage));
    message->endpoint.flags = IPV6;
    message->endpoint.device = 0;
    message->endpoint.addr.ipv6.port = aMessageInfo->mPeerPort;
    memcpy(message->endpoint.addr.ipv6.address, aMessageInfo->mPeerAddr.mFields.m8, 16);

    PRINT("Incoming message of size %zd bytes from ", message->length);
    PRINTipaddr(message->endpoint);
    PRINT("\r\n");

    oc_network_event(message);
}

int oc_connectivity_init(size_t device)
{
    otError           error = OT_ERROR_NONE;
    otSockAddr        sockaddr = {0};
    otNetifIdentifier netif = OT_NETIF_THREAD;

    sockaddr.mPort = COAP_UNSECURED_PORT;

    if (sInstance != NULL)
    {
        if (!otUdpIsOpen(sInstance, &mSocket))
        {
            error = otUdpOpen(sInstance, &mSocket, HandleUdpReceive, NULL);

            if (error == OT_ERROR_NONE)
            {
                error = otUdpBind(sInstance, &mSocket, &sockaddr, netif);
                if (error != OT_ERROR_NONE)
                {
                    PRINT("otUdpBind failed with %u\r\n", error);
                }
            }
            else
            {
                PRINT("otUdpOpen failed with %u\r\n", error);
            }
        }
        else
        {
            PRINT("Socket already open!\r\n");
        }
    }
    else
    {
        PRINT("OT instance not initialized!\r\n");
    }
    
    return 1;
}

int
oc_send_buffer(oc_message_t *message)
{
    otError           error   = OT_ERROR_NONE;
    otMessage        *otMessage = NULL;
    otMessageInfo     messageInfo;
    otMessageSettings messageSettings = {true, OT_MESSAGE_PRIORITY_NORMAL};

    if(!otUdpIsOpen(sInstance, &mSocket))
    {
        return 1;
    } 

    memset(&messageInfo, 0, sizeof(messageInfo));
    memcpy(messageInfo.mPeerAddr.mFields.m8, message->endpoint.addr.ipv6.address,
           sizeof(messageInfo.mPeerAddr.mFields.m8));

    messageInfo.mSockPort = mSocket.mSockName.mPort;
    messageInfo.mPeerPort = message->endpoint.addr.ipv6.port;

    otMessage = otUdpNewMessage(sInstance, &messageSettings);

    if (otMessage == NULL)
    {
        PRINT("Failed to allocate UDP message\r\n");
        goto exit;
    }

    error = otMessageAppend(otMessage, message->data, message->length);

    if (error != OT_ERROR_NONE)
    {
        PRINT("Failed to append message\r\n");
        goto exit;
    }

    error = otUdpSend(sInstance, &mSocket, otMessage, &messageInfo);

    if (error != OT_ERROR_NONE)
    {
        PRINT("otUdpSend failed with %u\r\n", error);
        goto exit;
    }

    otMessage = NULL;
 
    PRINT("Sent UDP message\r\n");
exit:
    if (otMessage != NULL)
    {
        otMessageFree(otMessage);
    }

    return 0;
}

void
oc_send_discovery_request(oc_message_t *message)
{
    PRINT("Sending discovery reqyest\r\n");
    oc_send_buffer(message);
}

oc_endpoint_t *oc_connectivity_get_endpoints(size_t device)
{
    return NULL;
}

void
oc_connectivity_shutdown(size_t device)
{
    otUdpClose(sInstance, &mSocket);
}

void
oc_network_event_handler_mutex_init(void)
{
  (void)OSA_MutexCreate(mThreadMutexId);
  assert(NULL != mThreadMutexId);
}

void
oc_network_event_handler_mutex_lock(void)
{
    (void)OSA_MutexLock((osa_mutex_handle_t) mThreadMutexId, osaWaitForever_c);
}

void
oc_network_event_handler_mutex_unlock(void)
{
    (void)OSA_MutexUnlock((osa_mutex_handle_t) mThreadMutexId);
}

void
oc_network_event_handler_mutex_destroy(void)
{
    (void)OSA_MutexDestroy((osa_mutex_handle_t) mThreadMutexId);
}

void
oc_connectivity_subscribe_mcast_ipv6(oc_endpoint_t *address)
{
    if (sInstance != NULL)
    {
        otIp6SubscribeMulticastAddress(sInstance, (const otIp6Address *) address->addr.ipv6.address);
    }
}
