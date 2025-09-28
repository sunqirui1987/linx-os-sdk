cmake_minimum_required(VERSION 3.10)

# HTTP客户端库通用源文件
set(HTTP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/http_client.c
)

set(HTTP_HEADERS
    http_client.h
)

# HTTP客户端依赖mongoose库
set(HTTP_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/mongoose
)

# 平台特定的库依赖
if(APPLE)
    set(HTTP_PLATFORM_LIBS)
elseif(UNIX)
    set(HTTP_PLATFORM_LIBS)
elseif(WIN32)
    set(HTTP_PLATFORM_LIBS ws2_32)
endif()

# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将HTTP模块的源文件列表设置为父作用域变量
set(LINX_HTTP_SOURCES ${HTTP_SOURCES} PARENT_SCOPE)

# 将HTTP模块的头文件列表设置为父作用域变量  
set(LINX_HTTP_HEADERS ${HTTP_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_HTTP_PLATFORM_LIBS ${HTTP_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_HTTP_INCLUDE_DIRS ${HTTP_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_HTTP_LIBRARY_DIRS ${HTTP_LIBRARY_DIRS} PARENT_SCOPE)