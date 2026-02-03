cmake_minimum_required(VERSION 3.10)
project(telaf_pa_all)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(TAF_PA_ROOT $ENV{TELAF_ROOT}/../telaf-pa)

include_directories(${TAF_PA_ROOT}/include)
include_directories(${TAF_PA_ROOT}/component/taf_pa_common/stub/include)
link_directories($ENV{sdk_rootfs}/lib)
include_directories($ENV{sdk_rootfs}/include)

# Add default RPATH
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN:")

add_subdirectory(
     ${TAF_PA_ROOT}/component/taf_pa_common
     ${CMAKE_CURRENT_BINARY_DIR}/taf_pa_common)

add_subdirectory(taf_pa_location)
add_subdirectory(taf_pa_data)
add_subdirectory(taf_pa_sms)
add_subdirectory(taf_pa_sim)
add_subdirectory(taf_pa_ecall)
