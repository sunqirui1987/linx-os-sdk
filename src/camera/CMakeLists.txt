cmake_minimum_required(VERSION 3.10)

# 相机库通用源文件
set(CAMERA_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_stub.c
)

set(CAMERA_HEADERS
    camera_interface.h
    camera_stub.h
)

# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将相机模块的源文件列表设置为父作用域变量
set(LINX_CAMERA_SOURCES ${CAMERA_SOURCES} PARENT_SCOPE)

# 将相机模块的头文件列表设置为父作用域变量  
set(LINX_CAMERA_HEADERS ${CAMERA_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_CAMERA_PLATFORM_LIBS ${CAMERA_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_CAMERA_INCLUDE_DIRS ${CAMERA_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_CAMERA_LIBRARY_DIRS ${CAMERA_LIBRARY_DIRS} PARENT_SCOPE)