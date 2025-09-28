cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(http_image_upload_test)

# 设置C标准
set(CMAKE_C_STANDARD 99)

# 添加编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# 查找必要的库
find_package(PkgConfig REQUIRED)

# 添加包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third/mongoose
)

# 添加源文件
set(SOURCES
    http_image_upload_test.c
    ../http_client.c
    ../../log/linx_log.c
    ../../third/mongoose/mongoose.c
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
if(APPLE)
    # macOS特定的链接库
    target_link_libraries(${PROJECT_NAME} 
        "-framework CoreFoundation"
        "-framework Security"
    )
elseif(UNIX)
    # Linux特定的链接库
    target_link_libraries(${PROJECT_NAME} 
        pthread
        m
    )
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# 添加编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    MG_ENABLE_OPENSSL=0
    MG_ENABLE_MBEDTLS=0
)

# 打印一些信息
message(STATUS "Building HTTP Image Upload Test")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")