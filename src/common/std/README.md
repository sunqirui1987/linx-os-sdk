# LinX STD Library - C语言标准容器库

这是一个C语言标准容器库，提供类似C++ STL的数据结构实现，包括动态数组(vector)等容器。

## 特性

- **类型安全**：使用宏定义实现类似C++模板的功能
- **高性能**：优化的内存分配策略和算法
- **易用性**：简洁的API设计，类似STL接口
- **C99兼容**：严格遵循C99标准
- **零依赖**：只依赖标准C库

## 包含的容器

### 1. Vector (动态数组)

支持任意类型的动态数组，提供两种使用方式：

#### 方式一：类型安全的宏接口（推荐）

```c
#include "vector.h"

// 使用预定义的基本类型
vector_int16_t int_vec;
vector_int16_init(&int_vec);
vector_int16_push_back(&int_vec, 42);

// 自定义结构体
typedef struct {
    int id;
    char name[32];
} Person;

// 声明Person类型的vector
VECTOR_DECLARE(Person)

vector_Person_t people;
vector_Person_init(&people);

Person p = {1, "Alice"};
vector_Person_push_back(&people, p);
```

#### 方式二：通用接口

```c
#include "vector.h"

// 通用vector，可存储任意类型
vector_t generic_vec;
vector_init(&generic_vec, sizeof(int));

int value = 42;
vector_push_back(&generic_vec, &value);

int* ptr = (int*)vector_at(&generic_vec, 0);
```

## 详细使用指南

### 基本类型Vector

库预定义了常用基本类型的vector：

```c
// 整数类型
vector_int_t, vector_int8_t, vector_int16_t, vector_int32_t, vector_int64_t
vector_uint8_t, vector_uint16_t, vector_uint32_t, vector_uint64_t

// 浮点类型
vector_float_t, vector_double_t

// 字符类型
vector_char_t

// 指针类型
vector_voidptr_t, vector_string_t
```

### 自定义类型Vector

对于自定义结构体，使用`VECTOR_DECLARE`宏：

```c
typedef struct {
    float x, y, z;
} Point3D;

// 声明Point3D类型的vector
VECTOR_DECLARE(Point3D)

// 使用
vector_Point3D_t points;
vector_Point3D_init(&points);

Point3D p1 = {1.0f, 2.0f, 3.0f};
vector_Point3D_push_back(&points, p1);

// 访问元素
Point3D first = vector_Point3D_get(&points, 0);
Point3D* ptr = vector_Point3D_at(&points, 0);
```

### 常用操作

#### 创建和销毁

```c
vector_int_t vec;

// 初始化
vector_int_init(&vec);

// 或者预分配容量
vector_int_init_with_capacity(&vec, 100);

// 销毁（必须调用）
vector_int_destroy(&vec);
```

#### 添加和删除元素

```c
// 在末尾添加
vector_int_push_back(&vec, 42);

// 在指定位置插入
vector_int_insert(&vec, 0, 10); // 在开头插入10

// 删除末尾元素
vector_int_pop_back(&vec);

// 删除指定位置元素
vector_int_erase(&vec, 1); // 删除索引1的元素
```

#### 访问元素

```c
// 获取元素值
int value = vector_int_get(&vec, 0);

// 获取元素指针（可修改）
int* ptr = vector_int_at(&vec, 0);
if (ptr) {
    *ptr = 999;
}

// 获取第一个和最后一个元素
int first = vector_int_front(&vec);
int last = vector_int_back(&vec);

// 直接访问数据数组
int* data = vector_int_data(&vec);
```

#### 容量管理

```c
// 获取大小和容量
size_t size = vector_int_size(&vec);
size_t capacity = vector_int_capacity(&vec);

// 检查是否为空
bool empty = vector_int_empty(&vec);

// 预留容量
vector_int_reserve(&vec, 1000);

// 调整大小
vector_int_resize(&vec, 50);

// 清空
vector_int_clear(&vec);
```

#### 批量操作

```c
// 批量添加
int data[] = {1, 2, 3, 4, 5};
vector_int_append(&vec, data, 5);

// 复制vector
vector_int_t vec2;
vector_int_init(&vec2);
vector_int_copy(&vec2, &vec);
```

#### 遍历

```c
// 使用宏遍历（推荐）
VECTOR_FOR_EACH(int, &vec, i) {
    int value = VECTOR_GET(int, &vec, i);
    printf("vec[%zu] = %d\n", i, value);
}

// 手动遍历
for (size_t i = 0; i < vector_int_size(&vec); i++) {
    int value = vector_int_get(&vec, i);
    printf("vec[%zu] = %d\n", i, value);
}

// 使用数据指针遍历（最高效）
int* data = vector_int_data(&vec);
size_t size = vector_int_size(&vec);
for (size_t i = 0; i < size; i++) {
    printf("vec[%zu] = %d\n", i, data[i]);
}
```

### 完整示例

```c
#include <stdio.h>
#include "vector.h"

// 自定义结构体
typedef struct {
    int id;
    char name[32];
    float score;
} Student;

// 声明Student类型的vector
VECTOR_DECLARE(Student)

int main() {
    // 创建学生vector
    vector_Student_t students;
    vector_Student_init(&students);
    
    // 添加学生
    Student s1 = {1, "Alice", 95.5f};
    Student s2 = {2, "Bob", 87.0f};
    
    vector_Student_push_back(&students, s1);
    vector_Student_push_back(&students, s2);
    
    // 遍历并打印
    printf("Students:\n");
    VECTOR_FOR_EACH(Student, &students, i) {
        Student s = VECTOR_GET(Student, &students, i);
        printf("ID: %d, Name: %s, Score: %.1f\n", s.id, s.name, s.score);
    }
    
    // 修改学生信息
    Student* bob = vector_Student_at(&students, 1);
    if (bob) {
        bob->score = 90.0f;
    }
    
    // 清理
    vector_Student_destroy(&students);
    
    return 0;
}
```

## 性能特点

- **内存增长策略**：容量不足时按2倍增长，减少重新分配次数
- **连续内存**：所有元素存储在连续内存中，缓存友好
- **最小开销**：结构体只包含必要的字段
- **批量操作优化**：使用memcpy/memmove进行批量数据操作

## 注意事项

1. **内存管理**：使用完毕后必须调用`destroy`函数释放内存
2. **越界检查**：访问函数会进行越界检查，越界时返回0或NULL
3. **线程安全**：此实现不是线程安全的，多线程使用需要外部同步
4. **指针有效性**：vector重新分配内存时，之前获取的指针可能失效
5. **字符串存储**：存储字符串时注意内存管理，建议复制字符串内容

## 编译

### 使用CMake

```bash
mkdir build
cd build
cmake ..
make

# 编译示例程序
cmake -DBUILD_VECTOR_EXAMPLES=ON ..
make
./vector_example
```

### 直接编译

```bash
gcc -std=c99 -Wall -Wextra -c vector.c vector_int16.c
gcc -std=c99 -Wall -Wextra vector_example.c vector.o vector_int16.o -o example
```

## API参考

### 类型安全接口

对于类型`T`，提供以下函数：

- `vector_T_init(vec)` - 初始化
- `vector_T_destroy(vec)` - 销毁
- `vector_T_size(vec)` - 获取大小
- `vector_T_push_back(vec, value)` - 添加元素
- `vector_T_get(vec, index)` - 获取元素值
- `vector_T_at(vec, index)` - 获取元素指针
- `vector_T_set(vec, index, value)` - 设置元素值
- 更多函数请参考头文件

### 通用接口

- `vector_init(vec, element_size)` - 初始化
- `vector_destroy(vec)` - 销毁
- `vector_push_back(vec, element_ptr)` - 添加元素
- `vector_at(vec, index)` - 获取元素指针
- 更多函数请参考头文件

## 扩展

要添加新的容器类型，可以参考vector的实现模式：

1. 定义基础结构体和函数
2. 使用宏定义提供类型安全的接口
3. 预定义常用类型
4. 提供使用示例和文档