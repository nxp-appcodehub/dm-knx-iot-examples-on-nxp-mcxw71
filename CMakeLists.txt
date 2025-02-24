#
#  Copyright 2024-2025 NXP
#
#  SPDX-License-Identifier: BSD-3-Clause
#

cmake_minimum_required(VERSION 3.20.0)
cmake_policy(SET CMP0079 NEW)

project(knx-nxp-openthread
    VERSION 0.2.0
    LANGUAGES C CXX ASM
)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/ot-nxp/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/ot-nxp/cmake/platform)

include(utils)
include(${OT_NXP_PLATFORM} OPTIONAL)
include(options)

otnxp_git_version(OT_PACKAGE_VERSION)
set(OT_PACKAGE_VERSION "${OT_PACKAGE_VERSION}" CACHE STRING "")
message(STATUS "Setting default package version: ${OT_PACKAGE_VERSION}")
message(STATUS "Package Version: ${OT_PACKAGE_VERSION}")
message(STATUS "NXP Platform: ${OT_NXP_PLATFORM}")

set(OT_PLATFORM_LIB openthread-${OT_NXP_PLATFORM})
set(NXP_DRIVER_LIB nxp-${OT_NXP_PLATFORM}-driver)
set(NXP_KNX_PORT nxp-${OT_NXP_PLATFORM}-knx-port)

add_subdirectory(ot-nxp)
add_subdirectory(KNX-IOT-STACK)
add_subdirectory(port)
add_subdirectory(light_actuator)
add_subdirectory(light_sensor)

if(OT_NXP_EXPORT_TO_SREC)
    export_all_targets_to_srec()
endif()
