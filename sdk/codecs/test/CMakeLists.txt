cmake_minimum_required(VERSION 3.10)
project(linx_test_codec C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../)




# Include find modules first
list(APPEND CMAKE_MODULE_PATH ${SDK_DIR}/cmake)

# Add the codecs subdirectory with explicit binary directory
add_subdirectory(${SDK_DIR}/codecs ${CMAKE_CURRENT_BINARY_DIR}/codecs)
message(STATUS "Building test codec ${SDK_DIR} ${LINX_CODEC_INCLUDE_DIRS}")

# Test executable
set(TEST_SOURCES
    codec_test.c
    ${CMAKE_CURRENT_LIST_DIR}/../../log/linx_log.c
    ${LINX_CODEC_SOURCES}
)

# Create test executable
add_executable(codec_test ${TEST_SOURCES})

# Include directories
target_include_directories(codec_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../opus/include
    ${CMAKE_CURRENT_LIST_DIR}/../../log
    ${LINX_CODEC_INCLUDE_DIRS}
)

# Link with the codec library and required dependencies
target_link_libraries(codec_test
    ${SDK_DIR}/third/opus/install/lib/libopus.a
    m  # Math library for sin/cos functions in test
)

# Compiler flags
target_compile_features(codec_test PRIVATE c_std_99)
target_compile_options(codec_test PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# Add definitions
target_compile_definitions(codec_test PRIVATE
    OPUS_BUILD
)

# Enable testing
enable_testing()

# Add test
add_test(NAME codec_basic_test COMMAND codec_test)

# Set test properties
set_tests_properties(codec_basic_test PROPERTIES
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Custom target to run tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS codec_test
    COMMENT "Running codec tests"
)