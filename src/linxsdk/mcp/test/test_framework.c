/**
 * @file test_framework.c
 * @brief 测试框架实现
 */

#include "test_framework.h"

/* 全局测试统计 */
test_stats_t g_test_stats = {0, 0, 0};

/* 内存跟踪 */
static int allocated_blocks = 0;
static size_t total_allocated = 0;

/**
 * 初始化测试环境
 */
void test_init(void) {
    g_test_stats.total_tests = 0;
    g_test_stats.passed_tests = 0;
    g_test_stats.failed_tests = 0;
    allocated_blocks = 0;
    total_allocated = 0;
    
    printf(COLOR_BLUE "=== MCP SDK Unit Tests ===" COLOR_RESET "\n");
    printf("Initializing test environment...\n");
}

/**
 * 打印测试总结
 */
void test_summary(void) {
    printf(COLOR_BLUE "\n=== Test Summary ===" COLOR_RESET "\n");
    printf("Total Tests: %d\n", g_test_stats.total_tests);
    
    if (g_test_stats.passed_tests > 0) {
        printf(COLOR_GREEN "Passed: %d" COLOR_RESET "\n", g_test_stats.passed_tests);
    }
    
    if (g_test_stats.failed_tests > 0) {
        printf(COLOR_RED "Failed: %d" COLOR_RESET "\n", g_test_stats.failed_tests);
    }
    
    double pass_rate = g_test_stats.total_tests > 0 ? 
        (double)g_test_stats.passed_tests / g_test_stats.total_tests * 100.0 : 0.0;
    
    printf("Pass Rate: %.1f%%\n", pass_rate);
    
    if (g_test_stats.failed_tests == 0) {
        printf(COLOR_GREEN "All tests passed!" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "Some tests failed!" COLOR_RESET "\n");
    }
    
    test_check_memory_leaks();
}

/**
 * 运行所有测试
 */
int test_run_all(void) {
    test_init();
    
    // 这里会调用各个测试模块的函数
    // 在各个测试文件中实现
    
    test_summary();
    
    return g_test_stats.failed_tests == 0 ? 0 : 1;
}

/**
 * 测试用内存分配
 */
void* test_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        allocated_blocks++;
        total_allocated += size;
    }
    return ptr;
}

/**
 * 测试用内存释放
 */
void test_free(void* ptr) {
    if (ptr) {
        allocated_blocks--;
        free(ptr);
    }
}

/**
 * 检查内存泄漏
 */
void test_check_memory_leaks(void) {
    printf("\n--- Memory Leak Check ---\n");
    if (allocated_blocks == 0) {
        printf(COLOR_GREEN "No memory leaks detected" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "Memory leak detected: %d blocks still allocated" COLOR_RESET "\n", allocated_blocks);
    }
    printf("Total allocated during tests: %zu bytes\n", total_allocated);
}