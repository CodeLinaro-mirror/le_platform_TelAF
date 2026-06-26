# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# CMake build for tafLegacyCApp - a legacy C app that calls TelAF taf_time APIs
# without being a Legato app.  Modeled on apps/sample/legacyDcsApp/cmake_app.
#
# Build (see README.md for full steps):
#   cd ~/telaf && source set_af_env.sh sa525m
#   ./bin/legs && export TARGET=$TARGET_GLOBAL
#   cd apps/test/tafLegacyCApp && mkdir build && cd build
#   cmake .. && make
#
# Output executable: build/tafLegacyCApp

cmake_minimum_required(VERSION 3.10)

project(tafLegacyCApp)

set(LEGATO_ROOT       $ENV{LEGATO_ROOT})
set(LEGATO_TARGET     $ENV{TARGET})
set(Legato_DIR        ${LEGATO_ROOT}/cmake)
set(TELAF_ROOT        $ENV{TELAF_ROOT})
set(TELAF_INTERFACES  $ENV{TELAF_INTERFACES})

find_package(Legato REQUIRED)

# Generate the client-side IPC stubs for the service(s) we use.
# taf_time is self-contained (no USETYPES), so only its client is needed.
generate_client(${TELAF_ROOT}/interfaces/taf_time.api)

# Executable: our main + the generated client stubs.
add_executable(tafLegacyCApp main.c
    ${CMAKE_CURRENT_BINARY_DIR}/taf_time_client.c
    ${CMAKE_CURRENT_BINARY_DIR}/taf_time_commonclient.c)

target_link_libraries(tafLegacyCApp pthread rt ${LEGATO_LIBRARY_PATH})

# Resolve liblegato from the on-target framework lib dir at runtime.
set_target_properties(tafLegacyCApp PROPERTIES
    LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")

target_include_directories(tafLegacyCApp PUBLIC "${LEGATO_INCLUDE_DIRS}")
target_include_directories(tafLegacyCApp PUBLIC "${TELAF_INTERFACES}")
target_include_directories(tafLegacyCApp PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}")
