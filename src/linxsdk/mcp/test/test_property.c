#include "test_framework.h"
#include "../mcp_property.h"
#include "../mcp_types.h"
#include <string.h>
#include <stdlib.h>

// 测试属性创建和销毁
void test_property_create_destroy() {
    printf("Testing property create and destroy...\n");
    
    // 测试字符串属性
    mcp_property_t* str_prop = mcp_property_create_string("test_string", "default", false);
    TEST_ASSERT(str_prop != NULL, "String property creation failed");
    TEST_ASSERT(strcmp(str_prop->name, "test_string") == 0, "Property name mismatch");
    TEST_ASSERT(str_prop->type == MCP_PROPERTY_TYPE_STRING, "Property type mismatch");
    mcp_property_destroy(str_prop);
    
    // 测试整数属性
    mcp_property_t* int_prop = mcp_property_create_integer("test_int", 0, false, false, 0, 0);
    TEST_ASSERT(int_prop != NULL, "Integer property creation failed");
    TEST_ASSERT(int_prop->type == MCP_PROPERTY_TYPE_INTEGER, "Integer property type mismatch");
    mcp_property_destroy(int_prop);
    
    // 测试布尔属性
    mcp_property_t* bool_prop = mcp_property_create_boolean("test_bool", false, false);
    TEST_ASSERT(bool_prop != NULL, "Boolean property creation failed");
    TEST_ASSERT(bool_prop->type == MCP_PROPERTY_TYPE_BOOLEAN, "Boolean property type mismatch");
    mcp_property_destroy(bool_prop);
    
    // 注意：当前MCP类型定义只支持BOOLEAN、INTEGER、STRING三种类型
}

// 测试属性值设置和获取
void test_property_value_operations() {
    printf("Testing property value operations...\n");
    
    // 测试字符串值
    mcp_property_t* str_prop = mcp_property_create_string("test_string", NULL, false);
    TEST_ASSERT(str_prop != NULL, "String property creation failed");
    
    int result = mcp_property_set_string_value(str_prop, "Hello World");
    TEST_ASSERT(result == 0, "String value setting failed");
    
    const char* str_value = mcp_property_get_string_value(str_prop);
    TEST_ASSERT(str_value != NULL, "String value retrieval failed");
    TEST_ASSERT(strcmp(str_value, "Hello World") == 0, "String value mismatch");
    
    mcp_property_destroy(str_prop);
    
    // 测试整数值
    mcp_property_t* int_prop = mcp_property_create_integer("test_int", 0, false, false, 0, 0);
    TEST_ASSERT(int_prop != NULL, "Integer property creation failed");
    
    result = mcp_property_set_int_value(int_prop, 42);
    TEST_ASSERT(result == 0, "Integer value setting failed");
    
    int int_value = mcp_property_get_int_value(int_prop);
    TEST_ASSERT(int_value == 42, "Integer value mismatch");
    
    mcp_property_destroy(int_prop);
    
    // 测试布尔值
    mcp_property_t* bool_prop = mcp_property_create_boolean("test_bool", false, false);
    TEST_ASSERT(bool_prop != NULL, "Boolean property creation failed");
    
    result = mcp_property_set_bool_value(bool_prop, true);
    TEST_ASSERT(result == 0, "Boolean value setting failed");
    
    bool bool_value = mcp_property_get_bool_value(bool_prop);
    TEST_ASSERT(bool_value == true, "Boolean value mismatch");
    
    mcp_property_destroy(bool_prop);
}

// 测试属性列表操作
void test_property_list_operations() {
    printf("Testing property list operations...\n");
    
    mcp_property_list_t* list = mcp_property_list_create();
    TEST_ASSERT(list != NULL, "Property list creation failed");
    TEST_ASSERT(list->count == 0, "Initial property list count should be 0");
    TEST_ASSERT(list->properties != NULL, "Initial property list should not be NULL");
    
    // 添加属性
    mcp_property_t* prop1 = mcp_property_create_string("prop1", "value1", true);
    
    bool result = mcp_property_list_add(list, prop1);
    TEST_ASSERT(result == true, "Property list add failed");
    TEST_ASSERT(list->count == 1, "Property list count should be 1");
    
    mcp_property_t* prop2 = mcp_property_create_integer("prop2", 100, true, false, 0, 0);
    
    result = mcp_property_list_add(list, prop2);
    TEST_ASSERT(result == true, "Property list add failed");
    TEST_ASSERT(list->count == 2, "Property list count should be 2");
    
    // 查找属性
    const mcp_property_t* found = mcp_property_list_find(list, "prop1");
    TEST_ASSERT(found != NULL, "Property not found");
    TEST_ASSERT(strcmp(found->name, "prop1") == 0, "Found property name mismatch");
    
    found = mcp_property_list_find(list, "prop2");
    TEST_ASSERT(found != NULL, "Property not found");
    TEST_ASSERT(found->type == MCP_PROPERTY_TYPE_INTEGER, "Found property type mismatch");
    
    found = mcp_property_list_find(list, "nonexistent");
    TEST_ASSERT(found == NULL, "Nonexistent property should not be found");
    
    // 注意：当前API中没有remove函数，跳过移除测试
    
    // 清理属性
    mcp_property_destroy(prop1);
    mcp_property_destroy(prop2);
    
    mcp_property_list_destroy(list);
}

// 测试属性验证
void test_property_validation() {
    printf("Testing property validation...\n");
    
    // 测试必需属性验证（通过has_default_value来模拟）
    mcp_property_t* prop = mcp_property_create_string("required_prop", NULL, false);
    TEST_ASSERT(prop != NULL, "Property creation failed");
    
    // 设置值
    int result = mcp_property_set_string_value(prop, "some_value");
    TEST_ASSERT(result == 0, "String value setting failed");
    
    const char* value = mcp_property_get_string_value(prop);
    TEST_ASSERT(value != NULL && strcmp(value, "some_value") == 0, "Property value should match");
    
    mcp_property_destroy(prop);
    
    // 测试可选属性（有默认值）
    prop = mcp_property_create_string("optional_prop", "default_value", true);
    TEST_ASSERT(prop != NULL, "Optional property creation failed");
    TEST_ASSERT(prop->has_default_value == true, "Optional property should have default value");
    
    mcp_property_destroy(prop);
}

// 测试属性序列化
void test_property_serialization() {
    printf("Testing property serialization...\n");
    
    // 测试字符串属性序列化
    mcp_property_t* str_prop = mcp_property_create_string("test_string", "Hello World", true);
    
    char* json = mcp_property_to_json(str_prop);
    TEST_ASSERT(json != NULL, "Property serialization failed");
    TEST_ASSERT(strstr(json, "test_string") != NULL, "Property name not in JSON");
    TEST_ASSERT(strstr(json, "Hello World") != NULL, "Property value not in JSON");
    free(json);
    
    mcp_property_destroy(str_prop);
    
    // 测试整数属性序列化
    mcp_property_t* int_prop = mcp_property_create_integer("test_int", 42, true, false, 0, 0);
    
    json = mcp_property_to_json(int_prop);
    TEST_ASSERT(json != NULL, "Integer property serialization failed");
    TEST_ASSERT(strstr(json, "test_int") != NULL, "Integer property name not in JSON");
    TEST_ASSERT(strstr(json, "42") != NULL, "Integer property value not in JSON");
    free(json);
    
    mcp_property_destroy(int_prop);
}

// 测试属性列表序列化
void test_property_list_serialization() {
    printf("Testing property list serialization...\n");
    
    mcp_property_list_t* list = mcp_property_list_create();
    
    mcp_property_t* prop1 = mcp_property_create_string("name", "John Doe", true);
    mcp_property_list_add(list, prop1);
    
    mcp_property_t* prop2 = mcp_property_create_integer("age", 30, true, false, 0, 0);
    mcp_property_list_add(list, prop2);
    
    mcp_property_t* prop3 = mcp_property_create_boolean("active", true, true);
    mcp_property_list_add(list, prop3);
    
    char* json = mcp_property_list_to_json(list);
    TEST_ASSERT(json != NULL, "Property list serialization failed");
    TEST_ASSERT(strstr(json, "name") != NULL, "Property name not in JSON");
    TEST_ASSERT(strstr(json, "John Doe") != NULL, "Property value not in JSON");
    TEST_ASSERT(strstr(json, "age") != NULL, "Age property not in JSON");
    TEST_ASSERT(strstr(json, "30") != NULL, "Age value not in JSON");
    TEST_ASSERT(strstr(json, "active") != NULL, "Active property not in JSON");
    free(json);
    
    // 清理属性
    mcp_property_destroy(prop1);
    mcp_property_destroy(prop2);
    mcp_property_destroy(prop3);
    
    mcp_property_list_destroy(list);
}

// 测试边界条件和错误处理
void test_property_edge_cases() {
    printf("Testing property edge cases...\n");
    
    // 测试NULL参数
    mcp_property_t* prop = mcp_property_create_string(NULL, NULL, false);
    TEST_ASSERT(prop == NULL, "Property creation with NULL name should fail");
    
    prop = mcp_property_create_string("", NULL, false);
    TEST_ASSERT(prop == NULL, "Property creation with empty name should fail");
    
    // 测试有效属性创建
    prop = mcp_property_create_string("test", NULL, false);
    TEST_ASSERT(prop != NULL, "Valid property creation failed");
    
    // 测试类型不匹配的值设置
    int result = mcp_property_set_int_value(prop, 42);
    TEST_ASSERT(result != 0, "Setting integer value on string property should fail");
    
    result = mcp_property_set_bool_value(prop, true);
    TEST_ASSERT(result != 0, "Setting boolean value on string property should fail");
    
    mcp_property_destroy(prop);
    
    // 测试空字符串值
    prop = mcp_property_create_string("test", "", true);
    const char* value = mcp_property_get_string_value(prop);
    TEST_ASSERT(value != NULL && strlen(value) == 0, "Empty string value should be retrievable");
    
    mcp_property_destroy(prop);
    
    // 测试长字符串
    char long_string[1024];
    memset(long_string, 'A', sizeof(long_string) - 1);
    long_string[sizeof(long_string) - 1] = '\0';
    
    prop = mcp_property_create_string("test", long_string, true);
    TEST_ASSERT(prop != NULL, "Long string property creation should succeed");
    
    value = mcp_property_get_string_value(prop);
    TEST_ASSERT(value != NULL && strcmp(value, long_string) == 0, "Long string value should match");
    
    mcp_property_destroy(prop);
}

// 运行所有属性测试
void run_property_tests() {
    printf("\n=== Running Property Tests ===\n");
    
    test_property_create_destroy();
    test_property_value_operations();
    test_property_list_operations();
    test_property_validation();
    test_property_serialization();
    test_property_list_serialization();
    test_property_edge_cases();
    
    printf("=== Property Tests Complete ===\n\n");
}

/**
 * 主函数 - 运行所有属性管理测试
 */
int main(void) {
    test_init();
    run_property_tests();
    test_summary();
    
    // 检查内存泄漏
    test_check_memory_leaks();
    
    return (g_test_stats.failed_tests > 0) ? 1 : 0;
}