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

# Generate the build-id of the .so file.
execute_process(COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR} bash -c "realpath ${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}-someip.so --relative-to=${LEGATO_ROOT} | sha1sum | awk '{print $1}'"
                OUTPUT_VARIABLE COMMAND_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE)
message("CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

# Build SOME/IP library
add_library(${PROJECT_NAME}-someip SHARED ${PRJ_SOMEIP_LIB_SRCS})
target_link_libraries(${PROJECT_NAME}-someip CommonAPI-SomeIP)

target_compile_options(${PROJECT_NAME}-someip PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-g;-Wall;-Werror;-fno-omit-frame-pointer;-fmacro-prefix-map=${LEGATO_ROOT}=.;-fmacro-prefix-map=${TELAF_ROOT}=.;-fmacro-prefix-map=${LEGATO_SYSROOT}=.>")

# Set runtime path for shared libraries
set_target_properties(${PROJECT_NAME}-someip PROPERTIES LINK_FLAGS "-g -Wl,--build-id=0x${COMMAND_OUTPUT}")

# Extract and save debug info.
add_custom_command(TARGET ${PROJECT_NAME}-someip POST_BUILD
                   COMMAND ${LEGATO_ROOT}/framework/tools/scripts/splitdebug -d ${TELAF_ROOT}/build/${LEGATO_TARGET}/debug
                   $<TARGET_FILE:${PROJECT_NAME}-someip>)
