#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"

/**
 * @file vector_example.c
 * @brief 通用vector使用示例
 */

// 自定义结构体示例
typedef struct {
    int id;
    char name[32];
    float score;
} Student;

// 声明Student类型的vector
VECTOR_DECLARE(Student)

void example_basic_types() {
    printf("=== 基本类型示例 ===\n");
    
    // int16_t vector示例
    vector_int16_t_t int_vec;
    vector_int16_t_init(&int_vec);
    
    // 添加一些数据
    for (int i = 0; i < 10; i++) {
        vector_int16_t_push_back(&int_vec, i * 10);
    }
    
    // 遍历并打印
    printf("int16_t vector: ");
    VECTOR_FOR_EACH(int16_t, &int_vec, i) {
        printf("%d ", VECTOR_GET(int16_t, &int_vec, i));
    }
    printf("\n");
    
    // 获取统计信息
    printf("Size: %zu, Capacity: %zu\n", 
           vector_int16_t_size(&int_vec), 
           vector_int16_t_capacity(&int_vec));
    
    vector_int16_t_destroy(&int_vec);
    
    // float vector示例
    vector_float_t float_vec;
    vector_float_init(&float_vec);
    
    float values[] = {1.1f, 2.2f, 3.3f, 4.4f, 5.5f};
    vector_float_append(&float_vec, values, 5);
    
    printf("float vector: ");
    for (size_t i = 0; i < vector_float_size(&float_vec); i++) {
        printf("%.1f ", vector_float_get(&float_vec, i));
    }
    printf("\n");
    
    vector_float_destroy(&float_vec);
}

void example_custom_struct() {
    printf("\n=== 自定义结构体示例 ===\n");
    
    vector_Student_t students;
    vector_Student_init(&students);
    
    // 添加学生数据
    Student s1 = {1, "Alice", 95.5f};
    Student s2 = {2, "Bob", 87.0f};
    Student s3 = {3, "Charlie", 92.3f};
    
    vector_Student_push_back(&students, s1);
    vector_Student_push_back(&students, s2);
    vector_Student_push_back(&students, s3);
    
    // 遍历并打印学生信息
    printf("Students:\n");
    for (size_t i = 0; i < vector_Student_size(&students); i++) {
        Student* student_ptr = vector_Student_at(&students, i);
        if (student_ptr) {
            printf("  ID: %d, Name: %s, Score: %.1f\n", 
                   student_ptr->id, student_ptr->name, student_ptr->score);
        } else {
            printf("  Error: Could not get student at index %zu\n", i);
        }
    }
    
    // 修改第二个学生的分数
    Student* bob = vector_Student_at(&students, 1);
    if (bob) {
        bob->score = 90.0f;
        printf("Updated Bob's score to %.1f\n", bob->score);
    }
    
    vector_Student_destroy(&students);
}

void example_string_vector() {
    printf("\n=== 字符串vector示例 ===\n");
    
    vector_string_t_t strings;
    vector_string_t_init(&strings);
    
    // 添加字符串（注意：这里存储的是指针，需要确保字符串生命周期）
    char* words[] = {"Hello", "World", "Vector", "Example"};
    
    for (int i = 0; i < 4; i++) {
        // 为每个字符串分配内存并复制
        char* str = malloc(strlen(words[i]) + 1);
        strcpy(str, words[i]);
        vector_string_t_push_back(&strings, str);
    }
    
    printf("Strings: ");
    VECTOR_FOR_EACH(string_t, &strings, i) {
        char* str = VECTOR_GET(string_t, &strings, i);
        printf("%s ", str);
    }
    printf("\n");
    
    // 释放字符串内存
    VECTOR_FOR_EACH(string_t, &strings, i) {
        char* str = VECTOR_GET(string_t, &strings, i);
        free(str);
    }
    
    vector_string_t_destroy(&strings);
}

void example_generic_vector() {
    printf("\n=== 通用vector示例 ===\n");
    
    // 使用通用vector存储int类型
    vector_t generic_vec;
    vector_init(&generic_vec, sizeof(int));
    
    // 添加数据
    for (int i = 1; i <= 5; i++) {
        int value = i * i;
        vector_push_back(&generic_vec, &value);
    }
    
    // 访问数据
    printf("Generic vector (squares): ");
    for (size_t i = 0; i < vector_size(&generic_vec); i++) {
        int* ptr = (int*)vector_at(&generic_vec, i);
        if (ptr) {
            printf("%d ", *ptr);
        }
    }
    printf("\n");
    
    vector_destroy(&generic_vec);
}

void example_vector_operations() {
    printf("\n=== vector操作示例 ===\n");
    
    vector_int_t vec1, vec2;
    vector_int_init(&vec1);
    vector_int_init(&vec2);
    
    // 向vec1添加数据
    for (int i = 0; i < 5; i++) {
        vector_int_push_back(&vec1, i);
    }
    
    // 复制vec1到vec2
    vector_int_copy(&vec2, &vec1);
    
    printf("Original vector: ");
    VECTOR_FOR_EACH(int, &vec1, i) {
        printf("%d ", VECTOR_GET(int, &vec1, i));
    }
    printf("\n");
    
    printf("Copied vector: ");
    VECTOR_FOR_EACH(int, &vec2, i) {
        printf("%d ", VECTOR_GET(int, &vec2, i));
    }
    printf("\n");
    
    // 在中间插入元素
    vector_int_insert(&vec1, 2, 99);
    printf("After inserting 99 at index 2: ");
    VECTOR_FOR_EACH(int, &vec1, i) {
        printf("%d ", VECTOR_GET(int, &vec1, i));
    }
    printf("\n");
    
    // 删除元素
    vector_int_erase(&vec1, 1);
    printf("After erasing index 1: ");
    VECTOR_FOR_EACH(int, &vec1, i) {
        printf("%d ", VECTOR_GET(int, &vec1, i));
    }
    printf("\n");
    
    vector_int_destroy(&vec1);
    vector_int_destroy(&vec2);
}

int main() {
    printf("通用Vector使用示例\n");
    printf("==================\n");
    
    example_basic_types();
    example_custom_struct();
    example_string_vector();
    example_generic_vector();
    example_vector_operations();
    
    printf("\n示例完成！\n");
    return 0;
}