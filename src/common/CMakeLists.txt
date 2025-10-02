cmake_minimum_required(VERSION 3.10)
project(linx_common)

# Set project name
set(LINX_COMMON_TARGET linx_common)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/http
    ${CMAKE_CURRENT_SOURCE_DIR}/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/mongoose
)

# Collect all source files from subdirectories
set(LINX_COMMON_SOURCES
    # cJSON sources
    ${CMAKE_CURRENT_SOURCE_DIR}/cjson/cJSON.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cjson/cJSON_Utils.c
    
    # HTTP client sources
    ${CMAKE_CURRENT_SOURCE_DIR}/http/http_client.c
    
    # Log sources
    ${CMAKE_CURRENT_SOURCE_DIR}/log/linx_log.c
)

# Create static library
add_library(${LINX_COMMON_TARGET} STATIC ${LINX_COMMON_SOURCES})

# Set target properties for static library
set_target_properties(${LINX_COMMON_TARGET} PROPERTIES
    OUTPUT_NAME "linx_common"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    PREFIX "lib"
    SUFFIX ".a"
)

# Target include directories (for consumers of this library)
target_include_directories(${LINX_COMMON_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/http
    ${CMAKE_CURRENT_SOURCE_DIR}/log
)

# Compiler flags
target_compile_options(${LINX_COMMON_TARGET} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-unused-function
    -O2
)

# Link with mongoose if available (for HTTP client)
if(TARGET mongoose)
    target_link_libraries(${LINX_COMMON_TARGET} PUBLIC mongoose)
endif()

# Platform-specific linking
if(APPLE)
    # macOS specific libraries for HTTP client
    target_link_libraries(${LINX_COMMON_TARGET} PUBLIC
        "-framework CoreFoundation"
        "-framework Security"
    )
elseif(UNIX)
    # Linux specific libraries
    target_link_libraries(${LINX_COMMON_TARGET} PUBLIC
        pthread
        m
    )
endif()

# Install rules
install(TARGETS ${LINX_COMMON_TARGET}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install header files with proper directory structure
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cjson/cJSON.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cjson/cJSON_Utils.h
    DESTINATION include/linx_common/cjson
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/http/http_client.h
    DESTINATION include/linx_common/http
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/log/linx_log.h
    DESTINATION include/linx_common/log
)

# Print build information
message(STATUS "Building LINX Common Library")
message(STATUS "  - Combined library: linx_common (includes cJSON, HTTP client, and Log)")