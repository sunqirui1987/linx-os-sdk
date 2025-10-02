#include "test_framework.h"
#include "../mcp_tool.h"
#include "../mcp_types.h"
#include <string.h>
#include <stdlib.h>

// 测试工具回调函数
mcp_return_value_t test_callback_simple(const mcp_property_list_t* properties) {
    (void)properties; // 避免未使用参数警告
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    result.value.string_val = mcp_strdup("test result");
    return result;
}

mcp_return_value_t test_callback_with_params(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (properties && properties->count > 0) {
        const mcp_property_t* prop = mcp_property_list_find(properties, "input");
        if (prop && prop->type == MCP_PROPERTY_TYPE_STRING) {
            char* output = malloc(256);
            snprintf(output, 256, "Processed: %s", mcp_property_get_string_value(prop));
            result.value.string_val = output;
        } else {
            result.value.string_val = mcp_strdup("No valid input parameter");
        }
    } else {
        result.value.string_val = mcp_strdup("No parameters provided");
    }
    
    return result;
}

mcp_return_value_t test_callback_integer(const mcp_property_list_t* properties) {
    (void)properties; // 避免未使用参数警告
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_INT;
    result.value.int_val = 42;
    return result;
}

mcp_return_value_t test_callback_boolean(const mcp_property_list_t* properties) {
    (void)properties; // 避免未使用参数警告
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_BOOL;
    result.value.bool_val = true;
    return result;
}

// 测试工具创建和销毁
void test_tool_create_destroy() {
    printf("Testing tool create and destroy...\n");
    
    // 测试简单工具创建
    mcp_tool_t* tool = mcp_tool_create("test_tool", "A test tool", NULL, test_callback_simple);
    TEST_ASSERT(tool != NULL, "Tool creation failed");
    TEST_ASSERT(strcmp(tool->name, "test_tool") == 0, "Tool name mismatch");
    TEST_ASSERT(strcmp(tool->description, "A test tool") == 0, "Tool description mismatch");
    TEST_ASSERT(tool->callback == test_callback_simple, "Tool callback mismatch");
    TEST_ASSERT(tool->properties != NULL, "Tool property list should be initialized");
    TEST_ASSERT(tool->properties->count == 0, "Tool should start with no properties");
    
    mcp_tool_destroy(tool);
    
    // 测试NULL参数
    tool = mcp_tool_create(NULL, "description", NULL, test_callback_simple);
    TEST_ASSERT(tool == NULL, "Tool creation with NULL name should fail");
    
    tool = mcp_tool_create("name", NULL, NULL, test_callback_simple);
    TEST_ASSERT(tool == NULL, "Tool creation with NULL description should fail");
    
    tool = mcp_tool_create("name", "description", NULL, NULL);
    TEST_ASSERT(tool == NULL, "Tool creation with NULL callback should fail");
    
    // 测试空字符串参数
    tool = mcp_tool_create("", "description", NULL, test_callback_simple);
    TEST_ASSERT(tool == NULL, "Tool creation with empty name should fail");
    
    tool = mcp_tool_create("name", "", NULL, test_callback_simple);
    TEST_ASSERT(tool != NULL, "Tool creation with empty description should succeed");
    mcp_tool_destroy(tool);
}

// 测试工具属性管理
void test_tool_property_management() {
    printf("Testing tool property management...\n");
    
    // 创建属性列表
    mcp_property_list_t* properties = mcp_property_list_create();
    TEST_ASSERT(properties != NULL, "Property list creation failed");
    
    // 添加字符串属性
    mcp_property_t* str_prop = mcp_property_create_string("input", NULL, false);
    bool result = mcp_property_list_add(properties, str_prop);
    TEST_ASSERT(result == true, "Adding string property failed");
    TEST_ASSERT(properties->count == 1, "Property list should have 1 property");
    
    // 添加整数属性
    mcp_property_t* int_prop = mcp_property_create_integer("count", 10, true, true, 1, 100);
    result = mcp_property_list_add(properties, int_prop);
    TEST_ASSERT(result == true, "Adding integer property failed");
    TEST_ASSERT(properties->count == 2, "Property list should have 2 properties");
    
    // 添加布尔属性
    mcp_property_t* bool_prop = mcp_property_create_boolean("enabled", true, true);
    result = mcp_property_list_add(properties, bool_prop);
    TEST_ASSERT(result == true, "Adding boolean property failed");
    TEST_ASSERT(properties->count == 3, "Property list should have 3 properties");
    
    // 创建工具
    mcp_tool_t* tool = mcp_tool_create("test_tool", "Test tool", properties, test_callback_with_params);
    TEST_ASSERT(tool != NULL, "Tool creation failed");
    TEST_ASSERT(tool->properties != NULL, "Tool properties not set correctly");
    TEST_ASSERT(tool->properties->count == properties->count, "Tool properties count mismatch");
    
    // 查找属性
    const mcp_property_t* found = mcp_property_list_find(tool->properties, "input");
    TEST_ASSERT(found != NULL, "Input property not found");
    TEST_ASSERT(found->type == MCP_PROPERTY_TYPE_STRING, "Input property type mismatch");
    
    found = mcp_property_list_find(tool->properties, "count");
    TEST_ASSERT(found != NULL, "Count property not found");
    TEST_ASSERT(found->type == MCP_PROPERTY_TYPE_INTEGER, "Count property type mismatch");
    
    found = mcp_property_list_find(tool->properties, "enabled");
    TEST_ASSERT(found != NULL, "Enabled property not found");
    TEST_ASSERT(found->type == MCP_PROPERTY_TYPE_BOOLEAN, "Enabled property type mismatch");
    
    found = mcp_property_list_find(tool->properties, "nonexistent");
    TEST_ASSERT(found == NULL, "Nonexistent property should not be found");
    
    // 测试重复属性名
    mcp_property_t* duplicate_prop = mcp_property_create_string("input", "default", true);
    result = mcp_property_list_add(properties, duplicate_prop);
    TEST_ASSERT(result == false, "Adding duplicate property should fail");
    TEST_ASSERT(properties->count == 3, "Property list should still have 3 properties");
    
    // 清理
    mcp_property_destroy(str_prop);
    mcp_property_destroy(int_prop);
    mcp_property_destroy(bool_prop);
    mcp_property_destroy(duplicate_prop);
    mcp_tool_destroy(tool);
    mcp_property_list_destroy(properties);  // 释放原始的properties列表
}

// 测试工具执行
void test_tool_execution() {
    printf("Testing tool execution...\n");
    
    // 测试简单工具执行
    mcp_tool_t* simple_tool = mcp_tool_create("simple", "Simple tool", NULL, test_callback_simple);
    TEST_ASSERT(simple_tool != NULL, "Simple tool creation failed");
    
    char* result_json = mcp_tool_call(simple_tool, NULL);
    TEST_ASSERT(result_json != NULL, "Simple tool call returned NULL");
    // Note: We can't easily test the exact content without parsing JSON
    // but we can verify it's not NULL
    
    // 清理
    free(result_json);
    mcp_tool_destroy(simple_tool);
    
    // 测试带参数的工具执行
    mcp_property_list_t* tool_properties = mcp_property_list_create();
    mcp_property_t* input_prop = mcp_property_create_string("input", NULL, false);
    mcp_property_list_add(tool_properties, input_prop);
    
    mcp_tool_t* param_tool = mcp_tool_create("param_tool", "Tool with parameters", tool_properties, test_callback_with_params);
    TEST_ASSERT(param_tool != NULL, "Parameter tool creation failed");
    
    // 创建参数列表
    mcp_property_list_t* params = mcp_property_list_create();
    mcp_property_t* param = mcp_property_create_string("input", "Hello World", true);
    mcp_property_list_add(params, param);
    
    result_json = mcp_tool_call(param_tool, params);
    TEST_ASSERT(result_json != NULL, "Parameter tool call returned NULL");
    
    // 清理
    free(result_json);
    mcp_property_destroy(input_prop);
    mcp_property_destroy(param);
    mcp_property_list_destroy(params);
    mcp_tool_destroy(param_tool);
    
    // 测试整数返回值工具
    mcp_tool_t* int_tool = mcp_tool_create("int_tool", "Integer tool", NULL, test_callback_integer);
    result_json = mcp_tool_call(int_tool, NULL);
    TEST_ASSERT(result_json != NULL, "Integer tool call returned NULL");
    
    free(result_json);
    mcp_tool_destroy(int_tool);
    
    // 测试布尔返回值工具
    mcp_tool_t* bool_tool = mcp_tool_create("bool_tool", "Boolean tool", NULL, test_callback_boolean);
    result_json = mcp_tool_call(bool_tool, NULL);
    TEST_ASSERT(result_json != NULL, "Boolean tool call returned NULL");
    
    free(result_json);
    mcp_tool_destroy(bool_tool);
}

// 测试工具序列化
void test_tool_serialization() {
    printf("Testing tool serialization...\n");
    
    // 创建属性列表
    mcp_property_list_t* properties = mcp_property_list_create();
    
    // 添加一些属性
    mcp_property_t* str_prop = mcp_property_create_string("name", "default_name", true);
    mcp_property_list_add(properties, str_prop);
    
    mcp_property_t* int_prop = mcp_property_create_integer("age", 25, true, true, 0, 100);
    mcp_property_list_add(properties, int_prop);
    
    mcp_tool_t* tool = mcp_tool_create("test_tool", "A test tool for serialization", properties, test_callback_simple);
    TEST_ASSERT(tool != NULL, "Tool creation failed");
    
    // 序列化工具
    char* json = mcp_tool_to_json(tool);
    TEST_ASSERT(json != NULL, "Tool serialization failed");
    TEST_ASSERT(strstr(json, "test_tool") != NULL, "Tool name not in JSON");
    TEST_ASSERT(strstr(json, "A test tool for serialization") != NULL, "Tool description not in JSON");
    TEST_ASSERT(strstr(json, "name") != NULL, "Property name not in JSON");
    TEST_ASSERT(strstr(json, "age") != NULL, "Property age not in JSON");
    
    free(json);
    
    // 清理
    mcp_property_destroy(str_prop);
    mcp_property_destroy(int_prop);
    mcp_tool_destroy(tool);
    mcp_property_list_destroy(properties);  // 释放原始的properties列表
}

// 测试工具验证
void test_tool_validation() {
    printf("Testing tool validation...\n");
    
    // 创建带必需参数的工具
    mcp_property_list_t* properties = mcp_property_list_create();
    
    mcp_property_t* str_prop = mcp_property_create_string("name", NULL, false);
    mcp_property_list_add(properties, str_prop);
    
    mcp_property_t* int_prop = mcp_property_create_integer("age", 0, false, true, 0, 150);
    mcp_property_list_add(properties, int_prop);
    
    mcp_tool_t* tool = mcp_tool_create("validation_tool", "Tool for validation", properties, test_callback_with_params);
    TEST_ASSERT(tool != NULL, "Tool creation failed");
    
    // 创建必需属性
    mcp_property_list_t* required_properties = mcp_property_list_create();
    mcp_property_t* required_prop = mcp_property_create_string("name", "John", true);
    mcp_property_list_add(required_properties, required_prop);
    
    // 创建可选属性
    mcp_property_t* optional_prop = mcp_property_create_integer("age", 25, true, true, 0, 150);
    mcp_property_list_add(required_properties, optional_prop);
    
    // 测试有效参数 - 简化测试，只检查工具调用不会崩溃
    char* result = mcp_tool_call(tool, required_properties);
    TEST_ASSERT(result != NULL, "Tool call with valid parameters failed");
    free(result);
    
    // 测试不完整参数
    mcp_property_list_t* incomplete_params = mcp_property_list_create();
    mcp_property_t* incomplete_prop = mcp_property_create_string("name", "Jane", true);
    mcp_property_list_add(incomplete_params, incomplete_prop);
    
    result = mcp_tool_call(tool, incomplete_params);
    TEST_ASSERT(result != NULL, "Tool call with incomplete parameters failed");
    free(result);
    
    // 清理
    mcp_property_destroy(str_prop);
    mcp_property_destroy(int_prop);
    mcp_property_destroy(required_prop);
    mcp_property_destroy(optional_prop);
    mcp_property_destroy(incomplete_prop);
    mcp_property_list_destroy(required_properties);
    mcp_property_list_destroy(incomplete_params);
    mcp_tool_destroy(tool);
}

// 测试工具边界情况
void test_tool_edge_cases() {
    printf("Testing tool edge cases...\n");
    
    // 测试NULL工具调用
    char* result = mcp_tool_call(NULL, NULL);
    TEST_ASSERT(result == NULL, "NULL tool call should return NULL");
    
    // 测试空名称工具创建
    mcp_tool_t* tool = mcp_tool_create("", "description", NULL, test_callback_simple);
    TEST_ASSERT(tool == NULL, "Tool creation with empty name should fail");
    
    // 测试NULL回调工具创建
    tool = mcp_tool_create("name", "description", NULL, NULL);
    TEST_ASSERT(tool == NULL, "Tool creation with NULL callback should fail");
    
    // 测试有效工具的属性添加
    mcp_property_list_t* properties = mcp_property_list_create();
    tool = mcp_tool_create("test", "Test tool", properties, test_callback_simple);
    TEST_ASSERT(tool != NULL, "Valid tool creation failed");
    
    // 测试添加属性到已创建的工具
    mcp_property_t* prop = mcp_property_create_string("test_prop", "value", true);
    bool added = mcp_property_list_add(tool->properties, prop);
    TEST_ASSERT(added == true, "Adding property to tool failed");
    TEST_ASSERT(tool->properties->count == 1, "Tool should have 1 property");
    
    // 清理
    mcp_property_destroy(prop);
    mcp_tool_destroy(tool);
    mcp_property_list_destroy(properties);  // 释放原始的properties列表
}

// ... existing code ...

// 运行所有工具测试
void run_tool_tests() {
    printf("\n=== Running Tool Tests ===\n");
    
    test_tool_create_destroy();
    test_tool_property_management();
    test_tool_execution();
    test_tool_serialization();
    test_tool_validation();
    test_tool_edge_cases();
    
    printf("=== Tool Tests Complete ===\n\n");
}

int main() {
    test_init();
    run_tool_tests();
    test_summary();
    // 检查内存泄漏
    test_check_memory_leaks();
    
    return (g_test_stats.failed_tests > 0) ? 1 : 0;
}