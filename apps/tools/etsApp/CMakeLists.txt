# File description:
#------------------
# This is a CMake build-configuration file.
# Author : samanta
#]=======================================================================]

cmake_minimum_required(VERSION 2.8)

set(PRJ_NAME someipETS)
set(PRJ_LIB_NAME ETS)

set(CMAKE_VERBOSE_MAKEFILE on)

OPTION(USE_FILE "Set to OFF to disable file logging" OFF )
message(STATUS "USE_FILE is set to value: ${USE_FILE}")

OPTION(USE_CONSOLE "Set to OFF to disable console logging" OFF )
message(STATUS "USE_CONSOLE is set to value: ${USE_CONSOLE}")

IF(USE_FILE)
  add_definitions(-DUSE_FILE)
ENDIF(USE_FILE)
IF(USE_CONSOLE)
  add_definitions(-DUSE_CONSOLE)
ENDIF(USE_CONSOLE)

SET(MAX_LOG_LEVEL "DEBUG" CACHE STRING "maximum log level")
message(STATUS "MAX_LOG_LEVEL is set to value: ${MAX_LOG_LEVEL}")
add_definitions(-DCOMMONAPI_LOGLEVEL=COMMONAPI_LOGLEVEL_${MAX_LOG_LEVEL})

if (${CMAKE_SYSTEM_NAME} MATCHES "QNX")
    set (OS "QNX")
    set (NO_DEPRECATED_REGISTER "")
    set (Boost_USE_STATIC_LIBS ON)
endif (${CMAKE_SYSTEM_NAME} MATCHES "QNX")

# CommonAPI
include(FindPkgConfig)

if (MSVC)
# Visual C++ is not always sure whether he is really C++
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS /EHsc /wd\\\"4503\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS /wd\\\"4503\\\"")
else()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -std=c++11 -pthread -D_GLIBCXX_USE_NANOSLEEP")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -pthread -D_GLIBCXX_USE_NANOSLEEP -D_FORTIFY_SOURCE=2")
endif()

message(STATUS "Compiler options: ${CMAKE_CXX_FLAGS}")

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
       "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

OPTION(USE_INSTALLED_COMMONAPI "Set to OFF to use the local (build tree) version of CommonAPI" ON)
message(STATUS "USE_INSTALLED_COMMONAPI is set to value: ${USE_INSTALLED_COMMONAPI}")

if ("${USE_INSTALLED_COMMONAPI}" STREQUAL "ON")
    FIND_PACKAGE(CommonAPI 3.2.0 REQUIRED CONFIG NO_CMAKE_PACKAGE_REGISTRY NO_CMAKE_PACKAGE_REGISTRY)
else()
    FIND_PACKAGE(CommonAPI 3.2.0 REQUIRED CONFIG NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
endif()

message(STATUS "CommonAPI_CONSIDERED_CONFIGS: ${CommonAPI_CONSIDERED_CONFIGS}")
message(STATUS "COMMONAPI_INCLUDE_DIRS: ${COMMONAPI_INCLUDE_DIRS}")
message(STATUS "VSOMEIP_INCLUDE_DIRS: ${VSOMEIP_INCLUDE_DIRS}")

# SOME/IP
find_package (CommonAPI-SomeIP 3.2.0 REQUIRED)
find_package (vsomeip3 3.1.20 REQUIRED)

# Source Files
set(PRJ_SRC_PATH src)
message(STATUS "Src_paths: ${PRJ_SRC_PATH}")
set(PRJ_SRC_GEN_PATH src-gen)

set(PRJ_NAME_CLIENT ${PRJ_NAME}Client)
message(STATUS "PRJ_NAME_CLIENT: ${PRJ_NAME_CLIENT}")

# Application
FILE(GLOB PRJ_PROXY_TESTABILITY_PATH ${PRJ_SRC_GEN_PATH}/v1/someip/testability/*.cpp)
FILE(GLOB PRJ_CORE_V1_SOMEIP_TESTABILITY ${PRJ_SRC_GEN_PATH}/v1/someip/testability/*.*)

message(STATUS "PRJ_PROXY_COMMONAPI_PATH: ${PRJ_PROXY_COMMONAPI_PATH}")
message(STATUS "PRJ_PROXY_TESTABILITY_PATH: ${PRJ_PROXY_TESTABILITY_PATH}")

set(PRJ_CLIENT_SRCS ${PRJ_SRC_PATH}/client/etsCliMain.cpp
                  ${PRJ_SRC_PATH}/client/etsProxyImpl.cpp
                  ${PRJ_PROXY_TESTABILITY_PATH}
                  #${PRJ_CORE_V1_SOMEIP_TESTABILITY}
                  )

# Boost
include_directories( ${Boost_INCLUDE_DIR} )

# SOME/IP library
FILE(GLOB PRJ_SOMEIP_LIB_SRCS ${PRJ_PROXY_TESTABILITY_PATH}/*.cpp)

include_directories(
    src-gen
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
    ../vsomeip/interface/vsomeip
)

if ("${USE_INSTALLED_DBUS}" STREQUAL "ON")
link_directories(
    ${COMMONAPI_LIBDIR}
    ${Boost_LIBRARY_DIR}
    ${COMMONAPI_SOMEIP_CMAKE_DIR}/build
    ${CMAKE_INSTALL_PREFIX}/lib
)
else()
link_directories(
    ${COMMONAPI_LIBDIR}
    ${COMMONAPI_SOMEIP_CMAKE_DIR}/build
    ${DBus_INCLUDE_DIRS}/dbus/.libs
    ${Boost_LIBRARY_DIR}
    ${CMAKE_INSTALL_PREFIX}/lib
)
endif()

if (MSVC)
  set(LINK_LIBRARIES CommonAPI)
else()
   set(LINK_LIBRARIES -Wl,--as-needed CommonAPI CommonAPI-SomeIP vsomeip3 -pthread stdc++)
endif()


#Build client
add_executable(${PRJ_NAME_CLIENT} ${PRJ_CLIENT_SRCS})
target_link_libraries(${PRJ_NAME_CLIENT} ${Boost_LIBRARIES} ${LINK_LIBRARIES})
#message(STATUS "LINK_LIBRARIES: ${LINK_LIBRARIES}")

# Build SOME/IP library
add_library (${PRJ_LIB_NAME}-someip SHARED ${PRJ_SOMEIP_LIB_SRCS})
target_link_libraries(${PRJ_LIB_NAME}-someip ${Boost_LIBRARIES} CommonAPI-SomeIP)

if (${SOA_APP_INSTALL_ETC})
else()
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for binaries")
set(SOA_APP_INSTALL_ETC etc CACHE PATH "Installation directory for configuration")

install(TARGETS ${PRJ_LIB_NAME}-someip
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR})

install(TARGETS ${PRJ_NAME_CLIENT}
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR})

endif ()

install(DIRECTORY config/ DESTINATION "${SOA_APP_INSTALL_ETC}/${PRJ_NAME}"
        CONFIGURATIONS ${CMAKE_BUILD_TYPE}
        COMPONENT SoA-examples
        FILES_MATCHING PATTERN "*")
