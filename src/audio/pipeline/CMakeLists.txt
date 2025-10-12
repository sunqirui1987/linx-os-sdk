# LinxOS Audio Pipeline CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

project(LinxAudioPipeline VERSION 1.0.0 LANGUAGES C)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
)

# 源文件
set(PIPELINE_SOURCES
    audio_pipeline.c
)

# 头文件
set(PIPELINE_HEADERS
    audio_pipeline.h
)

# 创建静态库
add_library(LinxAudioPipeline_static STATIC ${PIPELINE_SOURCES})
target_link_libraries(LinxAudioPipeline_static pthread m)

# 设置库属性
set_target_properties(LinxAudioPipeline_static PROPERTIES
    OUTPUT_NAME linx_audio_pipeline
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 可选：创建共享库
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    add_library(LinxAudioPipeline_shared SHARED ${PIPELINE_SOURCES})
    target_link_libraries(LinxAudioPipeline_shared pthread m)
    
    set_target_properties(LinxAudioPipeline_shared PROPERTIES
        OUTPUT_NAME linx_audio_pipeline
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# 平台特定设置
if(APPLE)
    target_compile_definitions(LinxAudioPipeline_static PRIVATE LINX_PLATFORM_MACOS=1)
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(LinxAudioPipeline_shared PRIVATE LINX_PLATFORM_MACOS=1)
    endif()
elseif(UNIX)
    target_compile_definitions(LinxAudioPipeline_static PRIVATE LINX_PLATFORM_LINUX=1)
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(LinxAudioPipeline_shared PRIVATE LINX_PLATFORM_LINUX=1)
    endif()
endif()

# 安装规则
install(TARGETS LinxAudioPipeline_static
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_SHARED_LIBS)
    install(TARGETS LinxAudioPipeline_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

install(FILES ${PIPELINE_HEADERS}
    DESTINATION include/linx/audio/pipeline
)

# 测试选项
option(BUILD_PIPELINE_TESTS "Build pipeline tests" OFF)
if(BUILD_PIPELINE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例选项
option(BUILD_PIPELINE_EXAMPLES "Build pipeline examples" OFF)
if(BUILD_PIPELINE_EXAMPLES)
    add_subdirectory(examples)
endif()