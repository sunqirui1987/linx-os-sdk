# cJSON Library CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Set library name
set(CJSON_LIB_NAME cjson_static)

# Source files
set(CJSON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/cJSON.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cJSON_Utils.c
)

# Header files
set(CJSON_HEADERS
    cJSON.h
    cJSON_Utils.h
)

# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将 cJSON 模块的源文件列表设置为父作用域变量
set(LINX_CJSON_SOURCES ${CJSON_SOURCES} PARENT_SCOPE)

# 将 cJSON 模块的头文件列表设置为父作用域变量
set(LINX_CJSON_HEADERS ${CJSON_HEADERS} PARENT_SCOPE)



# 将平台特定的库依赖设置为父作用域变量
set(LINX_CJSON_PLATFORM_LIBS ${CJSON_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_CJSON_INCLUDE_DIRS ${CJSON_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_CJSON_LIBRARY_DIRS ${CJSON_LIBRARY_DIRS} PARENT_SCOPE)





