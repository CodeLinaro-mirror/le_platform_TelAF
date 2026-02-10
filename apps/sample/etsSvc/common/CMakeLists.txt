cmake_minimum_required(VERSION 2.8)

project(${PROJECT_NAME})
message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")

# CommonAPI
include(FindPkgConfig)

set(PKG_CONFIG_SYSROOT_DIR $ENV{PKG_CONFIG_SYSROOT_DIR})
list(APPEND CMAKE_PREFIX_PATH "${PKG_CONFIG_SYSROOT_DIR}/usr/lib/cmake")

# CommonAPI library
find_package(vsomeip3 3.1.0 REQUIRED)
find_package(CommonAPI 3.2.0 REQUIRED)
find_package(CommonAPI-SomeIP 3.2.0 REQUIRED)

# Source Files
set(PRJ_SRC_GEN_PATH src-gen)
set(PRJ_INCLUDE_DIRS src-gen/)

# Paths
include_directories(
    ${PRJ_INCLUDE_DIRS}
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
)

# SOME/IP library
FILE(GLOB_RECURSE PRJ_SOMEIP_LIB_SRCS ${PRJ_SRC_GEN_PATH}/*.cpp)

# Build SOME/IP library
add_library(${PROJECT_NAME}-someip SHARED ${PRJ_SOMEIP_LIB_SRCS})
target_link_libraries(${PROJECT_NAME}-someip CommonAPI-SomeIP)
