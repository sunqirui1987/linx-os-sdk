cmake_minimum_required(VERSION 3.10)
project(linx_test_ota C)

# 设置 SDK 路径
set(SDK_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../out/linx")
set(SDK_INCLUDE_DIR "${SDK_INSTALL_DIR}/include")
set(SDK_LIB_DIR "${SDK_INSTALL_DIR}/lib")


# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../)

# Include find modules first
list(APPEND CMAKE_MODULE_PATH ${SDK_DIR}/cmake)

# Don't add the OTA directory as a subdirectory since we're already in the OTA test directory
# This was causing the recursion error
message(STATUS "Building test ota ${SDK_DIR} ")

# Test executable
set(TEST_SOURCES
    ota_test.c
    ../linx_ota.c
    ${CMAKE_CURRENT_LIST_DIR}/../../log/linx_log.c
    ${CMAKE_CURRENT_LIST_DIR}/../../cjson/cjson.c
)

# Create test executable
add_executable(ota_test ${TEST_SOURCES})

# Include directories
target_include_directories(ota_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../../
    ${CMAKE_CURRENT_LIST_DIR}/../../log
    ${CMAKE_CURRENT_LIST_DIR}/../../cjson
    ${CMAKE_CURRENT_LIST_DIR}/../../third/mongoose
    ${LINX_OTA_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(ota_test PRIVATE
    cjson
    ${SDK_LIB_DIR}/libmongoose.a
    ${SDK_LIB_DIR}/libopus.a
)

# Install test binary
install(TARGETS ota_test DESTINATION bin)