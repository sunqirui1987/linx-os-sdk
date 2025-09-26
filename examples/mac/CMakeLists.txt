cmake_minimum_required(VERSION 3.16)
project(linx_demo)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# 设置 SDK 路径
set(SDK_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../out/linx")
set(SDK_INCLUDE_DIR "${SDK_INSTALL_DIR}/include")
set(SDK_LIB_DIR "${SDK_INSTALL_DIR}/lib")
set(OPUS_INCLUDE_DIR "${SDK_INSTALL_DIR}/third/opus/include/opus")
set(MONGOOSE_INCLUDE_DIR "${SDK_INSTALL_DIR}/third/mongoose/include")
set(OPUS_LIB_DIR "${SDK_INSTALL_DIR}/third/opus/lib")
set(MONGOOSE_LIB_DIR "${SDK_INSTALL_DIR}/third/mongoose/lib")

message(STATUS "SDK install directory: ${SDK_INSTALL_DIR}")
message(STATUS "SDK include directory: ${SDK_INCLUDE_DIR}")
message(STATUS "SDK lib directory: ${SDK_LIB_DIR}")
message(STATUS "Opus include directory: ${OPUS_INCLUDE_DIR}")
message(STATUS "Mongoose include directory: ${MONGOOSE_INCLUDE_DIR}")

# 检查 SDK 安装目录是否存在
if(NOT EXISTS ${SDK_INSTALL_DIR})
    message(FATAL_ERROR "SDK install directory not found: ${SDK_INSTALL_DIR}")
endif()

# 检查头文件目录是否存在
if(NOT EXISTS ${SDK_INCLUDE_DIR})
    message(FATAL_ERROR "SDK include directory not found: ${SDK_INCLUDE_DIR}")
endif()

if(NOT EXISTS ${OPUS_INCLUDE_DIR})
    message(FATAL_ERROR "Opus include directory not found: ${OPUS_INCLUDE_DIR}")
endif()

if(NOT EXISTS ${MONGOOSE_INCLUDE_DIR})
    message(FATAL_ERROR "Mongoose include directory not found: ${MONGOOSE_INCLUDE_DIR}")
endif()

# 检查库文件目录是否存在
if(NOT EXISTS ${SDK_LIB_DIR})
    message(FATAL_ERROR "SDK lib directory not found: ${SDK_LIB_DIR}")
endif()

# 添加头文件搜索路径
include_directories(${SDK_INCLUDE_DIR})
include_directories(${OPUS_INCLUDE_DIR})
include_directories(${MONGOOSE_INCLUDE_DIR})

# 添加可执行文件
add_executable(linx_demo linx_demo.c)

# 链接 SDK 静态库
target_link_libraries(linx_demo 
    ${SDK_LIB_DIR}/liblinx_sdk_static.a
    ${MONGOOSE_LIB_DIR}/libmongoose.a
    ${OPUS_LIB_DIR}/libopus.a
)

# 查找并链接系统依赖库
find_package(PkgConfig REQUIRED)

# 查找 PortAudio
find_library(PORTAUDIO_LIBRARY portaudio)
if(PORTAUDIO_LIBRARY)
    target_link_libraries(linx_demo ${PORTAUDIO_LIBRARY})
else()
    message(WARNING "PortAudio library not found, you may need to install it")
endif()

# 查找 Opus
find_library(OPUS_LIBRARY opus)
if(OPUS_LIBRARY)
    target_link_libraries(linx_demo ${OPUS_LIBRARY})
else()
    message(WARNING "Opus library not found, you may need to install it")
endif()

# macOS 系统框架
if(APPLE)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    
    target_link_libraries(linx_demo 
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
    )
endif()

# 链接 pthread
find_package(Threads REQUIRED)
target_link_libraries(linx_demo Threads::Threads)

# 设置输出目录
set_target_properties(linx_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 打印配置信息
message(STATUS "SDK Install Directory: ${SDK_INSTALL_DIR}")
message(STATUS "SDK Include Directory: ${SDK_INCLUDE_DIR}")
message(STATUS "SDK Library Directory: ${SDK_LIB_DIR}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# 安装规则（可选）
install(TARGETS linx_demo
    RUNTIME DESTINATION bin
)