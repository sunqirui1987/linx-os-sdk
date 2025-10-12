# LinxOS Audio - Built-in Plugins CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# 设置项目名称
project(LinxAudioBuiltinPlugins VERSION 1.0.0 LANGUAGES C)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 源文件
set(BUILTIN_PLUGIN_SOURCES
    gain.c
    equalizer.c
    delay.c
    reverb.c
)

# 头文件
set(BUILTIN_PLUGIN_HEADERS
    ../plugin_interface.h
    ../../core/types.h
)

# 创建静态库
add_library(LinxAudioBuiltinPlugins_static STATIC
    ${BUILTIN_PLUGIN_SOURCES}
    ${BUILTIN_PLUGIN_HEADERS}
)

# 设置库属性
set_target_properties(LinxAudioBuiltinPlugins_static PROPERTIES
    OUTPUT_NAME "linx_audio_builtin_plugins"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# 链接数学库
target_link_libraries(LinxAudioBuiltinPlugins_static m)

# 平台特定设置
if(APPLE)
    target_compile_definitions(LinxAudioBuiltinPlugins_static PRIVATE
        LINX_PLATFORM_MACOS=1
        _DARWIN_C_SOURCE
    )
elseif(UNIX)
    target_compile_definitions(LinxAudioBuiltinPlugins_static PRIVATE
        LINX_PLATFORM_LINUX=1
        _GNU_SOURCE
    )
    # 链接pthread库
    target_link_libraries(LinxAudioBuiltinPlugins_static pthread)
endif()

# 创建共享库（可选）
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
if(BUILD_SHARED_LIBS)
    add_library(LinxAudioBuiltinPlugins_shared SHARED
        ${BUILTIN_PLUGIN_SOURCES}
        ${BUILTIN_PLUGIN_HEADERS}
    )
    
    set_target_properties(LinxAudioBuiltinPlugins_shared PROPERTIES
        OUTPUT_NAME "linx_audio_builtin_plugins"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
    )
    
    # 链接数学库
    target_link_libraries(LinxAudioBuiltinPlugins_shared m)
    
    # 平台特定设置
    if(APPLE)
        target_compile_definitions(LinxAudioBuiltinPlugins_shared PRIVATE
            LINX_PLATFORM_MACOS=1
            _DARWIN_C_SOURCE
        )
    elseif(UNIX)
        target_compile_definitions(LinxAudioBuiltinPlugins_shared PRIVATE
            LINX_PLATFORM_LINUX=1
            _GNU_SOURCE
        )
        target_link_libraries(LinxAudioBuiltinPlugins_shared pthread)
    endif()
endif()

# 安装设置
install(TARGETS LinxAudioBuiltinPlugins_static
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_SHARED_LIBS)
    install(TARGETS LinxAudioBuiltinPlugins_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# 创建插件注册头文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/builtin_plugins.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/builtin_plugins.h
    @ONLY
)

# 安装头文件
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/builtin_plugins.h
    DESTINATION include/linx/audio/plugins/builtin
)

# 测试（可选）
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    
    # 添加测试可执行文件
    add_executable(test_builtin_plugins
        test/test_gain.c
        test/test_equalizer.c
        test/test_delay.c
        test/test_reverb.c
        test/test_main.c
    )
    
    target_link_libraries(test_builtin_plugins
        LinxAudioBuiltinPlugins_static
        m
    )
    
    if(UNIX AND NOT APPLE)
        target_link_libraries(test_builtin_plugins pthread)
    endif()
    
    add_test(NAME BuiltinPluginsTest COMMAND test_builtin_plugins)
endif()

# 示例程序（可选）
option(BUILD_EXAMPLES "Build examples" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 打印构建信息
message(STATUS "LinxOS Audio Built-in Plugins Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")