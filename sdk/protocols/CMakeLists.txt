cmake_minimum_required(VERSION 3.10)

# 添加 cmake 模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

# 查找 Mongoose 库
set(MONGOOSE_INCLUDE_DIRS, ${CMAKE_CURRENT_SOURCE_DIR}/../mongoose/mongoose)

# 协议库源文件
set(PROTOCOLS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/linx_protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/linx_websocket.c
)

set(PROTOCOLS_HEADERS
    linx_protocol.h
    linx_websocket.h
)


# 包含目录
set(PROTOCOLS_INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/../log
    ${MONGOOSE_INCLUDE_DIRS}
)
set(PROTOCOLS_PLATFORM_LIBS mongoose)
set(PROTOCOLS_LIBRARY_DIRS ${MONGOOSE_LIBRARIES})




# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================

# 将协议模块的源文件列表设置为父作用域变量
set(LINX_PROTOCOLS_SOURCES ${PROTOCOLS_SOURCES} PARENT_SCOPE)

# 将协议模块的头文件列表设置为父作用域变量  
set(LINX_PROTOCOLS_HEADERS ${PROTOCOLS_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_PROTOCOLS_PLATFORM_LIBS ${PROTOCOLS_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_PROTOCOLS_INCLUDE_DIRS  ${PROTOCOLS_INCLUDE_DIRS} PARENT_SCOPE)     

# 将库目录设置为父作用域变量
set(LINX_PROTOCOLS_LIBRARY_DIRS ${PROTOCOLS_LIBRARY_DIRS} PARENT_SCOPE)
