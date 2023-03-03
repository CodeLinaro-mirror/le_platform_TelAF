

# Function to generate the required files for a server
function(generate_server API_FILE)

    get_filename_component(API_NAME ${API_FILE} NAME_WE)
    set(SERVER_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_server.h")
    set(SERVER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_server.c")
    set(LOCAL_PATH  "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_service.h")
    set(COMMON_LOCAL_PATH  "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_messages.h")
    set(COMMON_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_common.h")

    add_custom_command( OUTPUT ${SERVER_HEADER_PATH} ${SERVER_PATH} ${LOCAL_PATH}
                            ${COMMON_LOCAL_PATH} ${COMMON_HEADER_PATH}
                        COMMAND ifgen --gen-server-interface --gen-server --gen-local
                                --gen-messages --gen-common-interface
                        ${API_FILE}
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}
                        COMMENT "ifgen '${API_FILE}': ${SERVER_HEADER_PATH}"
                        DEPENDS ${API_FILE}
                        )

endfunction()