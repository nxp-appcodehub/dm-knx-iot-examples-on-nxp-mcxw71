/*
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright (c) 2021-2022 Cascoda Ltd
 Copyright 2024-2025 NXP
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/
#ifndef DOXYGEN
// Force doxygen to document static inline
#define STATIC static
#endif

/**
 * @brief Example device implementing Function Block LSSB
 * @file
 *  Example code for Function Block LSSB
 *  Implements only data point 61: switch on/off
 *  This implementation is a sensor, e.g. transmits data
 *
 * ## Application Design
 *
 * Support functions:
 *
 * - app_init:
 *   - initializes the stack values.
 * - register_resources:
 *   - function that registers all endpoints,
 *   - sets the GET/PUT/POST/DELETE handlers for each end point
 *
 * - main:
 *   - starts the stack, with the registered resources.
 *   - can be compiled out with NO_MAIN
 *
 *  Handlers for the implemented methods (get/post):
 *   - get_[path] :
 *     - function that is being called when a GET is called on [path]
 *     - set the global variables in the output
 *   - post_[path] :
 *     - function that is being called when a POST is called on [path]
 *     - checks the input data
 *     - if input data is correct
 *        - updates the global variables
 *
 * ## Defines
 * stack specific defines:
 * - OC_OSCORE
 *   oscore is enabled as compile flag
 *
 * File specific defines:
 * - NO_MAIN
 *   compile out the function main()
 * - INCLUDE_EXTERNAL
 *   includes header file "external_header.h", so that other tools/dependencies
 *   can be included without changing this code
 */

#include "oc_api.h"
#include "oc_core_res.h"
#include "oc_knx.h"
#include "oc_knx_dev.h"
#include "api/oc_knx_fp.h"
#include "port/oc_clock.h"
#ifdef OC_SPAKE
#include "security/oc_spake2plus.h"
#endif

#include <stdio.h> /* defines FILENAME_MAX */

#include "board.h"
#include "fsl_component_button.h"
#include "fsl_os_abstraction.h"
#include "fsl_debug_console.h"
#include "fsl_component_messaging.h"

#include "thread_connectivity.h"

/* FreeRTOS includes */
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

//TEST MODE DEFINE - use to test communication without shell parameters
//#define TEST_MODE

#define TEST_IA     2    // Test Individual Address
#define TEST_IIA    1    // Test Instalation Identifier

/* KNX task prio is set to let OpenThread initialize before */
#ifndef KNX_TASK_PRIORITY
#define KNX_TASK_PRIORITY 2
#endif

#ifndef KNX_TASK_SIZE
#define KNX_TASK_SIZE ((configSTACK_DEPTH_TYPE)4096 / sizeof(portSTACK_TYPE))
#endif

#define MAX_APP_DATA_SIZE 512

#define MY_NAME "Sensor (LSSB) 421.61" /**< The name of the application */

#define FIRST_URI_PATH "/p/o_1_1"
#define SECOND_URI_PATH "/p/o_1_2"

#define SERIAL_NUMBER "deadbeef0001"

/* Application Events */
#define gAppEvtFromKnx       (1U << 0U)

#if configAPPLICATION_ALLOCATED_HEAP
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

/**
 * @brief Definitions for function pointers related to handling the actions of button presses
 *        For this application, they are defined here to handle button presses for the light switch sensor
 */
typedef void *knx_param_t;
typedef void (*knx_action_t)(const char *);

typedef struct
{
    knx_action_t handler;
    knx_param_t  param;
} knxActionCallback_t;

#if (gAppButtonCnt_c > 0)
extern BUTTON_HANDLE_ARRAY_DEFINE(g_buttonHandle, gAppButtonCnt_c);
#endif

extern void appOtStart(int argc, char *argv[]);
void knx_main(void *context);

static TaskHandle_t sKnxTask = NULL;
OSA_EVENT_HANDLE_DEFINE(mKnxAppEvent);

/* Application input queues */
static messaging_t mKnxAppInputQueue;

/* g_mystate is dependent on the number of buttons available on the board */
bool g_mystate[gAppButtonCnt_c] = {false, false}; /**<  the state of the dpa 421.61 */

/**
 * @brief s-mode response callback
 * will be called when a response is received on an s-mode read request
 *
 * @param url the url
 * @param rep the full response
 * @param rep_value the parsed value of the response
 */
void
oc_add_s_mode_response_cb(char *url, oc_rep_t *rep, oc_rep_t *rep_value)
{
  (void)rep;
  (void)rep_value;

  PRINT("oc_add_s_mode_response_cb %s\r\n", url);
}

/**
 * @brief function to set up the device.
 *
 * sets the:
 * - serial number
 * - base path
 * - knx spec version
 * - hardware version
 * - firmware version
 * - hardware type
 * - device model
 *
 */
int
app_init(void)
{
  /* set the manufacturer name */
  int ret = oc_init_platform("NXP", NULL, NULL);

  /* set the application name, version, base url, device serial number */
  ret |= oc_add_device(MY_NAME, "1.0.0", "//", SERIAL_NUMBER, NULL, NULL);

  oc_device_info_t *device = oc_core_get_device_info(0);
  PRINT("Serial Number: %s\r\n", oc_string_checked(device->serialnumber));

  /* set the hardware version 1.0.0 */
  oc_core_set_device_hwv(0, 1, 0, 0);

  /* set the firmware version 1.0.0 */
  oc_core_set_device_fwv(0, 1, 0, 0);

  /* set the hardware type*/
  oc_core_set_device_hwt(0, "Pi");

  /* set the model */
  oc_core_set_device_model(0, "NXP Sensor");

  /* set the application info*/
  oc_core_set_device_ap(0, 1, 0, 0);

  /* set the manufacturer info*/
  oc_core_set_device_mid(0, 12);

  oc_set_s_mode_response_cb(oc_add_s_mode_response_cb);

#ifdef OC_SPAKE
#define PASSWORD "LETTUCE"
  oc_spake_set_password(PASSWORD);
  PRINT(" SPAKE password %s\r\n", PASSWORD);
#endif

  return ret;
}

/**
 * @brief GET method for "uri_path" resource.
 * function is called to initialize the return values of the GET method.
 * initialization of the returned values are done from the global property
 * values. Resource Description: This Resource describes a binary switch
 * (on/off). The Property "value" is a boolean. A value of 'true' means that the
 * switch is on. A value of 'false' means that the switch is off.
 *
 * @param request the request representation.
 * @param interfaces the interface used for this call
 * @param index index of the boolean "value"
 */
STATIC void get_handler(oc_request_t *request, oc_interface_mask_t interfaces, uint8_t index)
{
  /* TODO: SENSOR add here the code to talk to the HW if one implements a
     sensor. the call to the HW needs to fill in the global variable before it
     returns to this function here. alternative is to have a callback from the
     hardware that sets the global variables.
  */
  bool error_state = false; /* the error state, the generated code */
  int oc_status_code = OC_STATUS_OK;

  PRINT("-- Begin get_dpa_421_61: interface %d\r\n", interfaces);
  /* check if the accept header is CBOR */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
    return;
  }

  // check the query parameter m with the various values
  char *m;
  char *m_key;
  size_t m_key_len;
  size_t m_len = (int)oc_get_query_value(request, "m", &m);
  if (m_len != -1) {
    PRINT("  Query param: %.*s", (int)m_len, m);
    oc_init_query_iterator();
    size_t device_index = request->resource->device;
    oc_device_info_t *device = oc_core_get_device_info(device_index);
    if (device != NULL) {
      oc_rep_begin_root_object();
      while (oc_iterate_query(request, &m_key, &m_key_len, &m, &m_len) != -1) {
        // unique identifier
        if ((strncmp(m, "id", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          char mystring[100];
          snprintf(mystring, 99, "urn:knx:sn:%s%s",
                   oc_string(device->serialnumber),
                   oc_string(request->resource->uri));
          oc_rep_i_set_text_string(root, 0, mystring);
        }
        // resource types
        if ((strncmp(m, "rt", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          oc_rep_set_text_string(root, rt, "urn:knx:dpa.421.6");
        }
        // interfaces
        if ((strncmp(m, "if", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          oc_rep_set_text_string(root, if, "if.s");
        }
        if ((strncmp(m, "dpt", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          oc_rep_set_text_string(root, dpt, oc_string(request->resource->dpt));
        }
        // ga
        if ((strncmp(m, "ga", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          int index = oc_core_find_group_object_table_url(
            oc_string(request->resource->uri));
          if (index > -1) {
            oc_group_object_table_t *got_table_entry =
              oc_core_get_group_object_table_entry(index);
            if (got_table_entry) {
              oc_rep_set_int_array(root, ga, got_table_entry->ga,
                                   got_table_entry->ga_len);
            }
          }
        }
      } /* query iterator */
      oc_rep_end_root_object();
    } else {
      /* device is NULL */
      oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
    }
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  CborError error;
  oc_rep_begin_root_object();
  oc_rep_i_set_boolean(root, 1, g_mystate[index]);
  oc_rep_end_root_object();
  error = g_err;

  if (error) {
    oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\r\n", oc_rep_get_encoded_payload_size());

  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_421_61\r\n");
}

/**
 * @brief GET method for "p/o_1" resource.
 *
 * @param request the request representation.
 * @param interfaces the interface used for this call
 * @param user_data the user data.
 */
STATIC void
get_o_1(oc_request_t *request, oc_interface_mask_t interfaces,
          void *user_data)
{
    (void)user_data; /* variable not used */
    get_handler(request, interfaces, 0);
}

/**
 * @brief GET method for "p/o_2" resource.
 *
 * @param request the request representation.
 * @param interfaces the interface used for this call
 * @param user_data the user data.
 */
STATIC void
get_o_2(oc_request_t *request, oc_interface_mask_t interfaces,
          void *user_data)
{
    (void)user_data; /* variable not used */
    get_handler(request, interfaces, 1);
}

/**
 * @brief register all the resources to the stack
 * this function registers all application level resources:
 * - each resource path is bind to a specific function for the supported methods
 * (GET, POST, PUT, DELETE)
 * - each resource is
 *   - secure
 *   - observable
 *   - discoverable
 *   - used interfaces
 *
 */

void register_resource_by_uri_path(const char* uri_path, oc_request_callback_t callback)
{
  PRINT("Register Resource with local path \"%s\"\r\n", uri_path);
  PRINT("Light Switching Sensor 421.61 (LSSB) : SwitchOnOff \r\n");
  PRINT("Data point 61 (DPT_Switch) \r\n");
  PRINT("Register Resource with local path \"%s\"\r\n", uri_path);

  oc_resource_t *res_pushbutton =
    oc_new_resource("push button", uri_path, 2, 0);
  oc_resource_bind_resource_type(res_pushbutton, "urn:knx:dpa.421.61");
  oc_resource_bind_dpt(res_pushbutton, "urn:knx:dpt.switch");
  oc_resource_bind_content_type(res_pushbutton, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_pushbutton, OC_IF_S); /* if.s */
  oc_resource_set_function_block_instance(res_pushbutton, 1);
  oc_resource_set_discoverable(res_pushbutton, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  // oc_resource_set_periodic_observable(res_pushbutton, 1);
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  oc_resource_set_observable(res_pushbutton, true);
  oc_resource_set_request_handler(res_pushbutton, OC_GET, callback, NULL);
  oc_add_resource(res_pushbutton);
}

/* 
 * URL Table
 * | resource url |  functional block/dpa  | GET | PUT |
 * | ------------ | ---------------------- | ----| ----|
 * | p/o_1        | urn:knx:dpa.421.61     | Yes | No  |
 * | p/o_2        | urn:knx:dpa.421.61     | Yes | No  |
 */

void
register_resources(void)
{
  register_resource_by_uri_path(FIRST_URI_PATH, get_o_1);
  register_resource_by_uri_path(SECOND_URI_PATH, get_o_2);
}

/**
 * @brief initiate preset for device
 * current implementation: device reset as command line argument
 * @param device_index the device identifier of the list of devices
 * @param data the supplied data.
 */
void
factory_presets_cb(size_t device_index, void *data)
{
  (void)device_index;
  (void)data;

  PRINT("factory_presets_cb: resetting device\r\n");
}

/**
 * @brief application reset
 *
 * @param device_index the device identifier of the list of devices
 * @param reset_value the knx reset value
 * @param data the supplied data.
 */
void
reset_cb(size_t device_index, int reset_value, void *data)
{
  (void)device_index;
  (void)data;

  PRINT("reset_cb %d\r\n", reset_value);
}

/**
 * @brief restart the device (application depended)
 *
 * @param device_index the device identifier of the list of devices
 * @param data the supplied data.
 */
void
restart_cb(size_t device_index, void *data)
{
  (void)device_index;
  (void)data;

  PRINT("-----restart_cb -------\r\n");
  // exit(0);
}

/**
 * @brief set the host name on the device (application depended)
 *
 * @param device_index the device identifier of the list of devices
 * @param host_name the host name to be set on the device
 * @param data the supplied data.
 */
void
hostname_cb(size_t device_index, oc_string_t host_name, void *data)
{
  (void)device_index;
  (void)data;

  PRINT("-----host name ------- %s\r\n", oc_string_checked(host_name));
}

static oc_event_callback_retval_t
send_delayed_response(void *context)
{
  oc_separate_response_t *response = (oc_separate_response_t *)context;

  if (response->active) {
    oc_set_separate_response_buffer(response);
    oc_send_separate_response(response, OC_STATUS_CHANGED);
    PRINT("Delayed response sent\r\n");
  } else {
    PRINT("Delayed response NOT active\r\n");
  }

  return OC_EVENT_DONE;
}

/**
 * @brief initializes the global variables
 * registers and starts the handler
 */
void
initialize_variables(void)
{
  /* initialize global variables for resources */
  /* if wanted read them from persistent storage */
}

#ifdef TEST_MODE
void test_knx_got_add(void)
{
    oc_group_object_table_t entry;
    int index = 0;
    int temp_ga[1]= {1};
    int id = 0;

    index = find_empty_slot_in_group_object_table(id);

    if (oc_core_find_group_object_table_number_group_entries(index))
    {
        PRINT("Id alread in use!\r\n");
        return;
    }

    entry.id = id;
    oc_new_string(&entry.href, FIRST_URI_PATH, strlen(FIRST_URI_PATH));
    entry.cflags = 20;

    entry.ga_len = 1;
    entry.ga = temp_ga;

    oc_core_set_group_object_table(index, entry);
    oc_dump_group_object_table_entry(index);
    oc_register_group_multicasts();

    oc_free_string(&entry.href);
}

void test_knx_iid(void)
{
    oc_core_set_device_iid(0, TEST_IIA);
}

void test_knx_ia(void)
{
    oc_core_set_device_ia(0, TEST_IA);
}
#endif //TEST_MODE

void knx_action_button_press(const char *uri_path)
{
    if(oc_is_device_in_runtime(0) == true)
	{
        oc_do_s_mode_with_scope_no_check(2, uri_path, "w");
        oc_do_s_mode_with_scope_no_check(5, uri_path, "w");
	}
	else
	{
        /* KNX application is not in loaded mode yet and device is not connected to thread network */
        PRINT("KNX app is not running or parameters are not set, ignoring button pressing\r\n");
	}
}

#if (gAppButtonCnt_c > 0)
/**
 * @brief Creates a KNX action message from the buttons to be sent to the KNX app input queue
 * @return true in case of memory allocation error and false if operation was successful
 */
bool sendKnxActionMessage(const char* uri_path, uint8_t index)
{
    knxActionCallback_t *pMsgIn = NULL;

    /* Allocate a buffer with enough space to store the packet */
    pMsgIn = MSG_Alloc(sizeof(knxActionCallback_t));

    if (pMsgIn == NULL)
    {
        return true;
    }

    g_mystate[index] = !g_mystate[index];
    pMsgIn->handler = knx_action_button_press;
    pMsgIn->param = calloc(1, strlen(uri_path) + 1);
    memcpy(pMsgIn->param, uri_path, strlen(uri_path));

    /* Put message in the Cb App queue */
    (void)MSG_QueueAddTail(&mKnxAppInputQueue, pMsgIn);

    if(oc_a_lsm_state(0) == LSM_S_LOADED)
    {
        (void)OSA_EventSet((osa_event_handle_t) mKnxAppEvent, gAppEvtFromKnx);
    }
    return false;
}

button_status_t Btn_HandleKeys0(void *buttonHandle, button_callback_message_t *message, void *callbackParam)
{
    switch (message->event)
    {
        case kBUTTON_EventOneClick:
        case kBUTTON_EventShortPress:
            if(sendKnxActionMessage(FIRST_URI_PATH, 0))
            {
                return kStatus_BUTTON_Error;
            }
            break;
        case kBUTTON_EventLongPress:
            break;

        default:
            break;
    }

    return kStatus_BUTTON_Success;
}

#if (gAppButtonCnt_c > 1)
button_status_t Btn_HandleKeys1(void *buttonHandle, button_callback_message_t *message, void *callbackParam)
{
    switch (message->event)
    {
        case kBUTTON_EventOneClick:
        case kBUTTON_EventShortPress:
            if(sendKnxActionMessage(SECOND_URI_PATH, 1))
            {
                return kStatus_BUTTON_Error;
            }
            break;
        case kBUTTON_EventLongPress:
            break;

        default:
            break;
    }

    return kStatus_BUTTON_Success;
}
#endif /*gAppButtonCnt_c > 1*/
#endif /*gAppButtonCnt_c > 0*/

void knx_appSpecificInit(void)
{
    /* Button Pins init is done in function APP_InitServices from app_services_init.c */
#if (gAppButtonCnt_c > 0)
    BUTTON_InstallCallback((button_handle_t)g_buttonHandle[0], Btn_HandleKeys0, NULL);
#if (gAppButtonCnt_c > 1)
    BUTTON_InstallCallback((button_handle_t)g_buttonHandle[1], Btn_HandleKeys1, NULL);
#endif /*gAppButtonCnt_c > 1*/
#endif /*gAppButtonCnt_c > 0*/
}

/**
 *  @brief send a multicast s-mode message
 */
static void
issue_requests_s_mode(void)
{
  PRINT("issue_requests_s_mode: Demo \n\r\n");

  oc_do_s_mode_with_scope(2, FIRST_URI_PATH, "w");
  oc_do_s_mode_with_scope(5, FIRST_URI_PATH, "w");
}

void knx_check_app_message_queue(void)
{
    /* Check for existing messages in queue */
    if (MSG_QueueGetHead(&mKnxAppInputQueue) != NULL)
    {
        /* Pointer for storing the messages from host. */
        knxActionCallback_t *pMsgIn = MSG_QueueRemoveHead(&mKnxAppInputQueue);

        if (pMsgIn != NULL)
        {
            /* Process it */
            pMsgIn->handler((const char*) pMsgIn->param);
            free((void *) pMsgIn->param);

            /* Messages must always be freed. */
            (void)MSG_Free(pMsgIn);
        }
    }
}

#ifndef NO_MAIN
/**
 * @brief signal the event loop
 * wakes up the main function to handle the next callback
 */
STATIC void
signal_event_loop(void)
{
    (void)OSA_EventSet((osa_event_handle_t) mKnxAppEvent, gAppEvtFromKnx);
}

int appKnxStart(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    xTaskCreate(knx_main, "knx", KNX_TASK_SIZE, NULL, KNX_TASK_PRIORITY, &sKnxTask);
    return 0;
}

int main(int argc, char *argv[])
{
    appOtStart(argc, argv);
    appKnxStart(argc, argv);

    vTaskStartScheduler();

    return 0;
}

/**
 * @brief KNX main application.
 * initializes the global variables
 * registers and starts the handler
 * handles (in a loop) the next event.
 * shuts down the stack
 */
void
knx_main(void *context)
{
  int init;
  oc_clock_time_t next_event;
  bool do_send_s_mode = false;
  uint32_t event = 0U;
  otInstance * instance = NULL;

  PRINT("KNX-IOT Server name : \"%s\"", MY_NAME);

  /*
   The storage folder depends on the build system
   the folder is created in the makefile, with $target as name with _cred as
   post fix.
  */
  PRINT("\tstorage at './LSSB_minimal_all_creds' \r\n");
  oc_storage_config("./LSSB_minimal_all_creds");

  oc_set_max_app_data_size(MAX_APP_DATA_SIZE);

  /*initialize the variables */
  initialize_variables();

  /* initializes the handlers structure */
  STATIC oc_handler_t handler = { .init = app_init,
                                  .signal_event_loop = signal_event_loop,
                                  .register_resources = register_resources,
                                  .requests_entry = NULL };

  if (do_send_s_mode) {

    handler.requests_entry = issue_requests_s_mode;
  }

  /* Prepare application input queue.*/
  MSG_QueueInit(&mKnxAppInputQueue);

  /* set the application callbacks */
  oc_set_hostname_cb(hostname_cb, NULL);
  oc_set_reset_cb(reset_cb, NULL);
  oc_set_restart_cb(restart_cb, NULL);
  oc_set_factory_presets_cb(factory_presets_cb, NULL);

  /* start the stack */
  init = oc_main_init(&handler);

  instance = otGetOtInstance();

  if(instance != NULL)
  {
    while(otThreadGetDeviceRole(instance) < OT_DEVICE_ROLE_CHILD)
    {
      knx_check_app_message_queue();
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
  else
  {
    goto exit;
  }

  oc_a_lsm_set_state(0, LSM_S_LOADED);

  if (init < 0) {
    PRINT("oc_main_init failed %d, exiting.\r\n", init);
    return;
  }

#ifdef OC_OSCORE
  PRINT("OSCORE - Enabled\r\n");
#else
  PRINT("OSCORE - Disabled\r\n");
#endif /* OC_OSCORE */

  oc_device_info_t *device = oc_core_get_device_info(0);

  PRINT("serial number: %s\r\n", oc_string_checked(device->serialnumber));

  oc_endpoint_t *my_ep = oc_connectivity_get_endpoints(0);
  if (my_ep != NULL) {
    PRINTipaddr(*my_ep);
    PRINT("\r\n");
  }

  PRINT("Connected to Thread network\r\n");
  PRINT("Server \"%s\" running, waiting on incoming connections.\r\n", MY_NAME);

  /* Create application event */
  (void)OSA_EventCreate(mKnxAppEvent, 1U);
  assert(NULL != mKnxAppEvent);

#ifdef TEST_MODE
  test_knx_iid();
  test_knx_ia();
  test_knx_got_add();
#endif

  while(1)
  {
    knx_check_app_message_queue();
    next_event = oc_main_poll();

    if (next_event == 0)
    {
        (void)OSA_EventWait(mKnxAppEvent, osaEventFlagsAll_c, 0U, osaWaitForever_c , &event);
    }
    else
    {
        (void)OSA_EventWait(mKnxAppEvent, osaEventFlagsAll_c, 0U, next_event - oc_clock_time(), &event);
    }
  }

exit:
  oc_main_shutdown();
  return;
}
#endif /* NO_MAIN */
