cmake_minimum_required(VERSION 3.10)



# LVGL显示库通用源文件
set(LVGL_DISPLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_font.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_image.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_theme.c
    ${CMAKE_CURRENT_SOURCE_DIR}/emoji_collection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gif/gifdec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gif/lvgl_gif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/jpg/image_to_jpeg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/jpg/jpeg_encoder.c
)

set(LVGL_DISPLAY_HEADERS
    lvgl_display.h
    lvgl_font.h
    lvgl_image.h
    lvgl_theme.h
    emoji_collection.h
    gif/gifdec.h
    gif/gifdec_mve.h
    gif/lvgl_gif.h
    jpg/image_to_jpeg.h
    jpg/jpeg_encoder.h
)


# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将LVGL显示模块的源文件列表设置为父作用域变量
set(LINX_LVGL_DISPLAY_SOURCES ${LVGL_DISPLAY_SOURCES} PARENT_SCOPE)

# 将LVGL显示模块的头文件列表设置为父作用域变量  
set(LINX_LVGL_DISPLAY_HEADERS ${LVGL_DISPLAY_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_LVGL_DISPLAY_PLATFORM_LIBS ${LVGL_DISPLAY_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_LVGL_DISPLAY_INCLUDE_DIRS ${LVGL_DISPLAY_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_LVGL_DISPLAY_LIBRARY_DIRS ${LVGL_DISPLAY_LIBRARY_DIRS} PARENT_SCOPE)