cmake_minimum_required(VERSION 2.8)

project(IvssRadioSvcTest)

#SET(MAX_LOG_LEVEL "DEBUG" CACHE STRING "maximum log level")
message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")

# CommonAPI
include(FindPkgConfig)

# CommonAPI library
find_package(CommonAPI CONFIG 3.2.0 REQUIRED)
find_package(CommonAPI-SomeIP CONFIG 3.2.0 REQUIRED)
find_package(vsomeip3 CONFIG 3.1.0 REQUIRED)

# Source Files
set(PRJ_SRC_PATH src)
set(PRJ_SRC_GEN_PATH src-gen/v0/com/qualcomm/qti/modem)
set(PRJ_INCLUDE_DIRS src-gen/)

# Paths
include_directories(
    ${PRJ_INCLUDE_DIRS}
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
)

# Application
set(PRJ_NAME_CLIENT ${PROJECT_NAME}Client)
FILE(GLOB PRJ_SRC_LIB_SRCS ${PRJ_SRC_PATH}/*cpp)
FILE(GLOB PRJ_SRC_GEN_SRCS ${PRJ_SRC_GEN_PATH}/*.cpp)

add_executable(${PRJ_NAME_CLIENT}
    ${PRJ_SRC_LIB_SRCS}
    ${PRJ_SRC_GEN_SRCS}
)

target_link_libraries(${PRJ_NAME_CLIENT} CommonAPI CommonAPI-SomeIP vsomeip3)

install (
    TARGETS ${PRJ_NAME_CLIENT}
    RUNTIME DESTINATION ./ COMPONENT bin)

set(EXAMPLE_CONFIG_FILES
    "commonapi-test.ini"
    "vsomeip-test.json"
)

install (
    FILES ${EXAMPLE_CONFIG_FILES} DESTINATION conf COMPONENT config
)
