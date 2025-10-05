#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file vector.h
 * @brief 通用动态数组实现，支持任意类型
 * @details 使用宏定义实现类似C++模板的功能，支持任意数据类型的动态数组
 */

// ============================================================================
// 通用vector结构体定义
// ============================================================================

/**
 * @brief 通用动态数组结构体
 * @details 使用void指针存储任意类型的数据
 */
typedef struct {
    void* data;             /**< 数据指针 */
    size_t size;            /**< 当前元素数量 */
    size_t capacity;        /**< 当前容量 */
    size_t element_size;    /**< 单个元素大小 */
} vector_t;

// ============================================================================
// 基础函数声明
// ============================================================================

/**
 * @brief 初始化vector
 * @param vec vector指针
 * @param element_size 元素大小
 * @return 成功返回0，失败返回-1
 */
int vector_init(vector_t* vec, size_t element_size);

/**
 * @brief 初始化vector并预分配容量
 * @param vec vector指针
 * @param element_size 元素大小
 * @param capacity 初始容量
 * @return 成功返回0，失败返回-1
 */
int vector_init_with_capacity(vector_t* vec, size_t element_size, size_t capacity);

/**
 * @brief 销毁vector并释放内存
 * @param vec vector指针
 */
void vector_destroy(vector_t* vec);

/**
 * @brief 获取vector大小
 * @param vec vector指针
 * @return 当前元素数量
 */
size_t vector_size(const vector_t* vec);

/**
 * @brief 获取vector容量
 * @param vec vector指针
 * @return 当前容量
 */
size_t vector_capacity(const vector_t* vec);

/**
 * @brief 检查vector是否为空
 * @param vec vector指针
 * @return 空返回true，否则返回false
 */
bool vector_empty(const vector_t* vec);

/**
 * @brief 预留容量
 * @param vec vector指针
 * @param capacity 新容量
 * @return 成功返回0，失败返回-1
 */
int vector_reserve(vector_t* vec, size_t capacity);

/**
 * @brief 调整vector大小
 * @param vec vector指针
 * @param size 新大小
 * @return 成功返回0，失败返回-1
 */
int vector_resize(vector_t* vec, size_t size);

/**
 * @brief 清空vector
 * @param vec vector指针
 */
void vector_clear(vector_t* vec);

/**
 * @brief 获取指定位置元素的指针
 * @param vec vector指针
 * @param index 索引
 * @return 元素指针，越界时返回NULL
 */
void* vector_at(vector_t* vec, size_t index);

/**
 * @brief 获取常量指定位置元素的指针
 * @param vec vector指针
 * @param index 索引
 * @return 元素指针，越界时返回NULL
 */
const void* vector_at_const(const vector_t* vec, size_t index);

/**
 * @brief 获取数据指针
 * @param vec vector指针
 * @return 数据指针
 */
void* vector_data(vector_t* vec);

/**
 * @brief 获取常量数据指针
 * @param vec vector指针
 * @return 常量数据指针
 */
const void* vector_data_const(const vector_t* vec);

/**
 * @brief 在末尾添加元素
 * @param vec vector指针
 * @param element 要添加的元素指针
 * @return 成功返回0，失败返回-1
 */
int vector_push_back(vector_t* vec, const void* element);

/**
 * @brief 删除末尾元素
 * @param vec vector指针
 */
void vector_pop_back(vector_t* vec);

/**
 * @brief 在指定位置插入元素
 * @param vec vector指针
 * @param index 插入位置
 * @param element 要插入的元素指针
 * @return 成功返回0，失败返回-1
 */
int vector_insert(vector_t* vec, size_t index, const void* element);

/**
 * @brief 删除指定位置的元素
 * @param vec vector指针
 * @param index 要删除的位置
 * @return 成功返回0，失败返回-1
 */
int vector_erase(vector_t* vec, size_t index);

/**
 * @brief 批量添加元素
 * @param vec vector指针
 * @param elements 源数据数组
 * @param count 元素数量
 * @return 成功返回0，失败返回-1
 */
int vector_append(vector_t* vec, const void* elements, size_t count);

/**
 * @brief 复制另一个vector
 * @param dest 目标vector
 * @param src 源vector
 * @return 成功返回0，失败返回-1
 */
int vector_copy(vector_t* dest, const vector_t* src);

// ============================================================================
// 类型安全的宏定义
// ============================================================================

/**
 * @brief 声明特定类型的vector
 * @param type 数据类型
 */
#define VECTOR_DECLARE(type) \
    typedef struct { \
        type* data; \
        size_t size; \
        size_t capacity; \
        size_t element_size; \
    } vector_##type##_t; \
    \
    static inline int vector_##type##_init(vector_##type##_t* vec) { \
        vector_t* v = (vector_t*)vec; \
        return vector_init(v, sizeof(type)); \
    } \
    \
    static inline int vector_##type##_init_with_capacity(vector_##type##_t* vec, size_t capacity) { \
        vector_t* v = (vector_t*)vec; \
        return vector_init_with_capacity(v, sizeof(type), capacity); \
    } \
    \
    static inline void vector_##type##_destroy(vector_##type##_t* vec) { \
        vector_t* v = (vector_t*)vec; \
        vector_destroy(v); \
    } \
    \
    static inline size_t vector_##type##_size(const vector_##type##_t* vec) { \
        const vector_t* v = (const vector_t*)vec; \
        return vector_size(v); \
    } \
    \
    static inline size_t vector_##type##_capacity(const vector_##type##_t* vec) { \
        const vector_t* v = (const vector_t*)vec; \
        return vector_capacity(v); \
    } \
    \
    static inline bool vector_##type##_empty(const vector_##type##_t* vec) { \
        const vector_t* v = (const vector_t*)vec; \
        return vector_empty(v); \
    } \
    \
    static inline int vector_##type##_reserve(vector_##type##_t* vec, size_t capacity) { \
        vector_t* v = (vector_t*)vec; \
        return vector_reserve(v, capacity); \
    } \
    \
    static inline int vector_##type##_resize(vector_##type##_t* vec, size_t size) { \
        vector_t* v = (vector_t*)vec; \
        return vector_resize(v, size); \
    } \
    \
    static inline void vector_##type##_clear(vector_##type##_t* vec) { \
        vector_t* v = (vector_t*)vec; \
        vector_clear(v); \
    } \
    \
    static inline type* vector_##type##_at(vector_##type##_t* vec, size_t index) { \
        vector_t* v = (vector_t*)vec; \
        return (type*)vector_at(v, index); \
    } \
    \
    static inline const type* vector_##type##_at_const(const vector_##type##_t* vec, size_t index) { \
        const vector_t* v = (const vector_t*)vec; \
        return (const type*)vector_at_const(v, index); \
    } \
    \
    static inline type* vector_##type##_data(vector_##type##_t* vec) { \
        vector_t* v = (vector_t*)vec; \
        return (type*)vector_data(v); \
    } \
    \
    static inline const type* vector_##type##_data_const(const vector_##type##_t* vec) { \
        const vector_t* v = (const vector_t*)vec; \
        return (const type*)vector_data_const(v); \
    } \
    \
    static inline type vector_##type##_get(const vector_##type##_t* vec, size_t index) { \
        const type* ptr = vector_##type##_at_const(vec, index); \
        if (ptr) { \
            return *ptr; \
        } else { \
            type zero_value; \
            memset(&zero_value, 0, sizeof(type)); \
            return zero_value; \
        } \
    } \
    \
    static inline type vector_##type##_front(const vector_##type##_t* vec) { \
        return vector_##type##_get(vec, 0); \
    } \
    \
    static inline type vector_##type##_back(const vector_##type##_t* vec) { \
        return vector_##type##_get(vec, vector_##type##_size(vec) - 1); \
    } \
    \
    static inline int vector_##type##_push_back(vector_##type##_t* vec, type value) { \
        vector_t* v = (vector_t*)vec; \
        return vector_push_back(v, &value); \
    } \
    \
    static inline void vector_##type##_pop_back(vector_##type##_t* vec) { \
        vector_t* v = (vector_t*)vec; \
        vector_pop_back(v); \
    } \
    \
    static inline int vector_##type##_insert(vector_##type##_t* vec, size_t index, type value) { \
        vector_t* v = (vector_t*)vec; \
        return vector_insert(v, index, &value); \
    } \
    \
    static inline int vector_##type##_erase(vector_##type##_t* vec, size_t index) { \
        vector_t* v = (vector_t*)vec; \
        return vector_erase(v, index); \
    } \
    \
    static inline int vector_##type##_set(vector_##type##_t* vec, size_t index, type value) { \
        type* ptr = vector_##type##_at(vec, index); \
        if (ptr) { \
            *ptr = value; \
            return 0; \
        } \
        return -1; \
    } \
    \
    static inline int vector_##type##_append(vector_##type##_t* vec, const type* data, size_t count) { \
        vector_t* v = (vector_t*)vec; \
        return vector_append(v, data, count); \
    } \
    \
    static inline int vector_##type##_copy(vector_##type##_t* dest, const vector_##type##_t* src) { \
        vector_t* d = (vector_t*)dest; \
        const vector_t* s = (const vector_t*)src; \
        return vector_copy(d, s); \
    }

/**
 * @brief 遍历特定类型vector的宏
 * @param type 数据类型
 * @param vec vector指针
 * @param index 索引变量名
 */
#define VECTOR_FOR_EACH(type, vec, index) \
    for (size_t index = 0; index < vector_##type##_size(vec); ++index)

/**
 * @brief 安全访问特定类型vector元素的宏
 * @param type 数据类型
 * @param vec vector指针
 * @param index 索引
 */
#define VECTOR_GET(type, vec, index) \
    vector_##type##_get(vec, index)

/**
 * @brief 安全设置特定类型vector元素的宏
 * @param type 数据类型
 * @param vec vector指针
 * @param index 索引
 * @param value 值
 */
#define VECTOR_SET(type, vec, index, value) \
    vector_##type##_set(vec, index, value)

// ============================================================================
// 预定义常用类型
// ============================================================================

// 声明常用的基本类型vector
VECTOR_DECLARE(int)
VECTOR_DECLARE(int8_t)
VECTOR_DECLARE(int16_t)
VECTOR_DECLARE(int32_t)
VECTOR_DECLARE(int64_t)
VECTOR_DECLARE(uint8_t)
VECTOR_DECLARE(uint16_t)
VECTOR_DECLARE(uint32_t)
VECTOR_DECLARE(uint64_t)
VECTOR_DECLARE(float)
VECTOR_DECLARE(double)
VECTOR_DECLARE(char)

// 指针类型
typedef void* voidptr_t;
VECTOR_DECLARE(voidptr_t)

// 字符串类型（char*）
typedef char* string_t;
VECTOR_DECLARE(string_t)

#ifdef __cplusplus
}
#endif

#endif /* VECTOR_H */