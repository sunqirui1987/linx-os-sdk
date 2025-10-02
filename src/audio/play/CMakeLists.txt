cmake_minimum_required(VERSION 3.10)



# 播放器库通用源文件
set(PLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/linx_player.c
)

set(PLAY_HEADERS
    linx_player.h
)


# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将播放器模块的源文件列表设置为父作用域变量
set(LINX_PLAY_SOURCES ${PLAY_SOURCES} PARENT_SCOPE)

# 将播放器模块的头文件列表设置为父作用域变量  
set(LINX_PLAY_HEADERS ${PLAY_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_PLAY_PLATFORM_LIBS ${PLAY_PLATFORM_LIBS} PARENT_SCOPE)