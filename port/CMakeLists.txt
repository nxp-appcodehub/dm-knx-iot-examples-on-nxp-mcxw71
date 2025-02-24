#
#  Copyright 2024-2025 NXP
#
#  SPDX-License-Identifier: BSD-3-Clause
#

get_filename_component(KNX_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/../KNX-IOT-STACK ABSOLUTE)
get_filename_component(OTNXP_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/../ot-nxp ABSOLUTE)

add_library(kis-port
    # KNX Porting Layer to NXP hardware
    abort.c
    clock.c
    thread_connectivity.c
    dns-sd.c
    random.c
    storage.c
    # KNX Shell addon
    knx_shell.c

    # OT-NXP file for reset
    ${OTNXP_SRCDIR}/src/common/spinel/misc.c
)

target_include_directories(kis-port
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OT_PUBLIC_INCLUDES}
	${KNX_SRCDIR}
	${KNX_SRCDIR}/port
    ${KNX_SRCDIR}/api
    ${KNX_SRCDIR}/api/c-timestamp
    ${KNX_SRCDIR}/deps
    ${KNX_SRCDIR}/include
    ${KNX_SRCDIR}/port
    ${KNX_SRCDIR}/messaging/coap/
    ${KNX_SRCDIR}/security
    ${OTNXP_SRCDIR}/third_party/github_sdk/middleware/mbedtls/include
    ${OTNXP_SRCDIR}/examples/common/include
    ${OTNXP_SRCDIR}/openthread/examples/platforms
    ${OTNXP_SRCDIR}/openthread/include
    ${OTNXP_SRCDIR}/openthread/src/core
    ${OTNXP_SRCDIR}/src/common
    ${OTNXP_SRCDIR}/src/k32w1
	${PROJECT_SOURCE_DIR}/port
	${PROJECT_SOURCE_DIR}/port/freertos
)

if (KNX_ACTUATOR)
    target_sources(kis-port PUBLIC
        # Pin Mux for LED required by actuator app
        frdm_mcxw71/knx_pin_mux.c
    )

    target_include_directories(kis-port
        PUBLIC
        frdm_mcxw71
    )
endif()

target_link_libraries(kis-port
    PUBLIC
    kisClientServer
    ot-config
    ${NXP_DRIVER_LIB}
    ${OT_PLATFORM_LIB}
    ${OT_MBEDTLS}
)

add_compile_definitions(GOT_MAX_ENTRIES=${KNX_GOT_MAX_ENTRIES} GPT_MAX_ENTRIES=${KNX_GPT_MAX_ENTRIES} GRT_MAX_ENTRIES=${KNX_GRT_MAX_ENTRIES} G_AT_MAX_ENTRIES=${KNX_SEC_MAX_ENTRIES})