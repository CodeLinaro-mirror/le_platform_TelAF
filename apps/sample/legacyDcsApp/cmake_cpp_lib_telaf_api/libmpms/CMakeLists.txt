# ./CMakeLists.txt

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{TARGET})
set(Legato_DIR ${LEGATO_ROOT}/cmake)
set(TELAF_ROOT $ENV{TELAF_ROOT})
set(TELAF_INTERFACES  $ENV{TELAF_INTERFACES})

find_package(Legato REQUIRED)

# include the service which is using
generate_client(${TELAF_ROOT}/apps/tafMngdServices/interfaces/staging/taf_mngdPm.api)

add_library(mpmsAdaptor SHARED mpmsAdaptor.cpp
 ${CMAKE_CURRENT_BINARY_DIR}/taf_mngdPm_client.c
 ${CMAKE_CURRENT_BINARY_DIR}/taf_mngdPm_commonclient.c)

target_link_libraries(mpmsAdaptor ${LEGATO_LIBRARY_PATH})
# set link flags
set_target_properties(mpmsAdaptor PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")

target_include_directories(mpmsAdaptor PUBLIC "${LEGATO_INCLUDE_DIRS}")
target_include_directories(mpmsAdaptor PUBLIC "${TELAF_INTERFACES}")
target_include_directories(mpmsAdaptor PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(mpmsAdaptor PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")