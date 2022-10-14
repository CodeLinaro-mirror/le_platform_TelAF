# ./CMakeLists.txt

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{TARGET})
set(Legato_DIR ${LEGATO_ROOT}/cmake)
set(TELAF_ROOT $ENV{TELAF_ROOT})
set(TELAF_INTERFACES  $ENV{TELAF_INTERFACES})

find_package(Legato REQUIRED)

# include the common type
generate_client(${TELAF_ROOT}/interfaces/taf_types.api)
# include the service which is using
generate_client(${TELAF_ROOT}/interfaces/taf_dcs.api)

add_library(dataAdaptor SHARED dataAdaptor.c
 ${CMAKE_CURRENT_BINARY_DIR}/taf_dcs_client.c
 ${CMAKE_CURRENT_BINARY_DIR}/taf_dcs_commonclient.c
 ${CMAKE_CURRENT_BINARY_DIR}/taf_types_interface.h)


target_link_libraries(dataAdaptor ${LEGATO_LIBRARY_PATH})
# set link flags
set_target_properties(dataAdaptor PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")

target_include_directories(dataAdaptor PUBLIC "${LEGATO_INCLUDE_DIRS}")
target_include_directories(dataAdaptor PUBLIC "${TELAF_INTERFACES}")
target_include_directories(dataAdaptor PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(dataAdaptor PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")