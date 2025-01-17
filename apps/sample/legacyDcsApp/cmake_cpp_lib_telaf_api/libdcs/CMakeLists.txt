# ./CMakeLists.txt

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{TARGET})
set(Legato_DIR ${LEGATO_ROOT}/cmake)
set(TELAF_ROOT $ENV{TELAF_ROOT})
set(TELAF_INTERFACES  $ENV{TELAF_INTERFACES})

find_package(Legato REQUIRED)

# Generate the required interface files for DCS service.
generate_client(${TELAF_ROOT}/interfaces/taf_dcs.api)
generate_client(${TELAF_ROOT}/interfaces/taf_types.api)
generate_client(${TELAF_ROOT}/interfaces/taf_radio.api)

add_library(dcsAdaptor SHARED dcsAdaptor.cpp
 ${CMAKE_CURRENT_BINARY_DIR}/taf_dcs_client.c
 ${CMAKE_CURRENT_BINARY_DIR}/taf_dcs_commonclient.c
 ${CMAKE_CURRENT_BINARY_DIR}/taf_types_interface.h
 ${CMAKE_CURRENT_BINARY_DIR}/taf_radio_interface.h)

target_link_libraries(dcsAdaptor ${LEGATO_LIBRARY_PATH})
# set link flags
set_target_properties(dcsAdaptor PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")

target_include_directories(dcsAdaptor PUBLIC "${LEGATO_INCLUDE_DIRS}")
target_include_directories(dcsAdaptor PUBLIC "${TELAF_INTERFACES}")
target_include_directories(dcsAdaptor PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(dcsAdaptor PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
