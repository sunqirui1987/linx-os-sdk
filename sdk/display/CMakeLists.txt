cmake_minimum_required(VERSION 3.10)



# 显示库通用源文件
set(DISPLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/display_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/theme/theme.c
)

set(DISPLAY_HEADERS
    display.h
    display_stub.h
    theme/theme.h
)

# 添加子目录
add_subdirectory(lvgl_display)

# 合并子模块的源文件
set(DISPLAY_SOURCES
    ${DISPLAY_SOURCES}
    ${LINX_LVGL_DISPLAY_SOURCES}
)

set(DISPLAY_HEADERS
    ${DISPLAY_HEADERS}
    ${LINX_LVGL_DISPLAY_HEADERS}
)

# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将显示模块的源文件列表设置为父作用域变量
set(LINX_DISPLAY_SOURCES ${DISPLAY_SOURCES} PARENT_SCOPE)

# 将显示模块的头文件列表设置为父作用域变量  
set(LINX_DISPLAY_HEADERS ${DISPLAY_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_DISPLAY_PLATFORM_LIBS ${DISPLAY_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_DISPLAY_INCLUDE_DIRS ${DISPLAY_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_DISPLAY_LIBRARY_DIRS ${DISPLAY_LIBRARY_DIRS} PARENT_SCOPE)