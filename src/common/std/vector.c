#include "vector.h"
#include <stdlib.h>
#include <string.h>

/**
 * @file vector.c
 * @brief 通用动态数组实现
 */

// 默认初始容量
#define VECTOR_DEFAULT_CAPACITY 8
// 容量增长因子
#define VECTOR_GROWTH_FACTOR 2

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 扩展vector容量
 * @param vec vector指针
 * @param min_capacity 最小需要的容量
 * @return 成功返回0，失败返回-1
 */
static int vector_grow(vector_t* vec, size_t min_capacity) {
    if (!vec) {
        return -1;
    }
    
    if (min_capacity <= vec->capacity) {
        return 0;
    }
    
    size_t new_capacity = vec->capacity;
    if (new_capacity == 0) {
        new_capacity = VECTOR_DEFAULT_CAPACITY;
    }
    
    while (new_capacity < min_capacity) {
        new_capacity *= VECTOR_GROWTH_FACTOR;
    }
    
    void* new_data = realloc(vec->data, new_capacity * vec->element_size);
    if (!new_data) {
        return -1;
    }
    
    vec->data = new_data;
    vec->capacity = new_capacity;
    return 0;
}

// ============================================================================
// 基础函数实现
// ============================================================================

int vector_init(vector_t* vec, size_t element_size) {
    if (!vec || element_size == 0) {
        return -1;
    }
    
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
    vec->element_size = element_size;
    return 0;
}

int vector_init_with_capacity(vector_t* vec, size_t element_size, size_t capacity) {
    if (!vec || element_size == 0) {
        return -1;
    }
    
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
    vec->element_size = element_size;
    
    if (capacity > 0) {
        return vector_reserve(vec, capacity);
    }
    
    return 0;
}

void vector_destroy(vector_t* vec) {
    if (!vec) {
        return;
    }
    
    if (vec->data) {
        free(vec->data);
        vec->data = NULL;
    }
    vec->size = 0;
    vec->capacity = 0;
    vec->element_size = 0;
}

size_t vector_size(const vector_t* vec) {
    return vec ? vec->size : 0;
}

size_t vector_capacity(const vector_t* vec) {
    return vec ? vec->capacity : 0;
}

bool vector_empty(const vector_t* vec) {
    return !vec || vec->size == 0;
}

int vector_reserve(vector_t* vec, size_t capacity) {
    if (!vec) {
        return -1;
    }
    
    return vector_grow(vec, capacity);
}

int vector_resize(vector_t* vec, size_t size) {
    if (!vec) {
        return -1;
    }
    
    if (size > vec->capacity) {
        if (vector_grow(vec, size) != 0) {
            return -1;
        }
    }
    
    // 如果缩小，新增的内存区域会保持未初始化状态
    // 如果需要初始化为0，可以使用memset
    if (size > vec->size) {
        // 可选：将新增的内存区域清零
        char* data_ptr = (char*)vec->data;
        memset(data_ptr + vec->size * vec->element_size, 0, 
               (size - vec->size) * vec->element_size);
    }
    
    vec->size = size;
    return 0;
}

void vector_clear(vector_t* vec) {
    if (vec) {
        vec->size = 0;
    }
}

void* vector_at(vector_t* vec, size_t index) {
    if (!vec || index >= vec->size) {
        return NULL;
    }
    
    char* data_ptr = (char*)vec->data;
    return data_ptr + index * vec->element_size;
}

const void* vector_at_const(const vector_t* vec, size_t index) {
    if (!vec || index >= vec->size) {
        return NULL;
    }
    
    const char* data_ptr = (const char*)vec->data;
    return data_ptr + index * vec->element_size;
}

void* vector_data(vector_t* vec) {
    return vec ? vec->data : NULL;
}

const void* vector_data_const(const vector_t* vec) {
    return vec ? vec->data : NULL;
}

int vector_push_back(vector_t* vec, const void* element) {
    if (!vec || !element) {
        return -1;
    }
    
    if (vec->size >= vec->capacity) {
        if (vector_grow(vec, vec->size + 1) != 0) {
            return -1;
        }
    }
    
    char* data_ptr = (char*)vec->data;
    memcpy(data_ptr + vec->size * vec->element_size, element, vec->element_size);
    vec->size++;
    return 0;
}

void vector_pop_back(vector_t* vec) {
    if (vec && vec->size > 0) {
        vec->size--;
    }
}

int vector_insert(vector_t* vec, size_t index, const void* element) {
    if (!vec || !element || index > vec->size) {
        return -1;
    }
    
    if (vec->size >= vec->capacity) {
        if (vector_grow(vec, vec->size + 1) != 0) {
            return -1;
        }
    }
    
    char* data_ptr = (char*)vec->data;
    
    // 向后移动元素
    if (index < vec->size) {
        memmove(data_ptr + (index + 1) * vec->element_size,
                data_ptr + index * vec->element_size,
                (vec->size - index) * vec->element_size);
    }
    
    // 插入新元素
    memcpy(data_ptr + index * vec->element_size, element, vec->element_size);
    vec->size++;
    return 0;
}

int vector_erase(vector_t* vec, size_t index) {
    if (!vec || index >= vec->size) {
        return -1;
    }
    
    char* data_ptr = (char*)vec->data;
    
    // 向前移动元素
    if (index < vec->size - 1) {
        memmove(data_ptr + index * vec->element_size,
                data_ptr + (index + 1) * vec->element_size,
                (vec->size - index - 1) * vec->element_size);
    }
    
    vec->size--;
    return 0;
}

int vector_append(vector_t* vec, const void* elements, size_t count) {
    if (!vec || !elements || count == 0) {
        return -1;
    }
    
    if (vec->size + count > vec->capacity) {
        if (vector_grow(vec, vec->size + count) != 0) {
            return -1;
        }
    }
    
    char* data_ptr = (char*)vec->data;
    memcpy(data_ptr + vec->size * vec->element_size, elements, count * vec->element_size);
    vec->size += count;
    return 0;
}

int vector_copy(vector_t* dest, const vector_t* src) {
    if (!dest || !src) {
        return -1;
    }
    
    // 检查元素大小是否匹配
    if (dest->element_size != src->element_size) {
        return -1;
    }
    
    vector_clear(dest);
    
    if (src->size == 0) {
        return 0;
    }
    
    return vector_append(dest, src->data, src->size);
}