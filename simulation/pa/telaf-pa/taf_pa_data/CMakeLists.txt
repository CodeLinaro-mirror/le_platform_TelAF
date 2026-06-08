cmake_minimum_required(VERSION 3.16)
project(taf_pa_data LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")
set(TARGET_NAME taf_pa_data)
set(OUTPUT_NAME Component_${TARGET_NAME})

# Add sources
set(TAF_PA_DATA_SOURCES
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data.cpp
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data_Utils.cpp
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data_TeluxDataImpl.cpp
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data_TeluxDataConnectionImpl.cpp
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data_TeluxDataConnectionListenerImpl.cpp
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data_TeluxDataProfileImpl.cpp
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data_TeluxDataProfileListCallbackImpl.cpp
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data_TeluxDataProfileListenerImpl.cpp
    ${TAF_PA_ROOT}/component/taf_pa_data/taf_pa_data_TeluxDataServingSysListenerImpl.cpp
)

include_directories(${TAF_PA_ROOT}/include/data)
add_library(${TARGET_NAME} SHARED ${TAF_PA_DATA_SOURCES})

set_target_properties(${TARGET_NAME} PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    OUTPUT_NAME ${OUTPUT_NAME}
)

target_link_libraries(${TARGET_NAME} PRIVATE telux_common telux_data telux_tel)

target_link_libraries(${TARGET_NAME} PRIVATE taf_pa_common)

install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION lib
)
