/**
 * @file test_framework.h
 * @brief 简单的C语言单元测试框架
 * 
 * 提供基础的测试断言和测试运行功能
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 测试统计信息 */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
} test_stats_t;

/* 全局测试统计 */
extern test_stats_t g_test_stats;

/* 颜色输出宏 */
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

/* 测试断言宏 */
#define TEST_ASSERT(condition, message) \
    do { \
        g_test_stats.total_tests++; \
        if (condition) { \
            g_test_stats.passed_tests++; \
            printf(COLOR_GREEN "✓ PASS" COLOR_RESET ": %s\n", message); \
        } else { \
            g_test_stats.failed_tests++; \
            printf(COLOR_RED "✗ FAIL" COLOR_RESET ": %s (line %d)\n", message, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) \
    TEST_ASSERT((condition), #condition " should be true")

#define TEST_ASSERT_FALSE(condition) \
    TEST_ASSERT(!(condition), #condition " should be false")

#define TEST_ASSERT_NULL(ptr) \
    TEST_ASSERT((ptr) == NULL, #ptr " should be NULL")

#define TEST_ASSERT_NOT_NULL(ptr) \
    TEST_ASSERT((ptr) != NULL, #ptr " should not be NULL")

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    TEST_ASSERT((expected) == (actual), "Expected " #expected " but got " #actual)

#define TEST_ASSERT_EQUAL_STR(expected, actual) \
    TEST_ASSERT(strcmp((expected), (actual)) == 0, "String mismatch: expected \"" #expected "\" but got \"" #actual "\"")

#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
    TEST_ASSERT((expected) == (actual), "Pointer mismatch")

/* 测试套件宏 */
#define TEST_SUITE_START(name) \
    printf(COLOR_BLUE "\n=== Running Test Suite: %s ===" COLOR_RESET "\n", name)

#define TEST_SUITE_END(name) \
    printf(COLOR_BLUE "=== Test Suite %s Complete ===" COLOR_RESET "\n", name)

#define TEST_CASE_START(name) \
    printf(COLOR_YELLOW "\n--- Test Case: %s ---" COLOR_RESET "\n", name)

/* 测试运行函数 */
void test_init(void);
void test_summary(void);
int test_run_all(void);

/* 内存泄漏检测辅助 */
void* test_malloc(size_t size);
void test_free(void* ptr);
void test_check_memory_leaks(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_FRAMEWORK_H */