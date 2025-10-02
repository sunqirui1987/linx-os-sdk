#include "test_framework.h"
#include "../mcp_server.h"
#include "../mcp_types.h"
#include "../mcp_tool.h"
#include "../mcp_property.h"
#include <string.h>
#include <stdlib.h>

// 测试消息发送回调函数
static char* last_sent_message = NULL;

void test_send_callback(const char* message) {
    if (last_sent_message) {
        free(last_sent_message);
    }
    last_sent_message = mcp_strdup(message);
}

// 测试工具回调函数
mcp_return_value_t test_server_tool_callback(const mcp_property_list_t* properties) {
    (void)properties; // 避免未使用参数警告
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    result.value.string_val = mcp_strdup("server test result");
    return result;
}

mcp_return_value_t echo_tool_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (properties && properties->count > 0) {
        const mcp_property_t* prop = mcp_property_list_find(properties, "message");
        if (prop && prop->type == MCP_PROPERTY_TYPE_STRING) {
            char* output = malloc(256);
            snprintf(output, 256, "Echo: %s", mcp_property_get_string_value(prop));
            result.value.string_val = output;
        } else {
            result.value.string_val = mcp_strdup("No message parameter");
        }
    } else {
        result.value.string_val = mcp_strdup("No parameters");
    }
    
    return result;
}

// 测试能力回调函数
void test_camera_set_explain_url(const char* url, const char* token) {
    // 这里可以添加测试逻辑
    printf("Camera explain URL set: %s with token: %s\n", url, token);
}

// 测试服务器创建和销毁
void test_server_create_destroy() {
    printf("Testing server create and destroy...\n");
    
    // 测试正常创建
    mcp_server_t* server = mcp_server_create("test_server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    TEST_ASSERT(strcmp(server->server_name, "test_server") == 0, "Server name mismatch");
    TEST_ASSERT(strcmp(server->server_version, "1.0.0") == 0, "Server version mismatch");
    TEST_ASSERT(server->tool_count == 0, "Server should start with no tools");
    
    mcp_server_destroy(server);
    
    // 测试NULL参数
    server = mcp_server_create(NULL, "1.0.0");
    TEST_ASSERT(server == NULL, "Server creation with NULL name should fail");
    
    server = mcp_server_create("test", NULL);
    TEST_ASSERT(server == NULL, "Server creation with NULL version should fail");
    
    // 测试空字符串参数
    server = mcp_server_create("", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation with empty name should succeed");
    mcp_server_destroy(server);
    
    server = mcp_server_create("test", "");
    TEST_ASSERT(server != NULL, "Server creation with empty version should succeed");
    mcp_server_destroy(server);
}

// 测试工具管理
void test_server_tool_management() {
    printf("Testing server tool management...\n");
    
    mcp_server_t* server = mcp_server_create("test_server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 创建测试工具
    mcp_tool_t* tool1 = mcp_tool_create("tool1", "First test tool", NULL, test_server_tool_callback);
    TEST_ASSERT(tool1 != NULL, "Tool1 creation failed");
    
    // 添加工具到服务器
    bool result = mcp_server_add_tool(server, tool1);
    TEST_ASSERT(result == true, "Adding tool1 to server failed");
    TEST_ASSERT(server->tool_count == 1, "Server should have 1 tool");
    
    // 创建第二个工具
    mcp_tool_t* tool2 = mcp_tool_create("tool2", "Second test tool", NULL, echo_tool_callback);
    TEST_ASSERT(tool2 != NULL, "Tool2 creation failed");
    
    result = mcp_server_add_tool(server, tool2);
    TEST_ASSERT(result == true, "Adding tool2 to server failed");
    TEST_ASSERT(server->tool_count == 2, "Server should have 2 tools");
    
    // 查找工具
    const mcp_tool_t* found = mcp_server_find_tool(server, "tool1");
    TEST_ASSERT(found != NULL, "Tool1 not found");
    TEST_ASSERT(found == tool1, "Found tool1 pointer mismatch");
    
    found = mcp_server_find_tool(server, "tool2");
    TEST_ASSERT(found != NULL, "Tool2 not found");
    TEST_ASSERT(found == tool2, "Found tool2 pointer mismatch");
    
    found = mcp_server_find_tool(server, "nonexistent");
    TEST_ASSERT(found == NULL, "Nonexistent tool should not be found");
    
    // 测试重复工具名
    mcp_tool_t* duplicate_tool = mcp_tool_create("tool1", "Duplicate tool", NULL, test_server_tool_callback);
    result = mcp_server_add_tool(server, duplicate_tool);
    TEST_ASSERT(result == false, "Adding duplicate tool should fail");
    TEST_ASSERT(server->tool_count == 2, "Server should still have 2 tools");
    
    // 清理
    mcp_tool_destroy(duplicate_tool);
    mcp_server_destroy(server);
}

// 测试简单工具添加
void test_server_simple_tool_add() {
    printf("Testing server simple tool add...\n");
    
    mcp_server_t* server = mcp_server_create("test_server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 创建属性列表
    mcp_property_list_t* properties = mcp_property_list_create();
    mcp_property_t* prop = mcp_property_create_string("message", NULL, false);
    mcp_property_list_add(properties, prop);
    
    // 添加简单工具
    bool result = mcp_server_add_simple_tool(server, "echo", "Echo tool", properties, echo_tool_callback);
    TEST_ASSERT(result == true, "Adding simple tool failed");
    TEST_ASSERT(server->tool_count == 1, "Server should have 1 tool");
    
    // 查找工具
    const mcp_tool_t* found = mcp_server_find_tool(server, "echo");
    TEST_ASSERT(found != NULL, "Echo tool not found");
    TEST_ASSERT(strcmp(found->name, "echo") == 0, "Tool name mismatch");
    TEST_ASSERT(strcmp(found->description, "Echo tool") == 0, "Tool description mismatch");
    TEST_ASSERT(found->user_only == false, "Tool should not be user-only by default");
    
    // 添加用户专用工具
    result = mcp_server_add_user_only_tool(server, "user_tool", "User only tool", NULL, test_server_tool_callback);
    TEST_ASSERT(result == true, "Adding user-only tool failed");
    TEST_ASSERT(server->tool_count == 2, "Server should have 2 tools");
    
    found = mcp_server_find_tool(server, "user_tool");
    TEST_ASSERT(found != NULL, "User tool not found");
    TEST_ASSERT(found->user_only == true, "Tool should be user-only");
    
    // 清理
    // 注意：属性和属性列表的所有权已转移给工具，不需要手动释放
    // 服务器销毁时会自动清理所有工具及其属性
    mcp_server_destroy(server);
}

// 测试消息处理
void test_server_message_handling() {
    printf("Testing server message handling...\n");
    
    mcp_server_t* server = mcp_server_create("test_server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 设置消息发送回调
    mcp_server_set_send_callback(test_send_callback);
    
    // 测试初始化消息
    const char* init_message = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{}}}";
    mcp_server_parse_message(server, init_message);
    
    TEST_ASSERT(last_sent_message != NULL, "No response message sent");
    TEST_ASSERT(strstr(last_sent_message, "test_server") != NULL, "Server name not in response");
    TEST_ASSERT(strstr(last_sent_message, "1.0.0") != NULL, "Server version not in response");
    
    // 清理上一条消息
    if (last_sent_message) {
        free(last_sent_message);
        last_sent_message = NULL;
    }
    
    // 添加工具
    mcp_server_add_simple_tool(server, "test_tool", "Test tool", NULL, test_server_tool_callback);
    
    // 测试工具列表消息
    const char* tools_list_message = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\",\"params\":{}}";
    mcp_server_parse_message(server, tools_list_message);
    
    TEST_ASSERT(last_sent_message != NULL, "No tools list response sent");
    TEST_ASSERT(strstr(last_sent_message, "test_tool") != NULL, "Tool name not in tools list");
    
    // 清理
    if (last_sent_message) {
        free(last_sent_message);
        last_sent_message = NULL;
    }
    
    mcp_server_destroy(server);
}

// 测试工具调用
void test_server_tool_call() {
    printf("Testing server tool call...\n");
    
    mcp_server_t* server = mcp_server_create("test_server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 设置消息发送回调
    mcp_server_set_send_callback(test_send_callback);
    
    // 添加echo工具
    mcp_property_list_t* properties = mcp_property_list_create();
    mcp_property_t* prop = mcp_property_create_string("message", NULL, false);
    mcp_property_list_add(properties, prop);
    mcp_server_add_simple_tool(server, "echo", "Echo tool", properties, echo_tool_callback);
    
    // 测试工具调用消息
    const char* tool_call_message = "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"message\":\"Hello World\"}}}";
    mcp_server_parse_message(server, tool_call_message);
    
    TEST_ASSERT(last_sent_message != NULL, "No tool call response sent");
    TEST_ASSERT(strstr(last_sent_message, "Echo: Hello World") != NULL, "Tool response not correct");
    
    // 清理
    if (last_sent_message) {
        free(last_sent_message);
        last_sent_message = NULL;
    }
    
    // 注意：属性和属性列表的所有权已转移给工具，不需要手动释放
    // 服务器销毁时会自动清理所有工具及其属性
    mcp_server_destroy(server);
}

// 测试能力配置
void test_server_capabilities() {
    printf("Testing server capabilities...\n");
    
    mcp_server_t* server = mcp_server_create("test_server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 设置能力回调
    mcp_capability_callbacks_t callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.camera_set_explain_url = test_camera_set_explain_url;
    mcp_server_set_capability_callbacks(server, &callbacks);
    
    // 创建能力配置JSON
    cJSON* capabilities = cJSON_CreateObject();
    cJSON* camera = cJSON_CreateObject();
    cJSON_AddStringToObject(camera, "explain_url", "http://example.com/explain");
    cJSON_AddStringToObject(camera, "token", "test_token_123");
    cJSON_AddItemToObject(capabilities, "camera", camera);
    
    // 解析能力配置
    mcp_server_parse_capabilities(server, capabilities);
    
    // 验证回调函数被调用（这里只是简单测试，实际应用中可能需要更复杂的验证）
    TEST_ASSERT(server->capability_callbacks.camera_set_explain_url != NULL, "Camera callback not set");
    
    // 清理
    cJSON_Delete(capabilities);
    mcp_server_destroy(server);
}

// 测试工具列表JSON生成
void test_server_tools_list_json() {
    printf("Testing server tools list JSON generation...\n");
    
    mcp_server_t* server = mcp_server_create("test_server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 添加普通工具
    mcp_server_add_simple_tool(server, "normal_tool", "Normal tool", NULL, test_server_tool_callback);
    
    // 添加用户专用工具
    mcp_server_add_user_only_tool(server, "user_tool", "User only tool", NULL, test_server_tool_callback);
    
    // 获取所有工具列表
    char* json = mcp_server_get_tools_list_json(server, NULL, false);
    TEST_ASSERT(json != NULL, "Tools list JSON generation failed");
    TEST_ASSERT(strstr(json, "normal_tool") != NULL, "Normal tool not in JSON");
    TEST_ASSERT(strstr(json, "user_tool") != NULL, "User tool not in JSON");
    free(json);
    
    // 获取仅用户工具列表
    json = mcp_server_get_tools_list_json(server, NULL, true);
    TEST_ASSERT(json != NULL, "User tools list JSON generation failed");
    TEST_ASSERT(strstr(json, "normal_tool") == NULL, "Normal tool should not be in user-only JSON");
    TEST_ASSERT(strstr(json, "user_tool") != NULL, "User tool not in user-only JSON");
    free(json);
    
    mcp_server_destroy(server);
}

// 测试边界条件和错误处理
void test_server_edge_cases() {
    printf("Testing server edge cases...\n");
    
    mcp_server_t* server = mcp_server_create("test_server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 测试NULL参数处理
    mcp_server_parse_message(NULL, "test");
    mcp_server_parse_message(server, NULL);
    mcp_server_parse_json_message(NULL, NULL);
    mcp_server_parse_json_message(server, NULL);
    
    // 测试无效JSON消息
    mcp_server_parse_message(server, "invalid json");
    mcp_server_parse_message(server, "{incomplete json");
    
    // 测试工具数量限制
    for (int i = 0; i < MCP_MAX_TOOLS + 5; i++) {
        char tool_name[32];
        snprintf(tool_name, sizeof(tool_name), "tool_%d", i);
        
        mcp_tool_t* tool = mcp_tool_create(tool_name, "Test tool", NULL, test_server_tool_callback);
        bool added = mcp_server_add_tool(server, tool);
        
        if (i < MCP_MAX_TOOLS) {
            TEST_ASSERT(added == true, "Tool addition should succeed within limit");
        } else {
            TEST_ASSERT(added == false, "Tool addition should fail beyond limit");
            mcp_tool_destroy(tool);  // 清理未添加的工具
        }
    }
    
    TEST_ASSERT(server->tool_count == MCP_MAX_TOOLS, "Server should have maximum tools");
    
    // 测试服务器名称长度限制
    char long_name[MCP_MAX_NAME_LENGTH + 10];
    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    
    mcp_server_t* long_name_server = mcp_server_create(long_name, "1.0.0");
    TEST_ASSERT(long_name_server != NULL, "Server creation with long name should succeed");
    // 验证名称被截断
    TEST_ASSERT(strlen(long_name_server->server_name) < strlen(long_name), "Server name should be truncated");
    mcp_server_destroy(long_name_server);
    
    mcp_server_destroy(server);
}

// 运行所有服务器测试
void run_server_tests() {
    printf("\n=== Running Server Tests ===\n");
    
    test_server_create_destroy();
    test_server_tool_management();
    test_server_simple_tool_add();
    test_server_message_handling();
    test_server_tool_call();
    test_server_capabilities();
    test_server_tools_list_json();
    test_server_edge_cases();
    
    printf("=== Server Tests Complete ===\n\n");
    
    // 清理全局变量
    if (last_sent_message) {
        free(last_sent_message);
        last_sent_message = NULL;
    }
}

int main() {
    test_init();
    run_server_tests();
    test_summary();
    // 检查内存泄漏
    test_check_memory_leaks();
    return g_test_stats.failed_tests > 0 ? 1 : 0;
}