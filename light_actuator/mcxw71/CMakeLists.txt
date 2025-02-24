#
#  Copyright 2024-2025 NXP
#
#  SPDX-License-Identifier: BSD-3-Clause
#

cmake_minimum_required(VERSION 3.20.0)

get_filename_component(PRJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
get_filename_component(KNX_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/../../KNX-IOT-STACK ABSOLUTE)
get_filename_component(OTNXP_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/../../ot-nxp ABSOLUTE)
get_filename_component(KNX_PORT_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/../../port ABSOLUTE)

include_directories(light-actuator-${OT_NXP_PLATFORM} PUBLIC
    ${OTNXP_SRCDIR}/openthread/examples/platforms
    ${KNX_SRCDIR}/include
    ${KNX_PORT_SRCDIR}/frdm_mcxw71
    ${KNX_PORT_SRCDIR}
    ${KNX_SRCDIR}/include
)

if(KNX_ACTUATOR)
add_executable(light-actuator-${OT_NXP_PLATFORM}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${OTNXP_SRCDIR}/examples/cli/app_freertos_hooks.c
    ${OTNXP_SRCDIR}/examples/cli/libc_hook.cpp
    ${OTNXP_SRCDIR}/openthread/examples/apps/cli/cli_uart.cpp
)

target_include_directories(light-actuator-${OT_NXP_PLATFORM} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OTNXP_SRCDIR}/examples/common/include
    ${OTNXP_SRCDIR}/openthread/examples/platforms
    ${OTNXP_SRCDIR}/openthread/include
    ${OTNXP_SRCDIR}/openthread/src/core
    ${OTNXP_SRCDIR}/src/common
    ${OTNXP_SRCDIR}/src/k32w1
    ${KNX_PORT_SRCDIR}
)

# Linker file found in ${PROJECT_SOURCE_DIR}/ot-nxp/src/k32w1/K32W1480_connectivity.ld
# Real path is ENV{NXP_K32W1_SDK_ROOT}/core/${MIDLWR_DIR}/wireless/framework/Common/devices/kw45_k32w1/gcc

target_compile_definitions(${NXP_DRIVER_LIB}
    PUBLIC
        # Putting gDebugConsoleEnable_d to 1 routes the debug interface to UART0
        # SDK_DEBUGCONSOLE needs to be set to 1 in this case
        gDebugConsoleEnable_d=0
        SDK_DEBUGCONSOLE=0
        PRINTF_ADVANCED_ENABLE=1
        gAppLedsCnt_c=2 # NXP specific define for KNX application, not to be confused with gAppLedCnt_c
)

target_link_libraries(light-actuator-${OT_NXP_PLATFORM} PUBLIC
    kis-port
    ${NXP_DRIVER_LIB}
    openthread-cli-ftd
    ${OT_PLATFORM_LIB}
    openthread-ftd
    ${OT_PLATFORM_LIB}
    openthread-cli-ftd
    ${OT_MBEDTLS}
    ot-config
    tinycbor-master

    -Llib
    -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/light-actuator-${OT_NXP_PLATFORM}.map

    # Wrapping libc malloc functions and defining the libc heap size to 0
    -Wl,--defsym=__heap_size__=0
    -Wl,--wrap=_malloc_r
    -Wl,--wrap=_realloc_r
    -Wl,--wrap=_free_r
    -Wl,--wrap=_calloc_r
    -Wl,--wrap=malloc
    -Wl,--wrap=free
    -Wl,--wrap=realloc
    -Wl,--wrap=calloc
    -Wl,--wrap=printf
    -Wl,--wrap=puts
)
endif()