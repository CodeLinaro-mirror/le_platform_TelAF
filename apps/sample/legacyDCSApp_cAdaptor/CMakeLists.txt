# ./CMakeLists.txt
# Specify the minimum cmake version
cmake_minimum_required(VERSION 3.10)

# Project name
project(DataAppDemo)

add_subdirectory(datalib)
add_subdirectory(radiolib)

# Executable
add_executable(DataAppDemo main.c)

target_link_libraries(DataAppDemo pthread rt dataAdaptor radioAdaptor)
# set link flags
set_target_properties(DataAppDemo PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")

target_include_directories(DataAppDemo PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/datalib")
target_include_directories(DataAppDemo PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/radiolib")
target_include_directories(DataAppDemo PUBLIC "${PROJECT_SOURCE_DIR}/include")