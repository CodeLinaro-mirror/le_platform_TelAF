cmake_minimum_required(VERSION 2.8)

project (telaf-testapps)

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{LEGATO_TARGET} )
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Unit_testapp)
set(Interfaces $ENV{TELAF_ROOT}/interfaces)

# Get the DGTOOL from environment first, otherwise using V2
set(DGTOOL "$ENV{DGTOOL}")
if("${DGTOOL}" STREQUAL "")
    set(DGTOOL "dgtool-V2")
endif()

set(MKTOOLS_X_C_FLAGS  $ENV{MKTOOLS_X_C_FLAGS})
message(STATUS "MKTOOLS_X_C_FLAGS: ${MKTOOLS_X_C_FLAGS}")

# Define MKTOOLS_X_C_FLAGS as a list to aviod adding "\" before space
separate_arguments(MKAPP_CFLAGS_LIST UNIX_COMMAND "${MKTOOLS_X_C_FLAGS}")

# CMake Extension
message(STATUS "legato root dir: ${LEGATO_ROOT}")
set(Legato_DIR ${LEGATO_ROOT}/cmake)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/test)
find_package(Legato REQUIRED)

# Add one step to generate the diag_ids.h for diag app from the dgtool
add_custom_target(
    BuildDgtool
    COMMAND ${CMAKE_MAKE_PROGRAM} -C $ENV{TELAF_ROOT}/apps/tools/tafDiagGen -f dgtool.mk ${DGTOOL} DGTOOL_TARGET=${LEGATO_TARGET}
    WORKING_DIRECTORY $ENV{TELAF_ROOT}
    COMMENT "Leverage the external dgtool.mk to get the generated stuff for diag apps"
)
# add_custom_target(All ALL DEPENDS BuildDgtool)

# Build unit test apps
file(GLOB_RECURSE UNITTEST_ADEF_FILES ${TESTAPP_PATH}/*UnitTest*.adef)
foreach( unittestfile  ${UNITTEST_ADEF_FILES} )
    message(STATUS "adef file: ${unittestfile}")
    mkapp (${unittestfile} -i ${Interfaces} -i ${Interfaces}/staging ${MKAPP_CFLAGS_LIST})
endforeach()

# Build integration test apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Integration_testapp)
file(GLOB_RECURSE UNITTEST_ADEF_FILES ${TESTAPP_PATH}/*Int*Test.adef)
foreach( unittestfile  ${UNITTEST_ADEF_FILES} )
    message(STATUS "adef file: ${unittestfile}")
    mkapp( ${unittestfile} -i ${Interfaces} -i ${Interfaces}/staging ${MKAPP_CFLAGS_LIST})
endforeach()

# Build console test apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/test/ConsoleApp)
file(GLOB_RECURSE CONSOLEAPP_ADEF_FILES ${TESTAPP_PATH}/*.adef)
foreach( consoleappfile  ${CONSOLEAPP_ADEF_FILES} )
    message(STATUS "adef file: ${consoleappfile}")
    mkapp( ${consoleappfile} -i ${Interfaces} -i ${Interfaces}/staging ${MKAPP_CFLAGS_LIST})
endforeach()

# Build sample test apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/sample)
file(GLOB_RECURSE CONSOLEAPP_ADEF_FILES ${TESTAPP_PATH}/*.adef)
foreach( consoleappfile  ${CONSOLEAPP_ADEF_FILES} )
    if((${consoleappfile} MATCHES "tafRSPApp.adef$") OR
       (${consoleappfile} MATCHES "tafRSimApp.adef$") OR
       (${consoleappfile} MATCHES "tafSAPProviderApp.adef$") OR
       (${consoleappfile} MATCHES "tafECallApp.adef$"))
           message(STATUS "adef file: ${consoleappfile}")
           mkapp( ${consoleappfile} -i ${Interfaces} -i ${Interfaces}/staging ${MKAPP_CFLAGS_LIST})
    endif()
endforeach()

# Build legato sample apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp)
set(TESTAPP_PATH  $ENV{LEGATO_ROOT}/apps/sample)
file(GLOB_RECURSE CONSOLEAPP_ADEF_FILES ${TESTAPP_PATH}/voiceCallApp.adef)
foreach( consoleappfile  ${CONSOLEAPP_ADEF_FILES} )
    message(STATUS "adef file: ${consoleappfile}")
    mkapp( ${consoleappfile} -i ${Interfaces} ${MKAPP_CFLAGS_LIST})
endforeach()

# Build Managed Connectivity testapps
set(MngInterfaces $ENV{TELAF_ROOT}/apps/tafMngdServices/interfaces)
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Unit_testapp)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/tafMngdServices/tafMngdTest)
file(GLOB_RECURSE MNGD_ADEF_FILES ${TESTAPP_PATH}/*UnitTest.adef )
foreach( mngdappfile  ${MNGD_ADEF_FILES} )
	 message(STATUS "adef file: ${mngdappfile}")
	 mkapp( ${mngdappfile} -i ${Interfaces} -i ${MngInterfaces} -i ${MngInterfaces}/staging ${MKAPP_CFLAGS_LIST})
endforeach()

set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Integration_testapp)
file(GLOB_RECURSE MNGD_ADEF_FILES ${TESTAPP_PATH}/*Int*Test.adef )
foreach( mngdappfile  ${MNGD_ADEF_FILES} )
         message(STATUS "adef file: ${mngdappfile}")
         mkapp( ${mngdappfile} -i ${Interfaces} -i ${MngInterfaces}  -i ${MngInterfaces}/staging ${MKAPP_CFLAGS_LIST})
endforeach()

