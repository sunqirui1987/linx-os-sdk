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
# 源文件收集
# ============================================================================

# 递归获取所有.c源文件
file(GLOB_RECURSE ALL_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# 排除test开头的目录
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/test[^/]*/.+")

# 排除build开头的目录
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/build[^/]*/.+")

# 排除examples目录（如果存在）
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/examples/.+")

# 打印找到的源文件
message(STATUS "Found source files:")
foreach(source ${ALL_SOURCES})
    file(RELATIVE_PATH rel_source ${CMAKE_CURRENT_SOURCE_DIR} ${source})
    message(STATUS "  ${rel_source}")
endforeach()


# ============================================================================
# 库目标
# ============================================================================

# 构建选项
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LINX_AUDIO_BUILD_STATIC "Build static library" ON)
option(LINX_AUDIO_BUILD_SHARED "Build shared library" ${BUILD_SHARED_LIBS})

# 静态库
if(LINX_AUDIO_BUILD_STATIC)
    add_library(LinxAudioSystem_static STATIC ${ALL_SOURCES})
    set_target_properties(LinxAudioSystem_static PROPERTIES
        OUTPUT_NAME LinxAudioSystem
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    
    # 链接库
    target_link_libraries(LinxAudioSystem_static 
        Threads::Threads
    )
    
    if(MATH_LIBRARY)
        target_link_libraries(LinxAudioSystem_static ${MATH_LIBRARY})
    endif()
    
    message(STATUS "Building static library: libLinxAudioSystem.a")
endif()

# 共享库
if(LINX_AUDIO_BUILD_SHARED)
    add_library(LinxAudioSystem_shared SHARED ${ALL_SOURCES})
    set_target_properties(LinxAudioSystem_shared PROPERTIES
        OUTPUT_NAME LinxAudioSystem
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    
    # 链接库
    target_link_libraries(LinxAudioSystem_shared 
        Threads::Threads
    )
    
    if(MATH_LIBRARY)
        target_link_libraries(LinxAudioSystem_shared ${MATH_LIBRARY})
    endif()
    
    message(STATUS "Building shared library: libLinxAudioSystem.so")
endif()

# ============================================================================
# 平台特定链接
# ============================================================================

if(LINX_PLATFORM_MACOS)
    if(COREAUDIO_FRAMEWORK AND AUDIOTOOLBOX_FRAMEWORK)
        if(LINX_AUDIO_BUILD_STATIC)
            target_link_libraries(LinxAudioSystem_static 
                ${COREAUDIO_FRAMEWORK}
                ${AUDIOTOOLBOX_FRAMEWORK}
            )
        endif()
        if(LINX_AUDIO_BUILD_SHARED)
            target_link_libraries(LinxAudioSystem_shared 
                ${COREAUDIO_FRAMEWORK}
                ${AUDIOTOOLBOX_FRAMEWORK}
            )
        endif()
    endif()
endif()

if(LINX_PLATFORM_LINUX)
    if(ALSA_FOUND)
        if(LINX_AUDIO_BUILD_STATIC)
            target_link_libraries(LinxAudioSystem_static ${ALSA_LIBRARIES})
            target_include_directories(LinxAudioSystem_static PRIVATE ${ALSA_INCLUDE_DIRS})
        endif()
        if(LINX_AUDIO_BUILD_SHARED)
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
    if(LINX_AUDIO_BUILD_STATIC)
        target_compile_definitions(LinxAudioSystem_static PRIVATE
            LINX_PLATFORM_MACOS=1
            _DARWIN_C_SOURCE
        )
    endif()
    if(LINX_AUDIO_BUILD_SHARED)
        target_compile_definitions(LinxAudioSystem_shared PRIVATE
            LINX_PLATFORM_MACOS=1
            _DARWIN_C_SOURCE
        )
    endif()
endif()

if(LINX_PLATFORM_LINUX)
    if(LINX_AUDIO_BUILD_STATIC)
        target_compile_definitions(LinxAudioSystem_static PRIVATE
            LINX_PLATFORM_LINUX=1
            _GNU_SOURCE
        )
    endif()
    if(LINX_AUDIO_BUILD_SHARED)
        target_compile_definitions(LinxAudioSystem_shared PRIVATE
            LINX_PLATFORM_LINUX=1
            _GNU_SOURCE
        )
    endif()
endif()

if(LINX_PLATFORM_ESP32)
    if(LINX_AUDIO_BUILD_STATIC)
        target_compile_definitions(LinxAudioSystem_static PRIVATE
            LINX_PLATFORM_ESP32=1
        )
    endif()
    if(LINX_AUDIO_BUILD_SHARED)
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
if(LINX_AUDIO_BUILD_STATIC)
    install(TARGETS LinxAudioSystem_static
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

if(LINX_AUDIO_BUILD_SHARED)
    install(TARGETS LinxAudioSystem_shared
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

message(STATUS "")
message(STATUS "LinxOS Audio System Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Static: ${LINX_AUDIO_BUILD_STATIC}")
message(STATUS "  Build Shared: ${LINX_AUDIO_BUILD_SHARED}")
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
message(STATUS "")