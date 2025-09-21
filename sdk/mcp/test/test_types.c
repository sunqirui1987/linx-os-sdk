/**
 * @file test_types.c
 * @brief MCP类型定义单元测试
 * 
 * 测试mcp_types.h中定义的所有枚举、结构体和常量
 */

#include "test_framework.h"
#include "../mcp_types.h"
#include <string.h>
#include <limits.h>

/**
 * 测试属性类型枚举
 */
void test_property_types(void) {
    TEST_CASE_START("Property Types");
    
    // 测试枚举值
    TEST_ASSERT_EQUAL_INT(0, MCP_PROPERTY_TYPE_BOOLEAN);
    TEST_ASSERT_EQUAL_INT(1, MCP_PROPERTY_TYPE_INTEGER);
    TEST_ASSERT_EQUAL_INT(2, MCP_PROPERTY_TYPE_STRING);
    
    // 测试枚举大小
    TEST_ASSERT(sizeof(mcp_property_type_t) >= sizeof(int), "Property type enum should be at least int size");
}

/**
 * 测试返回值类型枚举
 */
void test_return_types(void) {
    TEST_CASE_START("Return Types");
    
    // 测试枚举值
    TEST_ASSERT_EQUAL_INT(0, MCP_RETURN_TYPE_BOOL);
    TEST_ASSERT_EQUAL_INT(1, MCP_RETURN_TYPE_INT);
    TEST_ASSERT_EQUAL_INT(2, MCP_RETURN_TYPE_STRING);
    TEST_ASSERT_EQUAL_INT(3, MCP_RETURN_TYPE_JSON);
    TEST_ASSERT_EQUAL_INT(4, MCP_RETURN_TYPE_IMAGE);
    
    // 测试枚举大小
    TEST_ASSERT(sizeof(mcp_return_type_t) >= sizeof(int), "Return type enum should be at least int size");
}

/**
 * 测试属性值联合体
 */
void test_property_value_union(void) {
    TEST_CASE_START("Property Value Union");
    
    mcp_property_value_t value;
    
    // 测试布尔值
    value.bool_val = true;
    TEST_ASSERT_TRUE(value.bool_val);
    
    value.bool_val = false;
    TEST_ASSERT_FALSE(value.bool_val);
    
    // 测试整数值
    value.int_val = 42;
    TEST_ASSERT_EQUAL_INT(42, value.int_val);
    
    value.int_val = -100;
    TEST_ASSERT_EQUAL_INT(-100, value.int_val);
    
    // 测试字符串值
    char test_str[] = "test string";
    value.string_val = test_str;
    TEST_ASSERT_EQUAL_STR("test string", value.string_val);
    
    // 测试联合体大小
    TEST_ASSERT(sizeof(mcp_property_value_t) >= sizeof(void*), "Property value union should be at least pointer size");
}

/**
 * 测试返回值结构体
 */
void test_return_value_struct(void) {
    TEST_CASE_START("Return Value Struct");
    
    mcp_return_value_t ret_val;
    
    // 测试布尔返回值
    ret_val.type = MCP_RETURN_TYPE_BOOL;
    ret_val.value.bool_val = true;
    TEST_ASSERT_EQUAL_INT(MCP_RETURN_TYPE_BOOL, ret_val.type);
    TEST_ASSERT_TRUE(ret_val.value.bool_val);
    
    // 测试整数返回值
    ret_val.type = MCP_RETURN_TYPE_INT;
    ret_val.value.int_val = 123;
    TEST_ASSERT_EQUAL_INT(MCP_RETURN_TYPE_INT, ret_val.type);
    TEST_ASSERT_EQUAL_INT(123, ret_val.value.int_val);
    
    // 测试字符串返回值
    ret_val.type = MCP_RETURN_TYPE_STRING;
    ret_val.value.string_val = "hello world";
    TEST_ASSERT_EQUAL_INT(MCP_RETURN_TYPE_STRING, ret_val.type);
    TEST_ASSERT_EQUAL_STR("hello world", ret_val.value.string_val);
    
    // 测试JSON返回值
    ret_val.type = MCP_RETURN_TYPE_JSON;
    ret_val.value.json_val = NULL;  // 暂时设为NULL
    TEST_ASSERT_EQUAL_INT(MCP_RETURN_TYPE_JSON, ret_val.type);
    TEST_ASSERT_NULL(ret_val.value.json_val);
    
    // 测试图像返回值
    ret_val.type = MCP_RETURN_TYPE_IMAGE;
    ret_val.value.image_val = NULL;  // 暂时设为NULL
    TEST_ASSERT_EQUAL_INT(MCP_RETURN_TYPE_IMAGE, ret_val.type);
    TEST_ASSERT_NULL(ret_val.value.image_val);
}

/**
 * 测试常量定义
 */
void test_constants(void) {
    TEST_CASE_START("Constants");
    
    // 测试最大长度常量
    TEST_ASSERT_EQUAL_INT(256, MCP_MAX_NAME_LENGTH);
    TEST_ASSERT_EQUAL_INT(1024, MCP_MAX_DESCRIPTION_LENGTH);
    TEST_ASSERT_EQUAL_INT(64, MCP_MAX_TOOLS);
    TEST_ASSERT_EQUAL_INT(32, MCP_MAX_PROPERTIES);
    TEST_ASSERT_EQUAL_INT(512, MCP_MAX_URL_LENGTH);
    
    // 测试常量的合理性
    TEST_ASSERT(MCP_MAX_NAME_LENGTH > 0, "Name length should be positive");
    TEST_ASSERT(MCP_MAX_DESCRIPTION_LENGTH > MCP_MAX_NAME_LENGTH, "Description should be longer than name");
    TEST_ASSERT(MCP_MAX_TOOLS > 0, "Max tools should be positive");
    TEST_ASSERT(MCP_MAX_PROPERTIES > 0, "Max properties should be positive");
    TEST_ASSERT(MCP_MAX_URL_LENGTH > 0, "Max URL length should be positive");
}

/**
 * 测试结构体大小和对齐
 */
void test_struct_sizes(void) {
    TEST_CASE_START("Struct Sizes and Alignment");
    
    // 测试基本类型大小
    TEST_ASSERT(sizeof(mcp_property_type_t) > 0, "Property type should have non-zero size");
    TEST_ASSERT(sizeof(mcp_return_type_t) > 0, "Return type should have non-zero size");
    TEST_ASSERT(sizeof(mcp_property_value_t) > 0, "Property value should have non-zero size");
    TEST_ASSERT(sizeof(mcp_return_value_t) > 0, "Return value should have non-zero size");
    
    // 测试结构体对齐
    TEST_ASSERT(sizeof(mcp_return_value_t) >= sizeof(mcp_return_type_t) + sizeof(mcp_property_value_t), 
                "Return value struct should contain both type and value");
    
    printf("Struct sizes:\n");
    printf("  mcp_property_type_t: %zu bytes\n", sizeof(mcp_property_type_t));
    printf("  mcp_return_type_t: %zu bytes\n", sizeof(mcp_return_type_t));
    printf("  mcp_property_value_t: %zu bytes\n", sizeof(mcp_property_value_t));
    printf("  mcp_return_value_t: %zu bytes\n", sizeof(mcp_return_value_t));
}

/**
 * 测试边界条件
 */
void test_boundary_conditions(void) {
    TEST_CASE_START("Boundary Conditions");
    
    mcp_property_value_t value;
    mcp_return_value_t ret_val;
    
    // 测试极值
    value.int_val = INT_MAX;
    TEST_ASSERT_EQUAL_INT(INT_MAX, value.int_val);
    
    value.int_val = INT_MIN;
    TEST_ASSERT_EQUAL_INT(INT_MIN, value.int_val);
    
    // 测试NULL指针
    value.string_val = NULL;
    TEST_ASSERT_NULL(value.string_val);
    
    ret_val.value.json_val = NULL;
    TEST_ASSERT_NULL(ret_val.value.json_val);
    
    ret_val.value.image_val = NULL;
    TEST_ASSERT_NULL(ret_val.value.image_val);
}

/**
 * 运行所有类型测试
 */
void run_types_tests(void) {
    TEST_SUITE_START("MCP Types Tests");
    
    test_property_types();
    test_return_types();
    test_property_value_union();
    test_return_value_struct();
    test_constants();
    test_struct_sizes();
    test_boundary_conditions();
    
    TEST_SUITE_END("MCP Types Tests");
}

/**
 * 主函数
 */
int main(void) {
    test_init();
    run_types_tests();
    test_summary();
    
    // 检查内存泄漏
    test_check_memory_leaks();
    
    return (g_test_stats.failed_tests > 0) ? 1 : 0;
}