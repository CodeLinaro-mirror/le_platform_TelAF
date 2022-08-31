cmake_minimum_required(VERSION 2.8)

project (telaf-testapps)

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{LEGATO_TARGET} )
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/testapp)
set(Interfaces $ENV{TELAF_ROOT}/interfaces)

# CMake Extension
message(STATUS "cmake_module_path include dir: $ENV{LEGATO_ROOT}")
set(CMAKE_MODULE_PATH $ENV{LEGATO_ROOT}/cmake)

include("${CMAKE_MODULE_PATH}/Helpers.cmake")
set(Legato_DIR $ENV{LEGATO_ROOT}/cmake)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/test)
find_package(Legato REQUIRED)

file(GLOB_RECURSE UNITTEST_ADEF_FILES ${TESTAPP_PATH}/*UnitTest.adef)

foreach( unittestfile  ${UNITTEST_ADEF_FILES} )
    message(STATUS "adef file: ${unittestfile}")
    mkapp( ${unittestfile} -i ${Interfaces})
endforeach()
