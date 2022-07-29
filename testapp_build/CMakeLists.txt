cmake_minimum_required(VERSION 2.8)

project (telaf-testapps)

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{LEGATO_TARGET} )
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Unit_testapp)
set(Interfaces $ENV{TELAF_ROOT}/interfaces)

# CMake Extension
message(STATUS "legato root dir: ${LEGATO_ROOT}")
set(Legato_DIR ${LEGATO_ROOT}/cmake)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/test)
find_package(Legato REQUIRED)

# Build unit test apps
file(GLOB_RECURSE UNITTEST_ADEF_FILES ${TESTAPP_PATH}/*UnitTest.adef)
foreach( unittestfile  ${UNITTEST_ADEF_FILES} )
    message(STATUS "adef file: ${unittestfile}")
    mkapp( ${unittestfile} -i ${Interfaces})
endforeach()

# Build integration test apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Integration_testapp)
file(GLOB_RECURSE UNITTEST_ADEF_FILES ${TESTAPP_PATH}/*Int*Test.adef)
foreach( unittestfile  ${UNITTEST_ADEF_FILES} )
    message(STATUS "adef file: ${unittestfile}")
    mkapp( ${unittestfile} -i ${Interfaces})
endforeach()
