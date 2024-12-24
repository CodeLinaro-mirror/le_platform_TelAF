# ./CMakeLists.txt
# Specify the minimum cmake version
cmake_minimum_required(VERSION 3.10)

# Project name
project(DataAppDemo)



add_subdirectory(libradio)
add_subdirectory(libmpms)

# Executable
add_executable(DataAppDemo main.cpp)

target_link_libraries(DataAppDemo pthread rt radioAdaptor mpmsAdaptor)
# set link flags
set_target_properties(DataAppDemo PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath=/legato/systems/current/lib")


target_include_directories(DataAppDemo PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/libradio")
target_include_directories(DataAppDemo PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/libmpms")
target_include_directories(DataAppDemo PUBLIC "${PROJECT_SOURCE_DIR}/include")