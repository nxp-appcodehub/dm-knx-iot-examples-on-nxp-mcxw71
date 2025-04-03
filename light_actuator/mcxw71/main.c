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
 * @brief Example device implementing Function Block LSAB
 * @file
 *  Example code for Function Block LSAB.
 *  Implements data point 61: switch on/off.
 *  This implementation is a actuator, e.g. receives data and acts on it.
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
#include "board_comp.h"
#include "fsl_os_abstraction.h"
#include "fsl_debug_console.h"
#include "fsl_component_led.h"

#include "knx_pin_mux.h"
#include "thread_connectivity.h"

/* FreeRTOS includes */
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

//TEST MODE DEFINE - use to test communication without shell parameters
//#define TEST_MODE

#define TEST_IA     1    // Test Individual Address
#define TEST_IIA    1    // Test Instalation Identifier

/* KNX task prio is set to let OpenThread initialize before */
#ifndef KNX_TASK_PRIORITY
#define KNX_TASK_PRIORITY 2
#endif

#ifndef KNX_TASK_SIZE
#define KNX_TASK_SIZE ((configSTACK_DEPTH_TYPE)4096 / sizeof(portSTACK_TYPE))
#endif

#define MAX_APP_DATA_SIZE 512

#define MY_NAME "Actuator (LSAB) 417" /**< The name of the application */

#define FIRST_URI_PATH "/p/o_1_1"
#define SECOND_URI_PATH "/p/o_1_2"

#define SERIAL_NUMBER "deadbeef0002"

/* Application Events */
#define gAppEvtFromKnx       (1U << 0U)

#if configAPPLICATION_ALLOCATED_HEAP
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

extern void appOtStart(int argc, char *argv[]);
void knx_main(void *context);
void knx_led_toggle(uint8_t index);

static TaskHandle_t sKnxTask = NULL;
OSA_EVENT_HANDLE_DEFINE(mKnxAppEvent);

/* g_mystate is dependent on the number of LEDs available on the board */
bool g_mystate[gAppLedsCnt_c] = {false, false}; /**< the state of the dpa 417.61 */

/**
 *  @brief function to set up the device.
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
  oc_core_set_device_model(0, "NXP Actuator");

  /* set the application info*/
  oc_core_set_device_ap(0, 1, 0, 0);

  /* set the manufacturer info*/
  oc_core_set_device_mid(0, 12);

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

  PRINT("-- Begin get_dpa_417_61: interface %d\r\n", interfaces);
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
          oc_rep_set_text_string(root, rt, "urn:knx:dpa.417.61");
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
  PRINT("-- End get_dpa_417_61\n");
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
 * Put method for "uri_path" resource.
 * The function has as input the request body, which are the input values of the
 * PUT method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 * values. Resource Description:
 *
 * @param request the request representation.
 * @param index index of the boolean "value"
 */
STATIC void
put_handler(oc_request_t *request, uint8_t index)
{
  PRINT("-- Begin put_dpa_417_61:\r\n");

  oc_rep_t *rep = NULL;
  // handle the different requests
  if (oc_is_redirected_request(request)) {
    PRINT(" S-MODE or /P\r\n");
  }
  rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  put_dpa_417_61 received : %d\r\n", rep->value.boolean);
        g_mystate[index] = rep->value.boolean;

        knx_led_toggle(index);

        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        PRINT("-- End put_dpa_417_61\r\n");
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
  PRINT("-- End put_dpa_417_61\r\n");
}

/**
 * @brief PUT method for "p/o_1" resource.
 *
 * @param request the request representation.
 * @param interfaces the interface used for this call
 * @param user_data the user data.
 */
STATIC void
put_o_1(oc_request_t *request, oc_interface_mask_t interfaces,
          void *user_data)
{
  (void)interfaces;
  (void)user_data;
  put_handler(request, 0);
}

/**
 * @brief GET method for "p/o_2" resource.
 *
 * @param request the request representation.
 * @param interfaces the interface used for this call
 * @param user_data the user data.
 */
STATIC void
put_o_2(oc_request_t *request, oc_interface_mask_t interfaces,
          void *user_data)
{
  (void)interfaces;
  (void)user_data;
  put_handler(request, 1);
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

void register_resource_by_uri_path(const char* uri_path, oc_request_callback_t get_cb, oc_request_callback_t put_cb)
{
  PRINT("Register Resource with local path \"%s\"\r\n", uri_path);
  PRINT("Light Switching actuator 417 (LSAB) : SwitchOnOff \r\n");
  PRINT("Data point 417.61 (DPT_Switch) \r\n");
  oc_resource_t *res_light =
    oc_new_resource("light actuation", uri_path, 2, 0);
  oc_resource_bind_resource_type(res_light, "urn:knx:dpa.417.61");
  oc_resource_bind_dpt(res_light, "urn:knx:dpt.switch");
  oc_resource_bind_content_type(res_light, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_light, OC_IF_A); /* if.a */
  oc_resource_set_discoverable(res_light, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  oc_resource_set_periodic_observable(res_light, 1);
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  // oc_resource_set_observable(res_352, true);
  // set the GET handler
  oc_resource_set_request_handler(res_light, OC_GET, get_cb, NULL);
  // set the PUT handler
  oc_resource_set_request_handler(res_light, OC_PUT, put_cb, NULL);
  // register this resource,
  // this means that the resource will be listed in /.well-known/core
  oc_add_resource(res_light);
}

/* 
 * URL Table
 * | resource url |  functional block/dpa  | GET | PUT  |
 * | ------------ | ---------------------- | ----| ---- |
 * | p/o_1        | urn:knx:dpa.417.61     | Yes | Yes  |
 * | p/o_2        | urn:knx:dpa.417.61     | Yes | Yes  |
 */

void
register_resources(void)
{
  register_resource_by_uri_path(FIRST_URI_PATH, get_o_1, put_o_1);
  register_resource_by_uri_path(SECOND_URI_PATH, get_o_2, put_o_2);
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

void knx_led_toggle(uint8_t index)
{
  switch(index)
  {
      case 0:
        /* Toggle Green color from RGB LED on FRDM MCXW71 */
        GPIO_PortToggle(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN_MASK);
      break;

      case 1:
        /* Toggle Monochrome LED on FRDM MCXW71 */
        GPIO_PortToggle(BOARD_INITPINLED1_LED1_GPIO, BOARD_INITPINLED1_LED1_GPIO_PIN_MASK);
      break;

      default:
      break;
  }
}

void knx_appSpecificInit(void)
{
#if (gAppLedsCnt_c > 0)
    knx_initPinGreenLed();
#if (gAppLedsCnt_c > 1)
    knx_InitPinMonoLed();
#endif /* (gAppLedsCnt_c > 1) */
#endif /* (gAppLedsCnt_c > 0) */
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
  uint32_t event = 0U;
  otInstance * instance = NULL;

  PRINT("KNX-IOT Server name : \"%s\"", MY_NAME);

  /*
   The storage folder depends on the build system
   the folder is created in the makefile, with $target as name with _cred as
   post fix.
  */
  PRINT("\tstorage at './LSAB_minimal_creds' \r\n");
  oc_storage_config("./LSAB_minimal_creds");

  oc_set_max_app_data_size(MAX_APP_DATA_SIZE);

  /*initialize the variables */
  initialize_variables();

  /* initializes the handlers structure */
  STATIC const oc_handler_t handler = { .init = app_init,
                                        .signal_event_loop = signal_event_loop,
                                        .register_resources = register_resources
#ifdef OC_CLIENT
                                        ,
                                        .requests_entry = NULL
#endif
  };

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
