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
file(GLOB_RECURSE UNITTEST_ADEF_FILES ${TESTAPP_PATH}/*UnitTest*.adef)
foreach( unittestfile  ${UNITTEST_ADEF_FILES} )
    message(STATUS "adef file: ${unittestfile}")
    mkapp( ${unittestfile} -i ${Interfaces} -i ${Interfaces}/staging)
endforeach()

# Build integration test apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Integration_testapp)
file(GLOB_RECURSE UNITTEST_ADEF_FILES ${TESTAPP_PATH}/*Int*Test.adef)
foreach( unittestfile  ${UNITTEST_ADEF_FILES} )
    message(STATUS "adef file: ${unittestfile}")
    mkapp( ${unittestfile} -i ${Interfaces} -i ${Interfaces}/staging)
endforeach()

# Build console test apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/test/ConsoleApp)
file(GLOB_RECURSE CONSOLEAPP_ADEF_FILES ${TESTAPP_PATH}/*.adef)
foreach( consoleappfile  ${CONSOLEAPP_ADEF_FILES} )
    message(STATUS "adef file: ${consoleappfile}")
    mkapp( ${consoleappfile} -i ${Interfaces} -i ${Interfaces}/staging)
endforeach()

# Build sample test apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/sample)
file(GLOB_RECURSE CONSOLEAPP_ADEF_FILES ${TESTAPP_PATH}/*.adef)
foreach( consoleappfile  ${CONSOLEAPP_ADEF_FILES} )
    if((${consoleappfile} MATCHES "tafRSPApp.adef$") OR
       (${consoleappfile} MATCHES "tafRSimApp.adef$") OR
       (${consoleappfile} MATCHES "tafDiagApp.adef") OR
       (${consoleappfile} MATCHES "tafSAPProviderApp.adef$") OR
       (${consoleappfile} MATCHES "tafECallApp.adef$") OR
       (${consoleappfile} MATCHES "tafDiagEventApp.adef$"))
           message(STATUS "adef file: ${consoleappfile}")
           mkapp( ${consoleappfile} -i ${Interfaces} -i ${Interfaces}/staging)
    endif()
endforeach()

# Build legato sample apps
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp)
set(TESTAPP_PATH  $ENV{LEGATO_ROOT}/apps/sample)
file(GLOB_RECURSE CONSOLEAPP_ADEF_FILES ${TESTAPP_PATH}/voiceCallApp.adef)
foreach( consoleappfile  ${CONSOLEAPP_ADEF_FILES} )
    message(STATUS "adef file: ${consoleappfile}")
    mkapp( ${consoleappfile} -i ${Interfaces})
endforeach()

# Build Managed Connectivity testapps
set(MngInterfaces $ENV{TELAF_ROOT}/apps/tafMngdServices/interfaces)
set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Unit_testapp)
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/tafMngdServices/tafMngdTest)
file(GLOB_RECURSE MNGD_ADEF_FILES ${TESTAPP_PATH}/*UnitTest.adef )
foreach( mngdappfile  ${MNGD_ADEF_FILES} )
	 message(STATUS "adef file: ${mngdappfile}")
	 mkapp( ${mngdappfile} -i ${Interfaces} -i ${MngInterfaces} -i ${MngInterfaces}/staging)
endforeach()

set(APP_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/TestApps/Integration_testapp)
file(GLOB_RECURSE MNGD_ADEF_FILES ${TESTAPP_PATH}/*Int*Test.adef )
foreach( mngdappfile  ${MNGD_ADEF_FILES} )
         message(STATUS "adef file: ${mngdappfile}")
         mkapp( ${mngdappfile} -i ${Interfaces} -i ${MngInterfaces}  -i ${MngInterfaces}/staging)
endforeach()

# Build VHAL Drivers
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/sample/vhal)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp/VHAL/)
file(GLOB_RECURSE CONSOLEAPP_CDEF_FILES ${TESTAPP_PATH}/*.cdef)
foreach( consoleappfile  ${CONSOLEAPP_CDEF_FILES} )
        #message(STATUS "cdef file: ${consoleappfile}")
         if(${consoleappfile} MATCHES "Driver")
                 get_filename_component(second_dir "${consoleappfile}" PATH)
                 get_filename_component(second_last_dir_name "${second_dir}" NAME)
                 set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp/VHAL/${second_last_dir_name})
           message(STATUS "cdef file: ${consoleappfile}")
           message(STATUS "driverdir: ${second_last_dir_name}")
           #mkcomp(${consoleappfile}  )
           mkcomp(${second_dir} -o ${LIBRARY_OUTPUT_PATH}/${second_last_dir_name}.so  )
         endif()
endforeach()

#Build Plugins
set(TESTAPP_PATH  $ENV{TELAF_ROOT}/apps/sample/plugin)
file(GLOB_RECURSE CONSOLEAPP_CDEF_FILES ${TESTAPP_PATH}/*.sh)
foreach( consoleappfile  ${CONSOLEAPP_CDEF_FILES} )
        #message(STATUS "cdef file: ${consoleappfile}")
         if(${consoleappfile} MATCHES "sh")
                 get_filename_component(second_dir "${consoleappfile}" PATH)
                 get_filename_component(second_last_dir_name "${second_dir}" NAME)
                 set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/build/TestApps/Console_testapp/Plugin/${second_last_dir_name})
           message(STATUS "cdef file: ${consoleappfile}")
           message(STATUS "driverdir: ${second_last_dir_name}")
           string(SUBSTRING ${second_last_dir_name} 0 1 first_letter)
           string(TOUPPER ${first_letter} first_letter)
           string(REGEX REPLACE "^.(.*)" "${first_letter}\\1" Lib_name "${second_last_dir_name}")
           mkcomp(${second_dir} -o ${LIBRARY_OUTPUT_PATH}/tafPi${Lib_name}.so  )
         endif()
endforeach()
