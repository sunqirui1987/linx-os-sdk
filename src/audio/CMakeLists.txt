# LinxOS Audio System CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

project(LinxAudioSystem 
    VERSION 1.0.0
    DESCRIPTION "LinxOS Audio System"
    LANGUAGES C
)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译器标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/manager
)

# ============================================================================
# 平台检测和配置
# ============================================================================

# 检测操作系统
if(APPLE)
    set(LINX_PLATFORM_MACOS ON)
    add_definitions(-DLINX_PLATFORM_MACOS=1)
elseif(UNIX AND NOT APPLE)
    set(LINX_PLATFORM_LINUX ON)
    add_definitions(-DLINX_PLATFORM_LINUX=1)
endif()

# ESP32平台检测
if(DEFINED ENV{IDF_PATH})
    set(LINX_PLATFORM_ESP32 ON)
    add_definitions(-DLINX_PLATFORM_ESP32=1)
endif()

# ============================================================================
# 依赖库查找
# ============================================================================

# 线程库
find_package(Threads REQUIRED)

# 数学库
find_library(MATH_LIBRARY m)

# 平台特定库
if(LINX_PLATFORM_MACOS)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    if(NOT COREAUDIO_FRAMEWORK OR NOT AUDIOTOOLBOX_FRAMEWORK)
        message(WARNING "CoreAudio or AudioToolbox framework not found")
    else()
        add_definitions(-DLINX_HAVE_COREAUDIO=1)
    endif()
endif()

if(LINX_PLATFORM_LINUX)
    # 查找ALSA
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ALSA alsa)
        if(ALSA_FOUND)
            add_definitions(-DLINX_HAVE_ALSA=1)
        endif()
    endif()
endif()

# ============================================================================
# 源文件列表
# ============================================================================

# 核心源文件
set(CORE_SOURCES
    core/audio_manager.c
    core/event_bus.c
    core/stream_manager.c
)

# 音频工具源文件
set(UTILS_SOURCES
    utils/audio_utils.c
)

# 驱动源文件
set(DRIVER_SOURCES
    drivers/driver_factory.c
    drivers/dummy_driver.c
)

# 平台特定驱动
if(LINX_PLATFORM_MACOS AND COREAUDIO_FRAMEWORK AND AUDIOTOOLBOX_FRAMEWORK)
    list(APPEND DRIVER_SOURCES drivers/coreaudio/coreaudio_driver.c)
endif()

if(LINX_PLATFORM_LINUX AND ALSA_FOUND)
    list(APPEND DRIVER_SOURCES drivers/alsa/alsa_driver.c)
endif()

if(LINX_PLATFORM_ESP32)
    list(APPEND DRIVER_SOURCES drivers/esp32/esp32_driver.c)
endif()

# 管理器源文件（已包含在CORE_SOURCES中）
set(MANAGER_SOURCES
    # audio_manager.c 已在 CORE_SOURCES 中
)

# 管道源文件
set(PIPELINE_SOURCES
    pipeline/audio_pipeline.c
)

# 内置插件源文件
set(PLUGIN_SOURCES
    plugins/plugin_manager.c
    plugins/builtin/builtin_plugins.c
    plugins/builtin/gain.c
    plugins/builtin/equalizer.c
    plugins/builtin/delay.c
    plugins/builtin/reverb.c
)

# 所有源文件
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
    ${DRIVER_SOURCES}
    ${MANAGER_SOURCES}
    ${PIPELINE_SOURCES}
    ${PLUGIN_SOURCES}
)

# 头文件列表
set(PUBLIC_HEADERS
    core/types.h
    drivers/audio_driver.h
    manager/audio_manager.h
    pipeline/audio_pipeline.h
    plugins/builtin/builtin_plugins.h
    utils/audio_utils.h
)

# ============================================================================
# 库目标
# ============================================================================

# 静态库
add_library(LinxAudioSystem_static STATIC ${ALL_SOURCES})
set_target_properties(LinxAudioSystem_static PROPERTIES
    OUTPUT_NAME LinxAudioSystem
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 共享库（可选）
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    add_library(LinxAudioSystem_shared SHARED ${ALL_SOURCES})
    set_target_properties(LinxAudioSystem_shared PROPERTIES
        OUTPUT_NAME LinxAudioSystem
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# ============================================================================
# 链接库
# ============================================================================

# 静态库链接
target_link_libraries(LinxAudioSystem_static 
    Threads::Threads
)

if(MATH_LIBRARY)
    target_link_libraries(LinxAudioSystem_static ${MATH_LIBRARY})
endif()

# 平台特定链接
if(LINX_PLATFORM_MACOS)
    if(COREAUDIO_FRAMEWORK AND AUDIOTOOLBOX_FRAMEWORK)
        target_link_libraries(LinxAudioSystem_static 
            ${COREAUDIO_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
        )
    endif()
endif()

if(LINX_PLATFORM_LINUX)
    if(ALSA_FOUND)
        target_link_libraries(LinxAudioSystem_static ${ALSA_LIBRARIES})
        target_include_directories(LinxAudioSystem_static PRIVATE ${ALSA_INCLUDE_DIRS})
    endif()
endif()

# 共享库链接
if(BUILD_SHARED_LIBS)
    target_link_libraries(LinxAudioSystem_shared 
        Threads::Threads
    )
    
    if(MATH_LIBRARY)
        target_link_libraries(LinxAudioSystem_shared ${MATH_LIBRARY})
    endif()
    
    if(LINX_PLATFORM_MACOS)
        if(COREAUDIO_FRAMEWORK AND AUDIOTOOLBOX_FRAMEWORK)
            target_link_libraries(LinxAudioSystem_shared 
                ${COREAUDIO_FRAMEWORK}
                ${AUDIOTOOLBOX_FRAMEWORK}
            )
        endif()
    endif()
    
    if(LINX_PLATFORM_LINUX)
        if(ALSA_FOUND)
            target_link_libraries(LinxAudioSystem_shared ${ALSA_LIBRARIES})
            target_include_directories(LinxAudioSystem_shared PRIVATE ${ALSA_INCLUDE_DIRS})
        endif()
    endif()
endif()

# ============================================================================
# 编译定义
# ============================================================================

# 平台特定定义
if(LINX_PLATFORM_MACOS)
    target_compile_definitions(LinxAudioSystem_static PRIVATE
        LINX_PLATFORM_MACOS=1
        _DARWIN_C_SOURCE
    )
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(LinxAudioSystem_shared PRIVATE
            LINX_PLATFORM_MACOS=1
            _DARWIN_C_SOURCE
        )
    endif()
endif()

if(LINX_PLATFORM_LINUX)
    target_compile_definitions(LinxAudioSystem_static PRIVATE
        LINX_PLATFORM_LINUX=1
        _GNU_SOURCE
    )
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(LinxAudioSystem_shared PRIVATE
            LINX_PLATFORM_LINUX=1
            _GNU_SOURCE
        )
    endif()
endif()

if(LINX_PLATFORM_ESP32)
    target_compile_definitions(LinxAudioSystem_static PRIVATE
        LINX_PLATFORM_ESP32=1
    )
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(LinxAudioSystem_shared PRIVATE
            LINX_PLATFORM_ESP32=1
        )
    endif()
endif()

# ============================================================================
# 示例和测试
# ============================================================================

# 示例选项
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# 测试选项
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# 安装规则
# ============================================================================

# 安装库文件
install(TARGETS LinxAudioSystem_static
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_SHARED_LIBS)
    install(TARGETS LinxAudioSystem_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# 安装头文件
install(FILES ${PUBLIC_HEADERS}
    DESTINATION include/linx/audio
)

# 安装pkg-config文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/linx-audio.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/linx-audio.pc
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/linx-audio.pc
    DESTINATION lib/pkgconfig
)

# ============================================================================
# 信息输出
# ============================================================================

message(STATUS "LinxOS Audio System Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Shared: ${BUILD_SHARED_LIBS}")
message(STATUS "  Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")

if(LINX_PLATFORM_MACOS)
    message(STATUS "  CoreAudio Support: ${COREAUDIO_FRAMEWORK}")
endif()

if(LINX_PLATFORM_LINUX)
    message(STATUS "  ALSA Support: ${ALSA_FOUND}")
endif()

if(LINX_PLATFORM_ESP32)
    message(STATUS "  ESP32 Support: ON")
endif()

message(STATUS "  Drivers: dummy")
if(LINX_PLATFORM_MACOS AND COREAUDIO_FRAMEWORK)
    message(STATUS "           coreaudio")
endif()
if(LINX_PLATFORM_LINUX AND ALSA_FOUND)
    message(STATUS "           alsa")
endif()
if(LINX_PLATFORM_ESP32)
    message(STATUS "           esp32")
endif()

# 编译器设置
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# 平台检测
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DLINX_AUDIO_PLATFORM_LINUX=1 -D_GNU_SOURCE)
    message(STATUS "Platform: Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(-DLINX_AUDIO_PLATFORM_MACOS=1)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14")
    message(STATUS "Platform: macOS")
elseif(ESP_PLATFORM)
    add_definitions(-DLINX_AUDIO_PLATFORM_ESP32=1 -DLINX_AUDIO_EMBEDDED=1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
    message(STATUS "Platform: ESP32")
endif()

# 查找系统库
find_package(Threads REQUIRED)
set(LINX_AUDIO_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

# 查找数学库
if(NOT ESP_PLATFORM)
    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        list(APPEND LINX_AUDIO_LIBRARIES ${MATH_LIBRARY})
    endif()
endif()

# 平台特定库
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # 查找ALSA
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ALSA alsa)
        if(ALSA_FOUND)
            add_definitions(-DLINX_AUDIO_HAVE_ALSA=1)
            list(APPEND LINX_AUDIO_LIBRARIES ${ALSA_LIBRARIES})
            include_directories(${ALSA_INCLUDE_DIRS})
            message(STATUS "ALSA: Found")
        endif()
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # 查找CoreAudio框架
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    if(COREAUDIO_FRAMEWORK AND AUDIOTOOLBOX_FRAMEWORK)
        add_definitions(-DLINX_AUDIO_HAVE_COREAUDIO=1)
        list(APPEND LINX_AUDIO_LIBRARIES ${COREAUDIO_FRAMEWORK} ${AUDIOTOOLBOX_FRAMEWORK})
        message(STATUS "CoreAudio: Found")
    endif()
endif()

# 自动收集所有.c源文件
file(GLOB_RECURSE LINX_AUDIO_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# 排除示例和测试文件
list(FILTER LINX_AUDIO_SOURCES EXCLUDE REGEX ".*/examples/.*")
list(FILTER LINX_AUDIO_SOURCES EXCLUDE REGEX ".*/tests/.*")

# 打印找到的源文件
message(STATUS "Found source files:")
foreach(source ${LINX_AUDIO_SOURCES})
    file(RELATIVE_PATH rel_source ${CMAKE_CURRENT_SOURCE_DIR} ${source})
    message(STATUS "  ${rel_source}")
endforeach()

# 创建静态库
if(LINX_AUDIO_BUILD_STATIC)
    add_library(linx_audio STATIC ${LINX_AUDIO_SOURCES})
    target_link_libraries(linx_audio ${LINX_AUDIO_LIBRARIES})
    
    # 设置库属性
    set_target_properties(linx_audio PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME linx_audio
    )
    
    message(STATUS "Building static library: liblinx_audio.a")
endif()

# 创建共享库
if(LINX_AUDIO_BUILD_SHARED)
    add_library(linx_audio_shared SHARED ${LINX_AUDIO_SOURCES})
    target_link_libraries(linx_audio_shared ${LINX_AUDIO_LIBRARIES})
    
    # 设置库属性
    set_target_properties(linx_audio_shared PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME linx_audio
    )
    
    message(STATUS "Building shared library: liblinx_audio.so")
endif()

# 安装库文件
if(LINX_AUDIO_BUILD_STATIC)
    install(TARGETS linx_audio
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
    )
endif()

if(LINX_AUDIO_BUILD_SHARED)
    install(TARGETS linx_audio_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# 安装头文件
install(DIRECTORY core/ DESTINATION include/linx_audio/core
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY utils/ DESTINATION include/linx_audio/utils
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY drivers/ DESTINATION include/linx_audio/drivers
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY plugins/ DESTINATION include/linx_audio/plugins
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY pipeline/ DESTINATION include/linx_audio/pipeline
    FILES_MATCHING PATTERN "*.h"
)

# 打印配置信息
message(STATUS "")
message(STATUS "LinxOS Audio System Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "  Build static library: ${LINX_AUDIO_BUILD_STATIC}")
message(STATUS "  Build shared library: ${LINX_AUDIO_BUILD_SHARED}")
message(STATUS "  Linked libraries: ${LINX_AUDIO_LIBRARIES}")
message(STATUS "")