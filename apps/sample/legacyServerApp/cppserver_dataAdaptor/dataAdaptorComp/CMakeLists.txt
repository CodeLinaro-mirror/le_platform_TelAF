# ./CMakeLists.txt
# Specify the minimum cmake version
cmake_minimum_required(VERSION 3.10)

# Project name
project(DataAppDemo)

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

set(API_PATH    taf_dataAdaptor.api)
set(API_NAME    "taf_dataAdaptor")
set(SERVER_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_server.h")
set(SERVER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_server.c")
set(LOCAL_PATH  "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_service.h")
set(COMMON_LOCAL_PATH  "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_messages.h")
set(COMMON_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_common.h")

add_custom_command( OUTPUT ${SERVER_HEADER_PATH} ${SERVER_PATH} ${LOCAL_PATH}
                           ${COMMON_LOCAL_PATH} ${COMMON_HEADER_PATH}
                    COMMAND ifgen --gen-server-interface --gen-server --gen-local
                            --gen-messages --gen-common-interface
                    ${API_PATH}
                    --import-dir ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "ifgen '${API_PATH}': ${SERVER_HEADER_PATH}"
                    DEPENDS ${API_PATH}
                    )

# Executable
add_executable(DataAppDemo main.cpp dataAdaptor.cpp
${CMAKE_CURRENT_BINARY_DIR}/taf_dcs_client.c
${CMAKE_CURRENT_BINARY_DIR}/taf_dcs_commonclient.c
${CMAKE_CURRENT_BINARY_DIR}/taf_types_interface.h
${SERVER_HEADER_PATH} ${SERVER_PATH} ${COMMON_HEADER_PATH}
)

target_link_libraries(DataAppDemo pthread rt ${LEGATO_LIBRARY_PATH})
# set link flags
set_target_properties(DataAppDemo PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")

target_include_directories(DataAppDemo PUBLIC "${LEGATO_INCLUDE_DIRS}")
target_include_directories(DataAppDemo PUBLIC "${TELAF_INTERFACES}")
target_include_directories(DataAppDemo PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/include")