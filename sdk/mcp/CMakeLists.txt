cmake_minimum_required(VERSION 3.10)
project(linx_mcp C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# MCP library sources
set(MCP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp_property.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp_tool.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp_utils.c
)

set(MCP_HEADERS
    mcp.h
    mcp_property.h
    mcp_server.h
    mcp_tool.h
    mcp_types.h
    mcp_utils.h
)

set(MCP_PLATFORM_LIBS "")
set(MCP_INCLUDE_DIRS "")
set(MCP_LIBRARY_DIRS "")
# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================

# 将MCP模块的源文件列表设置为父作用域变量
set(LINX_MCP_SOURCES ${MCP_SOURCES} PARENT_SCOPE)

# 将MCP模块的头文件列表设置为父作用域变量  
set(LINX_MCP_HEADERS ${MCP_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_MCP_PLATFORM_LIBS ${MCP_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_MCP_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${MCP_INCLUDE_DIRS} PARENT_SCOPE)     

# 将库目录设置为父作用域变量
set(LINX_MCP_LIBRARY_DIRS ${MCP_LIBRARY_DIRS} PARENT_SCOPE)

