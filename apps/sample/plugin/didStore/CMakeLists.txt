# ./CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Project name
project(TafPiDiagDID)

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{TARGET})
set(Legato_DIR ${LEGATO_ROOT}/cmake)
set(TELAF_ROOT $ENV{TELAF_ROOT})
set(TELAF_INTERFACES  $ENV{TELAF_INTERFACES})

find_package(Legato REQUIRED)

# include the common type
generate_client(${LEGATO_ROOT}/interfaces/le_cfg.api)

add_library(TafPiDiagDID_shared_lib SHARED tafPiDidStore.c
 ${CMAKE_CURRENT_BINARY_DIR}/le_cfg_client.c
 ${CMAKE_CURRENT_BINARY_DIR}/le_cfg_commonclient.c
 ${CMAKE_CURRENT_BINARY_DIR}/le_cfg_common.h
 ${CMAKE_CURRENT_BINARY_DIR}/le_cfg_interface.h)

target_link_libraries(TafPiDiagDID_shared_lib ${LEGATO_LIBRARY_PATH})

# set link flags
set_target_properties(TafPiDiagDID_shared_lib PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")
target_include_directories(TafPiDiagDID_shared_lib PUBLIC "${LEGATO_INCLUDE_DIRS}")
target_include_directories(TafPiDiagDID_shared_lib PUBLIC "${TELAF_INTERFACES}")
target_include_directories(TafPiDiagDID_shared_lib PUBLIC "${TELAF_ROOT}/include")
target_include_directories(TafPiDiagDID_shared_lib PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(TafPiDiagDID_shared_lib PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(TafPiDiagDID_shared_lib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(TafPiDiagDID_shared_lib PUBLIC "${TELAF_ROOT}/include/plugin")
