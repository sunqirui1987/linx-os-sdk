cmake_minimum_required(VERSION 3.10)
project(linx_test_camera C)

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

# Don't add the camera directory as a subdirectory since we're already in the camera test directory
# This was causing the recursion error
message(STATUS "Building test camera ${SDK_DIR} ")

# Test executable sources
set(TEST_SOURCES
    camera_mac_explain.c
    ../camera_interface.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../board/mac/common/camera/camera_mac.c
    ${CMAKE_CURRENT_LIST_DIR}/../../log/linx_log.c
    ${CMAKE_CURRENT_LIST_DIR}/../../cjson/cJSON.c
)

# GUI test executable sources
set(GUI_TEST_SOURCES
    camera_mac_explain_gui.c
    ../camera_interface.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../board/mac/common/camera/camera_mac.c
    ${CMAKE_CURRENT_LIST_DIR}/../../log/linx_log.c
    ${CMAKE_CURRENT_LIST_DIR}/../../cjson/cJSON.c
)

# Create test executables
add_executable(camera_mac_explain ${TEST_SOURCES})
add_executable(camera_mac_explain_gui ${GUI_TEST_SOURCES})

# Include directories
target_include_directories(camera_mac_explain PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../../
    ${CMAKE_CURRENT_LIST_DIR}/../../log
    ${CMAKE_CURRENT_LIST_DIR}/../../cjson
    ${CMAKE_CURRENT_LIST_DIR}/../../third/mongoose
    ${CMAKE_CURRENT_LIST_DIR}/../../../board/mac/common/camera
)

# Include directories for GUI
target_include_directories(camera_mac_explain_gui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../../
    ${CMAKE_CURRENT_LIST_DIR}/../../log
    ${CMAKE_CURRENT_LIST_DIR}/../../cjson
    ${CMAKE_CURRENT_LIST_DIR}/../../third/mongoose
    ${CMAKE_CURRENT_LIST_DIR}/../../../board/mac/common/camera
    ${SDK_INCLUDE_DIR}/third/liblvgl
    ${SDK_INCLUDE_DIR}/third/liblvgl/src
    ${SDK_INCLUDE_DIR}/third/liblvgl/conf
)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Ensure we use the correct SDL2 library path for arm64 on macOS
if(APPLE)
    # Force use of Homebrew SDL2 on Apple Silicon
    set(SDL2_LIBRARY_DIRS "/opt/homebrew/lib")
    set(SDL2_INCLUDE_DIRS "/opt/homebrew/include/SDL2")
    set(SDL2_LIBRARIES "-L/opt/homebrew/lib -lSDL2")
endif()

# Link libraries for macOS
if(APPLE)
    target_link_libraries(camera_mac_explain PRIVATE
        ${SDK_LIB_DIR}/libmongoose.a
        "-framework Foundation"
        "-framework CoreGraphics"
        "-framework CoreFoundation"
        "-framework ImageIO"
    )
    
    target_link_libraries(camera_mac_explain_gui PRIVATE
        ${SDK_LIB_DIR}/libmongoose.a
        ${SDK_LIB_DIR}/libv9.a
        ${SDL2_LIBRARIES}
        "-framework Foundation"
        "-framework CoreGraphics"
        "-framework CoreFoundation"
        "-framework ImageIO"
        "-framework Cocoa"
        "-framework OpenGL"
    )
    
    target_include_directories(camera_mac_explain_gui PRIVATE ${SDL2_INCLUDE_DIRS})
    target_compile_options(camera_mac_explain_gui PRIVATE ${SDL2_CFLAGS_OTHER})
else()
    target_link_libraries(camera_mac_explain PRIVATE
        ${SDK_LIB_DIR}/libmongoose.a
    )
    
    target_link_libraries(camera_mac_explain_gui PRIVATE
        ${SDK_LIB_DIR}/libmongoose.a
        ${SDK_LIB_DIR}/libv9.a
        ${SDL2_LIBRARIES}
    )
    
    target_include_directories(camera_mac_explain_gui PRIVATE ${SDL2_INCLUDE_DIRS})
    target_compile_options(camera_mac_explain_gui PRIVATE ${SDL2_CFLAGS_OTHER})
endif()

# Install test binaries
install(TARGETS camera_mac_explain camera_mac_explain_gui DESTINATION bin)