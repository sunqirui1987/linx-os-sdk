cmake_minimum_required(VERSION 3.10)

# 添加 cmake 模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

# 查找 Mongoose 库
set(MONGOOSE_INCLUDE_DIRS, ${CMAKE_CURRENT_SOURCE_DIR}/../mongoose/mongoose)

# OTA库源文件
set(OTA_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/linx_ota.c
)

set(OTA_HEADERS
    linx_ota.h
)

# 包含目录
set(OTA_INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/../log
    ${MONGOOSE_INCLUDE_DIRS}
)
set(OTA_PLATFORM_LIBS mongoose)
set(OTA_LIBRARY_DIRS ${MONGOOSE_LIBRARIES})

# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将OTA模块的源文件列表设置为父作用域变量
set(LINX_OTA_SOURCES ${OTA_SOURCES} PARENT_SCOPE)

# 将OTA模块的头文件列表设置为父作用域变量  
set(LINX_OTA_HEADERS ${OTA_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_OTA_PLATFORM_LIBS ${OTA_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_OTA_INCLUDE_DIRS ${OTA_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_OTA_LIBRARY_DIRS ${OTA_LIBRARY_DIRS} PARENT_SCOPE)
