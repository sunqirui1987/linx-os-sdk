#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"

// 自定义结构体示例
typedef struct {
    int id;
    char name[32];
    float score;
} Student;

// 声明Student类型的vector
VECTOR_DECLARE(Student)

int main() {
    printf("简单Vector测试\n");
    printf("==============\n");
    
    // 测试基本类型
    printf("测试int16_t vector...\n");
    vector_int16_t_t int_vec;
    vector_int16_t_init(&int_vec);
    
    vector_int16_t_push_back(&int_vec, 42);
    vector_int16_t_push_back(&int_vec, 100);
    
    printf("int16_t vector size: %zu\n", vector_int16_t_size(&int_vec));
    printf("int16_t vector[0]: %d\n", vector_int16_t_get(&int_vec, 0));
    printf("int16_t vector[1]: %d\n", vector_int16_t_get(&int_vec, 1));
    
    vector_int16_t_destroy(&int_vec);
    printf("int16_t vector测试完成\n");
    
    // 测试自定义结构体
    printf("\n测试Student vector...\n");
    vector_Student_t students;
    vector_Student_init(&students);
    
    Student s1 = {1, "Alice", 95.5f};
    printf("准备添加学生: ID=%d, Name=%s, Score=%.1f\n", s1.id, s1.name, s1.score);
    
    vector_Student_push_back(&students, s1);
    printf("学生添加成功\n");
    
    printf("vector size: %zu\n", vector_Student_size(&students));
    
    // 获取学生信息
    Student* student_ptr = vector_Student_at(&students, 0);
    if (student_ptr) {
        printf("获取到学生: ID=%d, Name=%s, Score=%.1f\n", 
               student_ptr->id, student_ptr->name, student_ptr->score);
    } else {
        printf("无法获取学生信息\n");
    }
    
    vector_Student_destroy(&students);
    printf("Student vector测试完成\n");
    
    printf("\n所有测试完成！\n");
    return 0;
}