# ./CMakeLists.txt
# Specify the minimum cmake version
cmake_minimum_required(VERSION 3.10)

# Project name
project(DataAppDemo)

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{TARGET})
set(Legato_DIR ${LEGATO_ROOT}/cmake)
set(TELAF_ROOT $ENV{TELAF_ROOT})
set(TelAF_DIR ${TELAF_ROOT}/cmake)
set(TELAF_INTERFACES  $ENV{TELAF_INTERFACES})

find_package(Legato REQUIRED)
find_package(TelAF REQUIRED)

# include the common type
generate_client(${TELAF_ROOT}/interfaces/taf_types.api)
# include the service which is using
generate_client(${TELAF_ROOT}/interfaces/taf_dcs.api)
generate_server(taf_dataAdaptor.api)

# Executable
add_executable(DataAppDemo main.cpp dataAdaptor.cpp
${CMAKE_CURRENT_BINARY_DIR}/taf_dcs_client.c
${CMAKE_CURRENT_BINARY_DIR}/taf_dcs_commonclient.c
${CMAKE_CURRENT_BINARY_DIR}/taf_types_interface.h
${CMAKE_CURRENT_BINARY_DIR}/taf_dataAdaptor_server.h
${CMAKE_CURRENT_BINARY_DIR}/taf_dataAdaptor_server.c
${CMAKE_CURRENT_BINARY_DIR}/taf_dataAdaptor_common.h
)

target_link_libraries(DataAppDemo pthread rt ${LEGATO_LIBRARY_PATH})
# set link flags
set_target_properties(DataAppDemo PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")

target_include_directories(DataAppDemo PUBLIC "${LEGATO_INCLUDE_DIRS}")
target_include_directories(DataAppDemo PUBLIC "${TELAF_INTERFACES}")
target_include_directories(DataAppDemo PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/include")