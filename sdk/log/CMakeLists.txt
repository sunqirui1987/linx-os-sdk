cmake_minimum_required(VERSION 3.10)
project(linx_log C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Log library sources
set(LOG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/linx_log.c
)

set(LOG_HEADERS
    linx_log.h
)

# Platform-specific libraries (initialize as empty for log module)
set(LOG_PLATFORM_LIBS "")

# Include directories (current directory for headers)
set(LOG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

# Library directories (initialize as empty for log module)
set(LOG_LIBRARY_DIRS "")

# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将日志模块的源文件列表设置为父作用域变量
set(LINX_LOG_SOURCES ${LOG_SOURCES} PARENT_SCOPE)

# 将日志模块的头文件列表设置为父作用域变量  
set(LINX_LOG_HEADERS ${LOG_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_LOG_PLATFORM_LIBS ${LOG_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_LOG_INCLUDE_DIRS ${LOG_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_LOG_LIBRARY_DIRS ${LOG_LIBRARY_DIRS} PARENT_SCOPE)

