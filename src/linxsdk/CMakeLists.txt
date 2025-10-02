cmake_minimum_required(VERSION 3.10)
project(linxsdk)

# Set project name
set(LINX_SDK_TARGET linx_sdk)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp
    ${CMAKE_CURRENT_SOURCE_DIR}/ota
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/mongoose
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/http
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
)

# Collect all source files
set(LINX_SDK_SOURCES
    # Main SDK source
    ${CMAKE_CURRENT_SOURCE_DIR}/linx_sdk.c
    
    # MCP module sources
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_property.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_tool.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_utils.c
    
    # OTA module sources
    ${CMAKE_CURRENT_SOURCE_DIR}/ota/linx_ota.c
    
    # Protocols module sources
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/linx_protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/linx_websocket.c
)

# Create static library
add_library(${LINX_SDK_TARGET} STATIC ${LINX_SDK_SOURCES})

# Set target properties
set_target_properties(${LINX_SDK_TARGET} PROPERTIES
    OUTPUT_NAME "linx_sdk"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Target include directories (for consumers of this library)
target_include_directories(${LINX_SDK_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp
    ${CMAKE_CURRENT_SOURCE_DIR}/ota
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/http
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
)

# Compiler flags
target_compile_options(${LINX_SDK_TARGET} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-variable
    -Wno-unused-parameter
    -O2
)

# Link with common libraries if they exist
if(TARGET cjson)
    target_link_libraries(${LINX_SDK_TARGET} PUBLIC cjson)
endif()

if(TARGET http_client)
    target_link_libraries(${LINX_SDK_TARGET} PUBLIC http_client)
endif()

if(TARGET linx_log)
    target_link_libraries(${LINX_SDK_TARGET} PUBLIC linx_log)
endif()

# Install rules
install(TARGETS ${LINX_SDK_TARGET}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install header files
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/linx_sdk.h
    DESTINATION include/linxsdk
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_property.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_server.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_tool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/mcp_utils.h
    DESTINATION include/linxsdk/mcp
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ota/linx_ota.h
    DESTINATION include/linxsdk/ota
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/linx_protocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/linx_websocket.h
    DESTINATION include/linxsdk/protocols
)

# Print build information
message(STATUS "LinX SDK static library will be built")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/lib")
message(STATUS "Library name: lib${LINX_SDK_TARGET}.a")