# ./CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Project name
project(TafPiDiagDIDInDB)

set(LEGATO_ROOT  $ENV{LEGATO_ROOT})
set(LEGATO_TARGET $ENV{TARGET})
set(Legato_DIR ${LEGATO_ROOT}/cmake)
set(TELAF_ROOT $ENV{TELAF_ROOT})
set(TELAF_INTERFACES  $ENV{TELAF_INTERFACES})

find_package(Legato REQUIRED)

# Find the pre-built tafDIDDataAccessComp library anywhere in the build tree
find_library(TAF_DATA_ACCESS_LIB
    NAMES Component_tafDIDDataAccessComp
    PATHS ${LEGATO_ROOT}/build/${LEGATO_TARGET}
    PATH_SUFFIXES staging/lib lib
    NO_DEFAULT_PATH)

if(NOT TAF_DATA_ACCESS_LIB)
    # Fall back: search recursively under the build directory
    file(GLOB_RECURSE TAF_DATA_ACCESS_LIB_GLOB
        "${LEGATO_ROOT}/build/${LEGATO_TARGET}/libComponent_tafDIDDataAccessComp.so"
        "${LEGATO_ROOT}/build/${LEGATO_TARGET}/*/libComponent_tafDIDDataAccessComp.so")
    list(LENGTH TAF_DATA_ACCESS_LIB_GLOB _lib_count)
    if(_lib_count GREATER 0)
        list(GET TAF_DATA_ACCESS_LIB_GLOB 0 TAF_DATA_ACCESS_LIB)
    endif()
endif()

if(NOT TAF_DATA_ACCESS_LIB)
    message(WARNING "libComponent_tafDIDDataAccessComp.so not found under ${LEGATO_ROOT}/build/${LEGATO_TARGET}. Skipping the TafPiDiagDIDInDB plugin; build TelAF first if it is required.")
    return()
endif()

add_library(TafPiDiagDIDInDB_shared_lib SHARED tafPiDidStoreInDB.c)

target_link_libraries(TafPiDiagDIDInDB_shared_lib
    ${LEGATO_LIBRARY_PATH}
    ${TAF_DATA_ACCESS_LIB})

# set link flags
set_target_properties(TafPiDiagDIDInDB_shared_lib PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")
target_include_directories(TafPiDiagDIDInDB_shared_lib PUBLIC "${LEGATO_INCLUDE_DIRS}")
target_include_directories(TafPiDiagDIDInDB_shared_lib PUBLIC "${TELAF_INTERFACES}")
target_include_directories(TafPiDiagDIDInDB_shared_lib PUBLIC "${TELAF_ROOT}/include")
target_include_directories(TafPiDiagDIDInDB_shared_lib PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(TafPiDiagDIDInDB_shared_lib PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(TafPiDiagDIDInDB_shared_lib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(TafPiDiagDIDInDB_shared_lib PUBLIC "${TELAF_ROOT}/include/plugin")
target_include_directories(TafPiDiagDIDInDB_shared_lib PUBLIC "${TELAF_ROOT}/components/tafDataAccessComp/tafDIDDataAccessComp")
