#include "test_framework.h"
#include "../mcp.h"
#include <string.h>
#include <stdlib.h>

// 全局变量用于测试
static char* received_messages[10];
static int message_count = 0;

// 测试消息接收回调
void integration_test_send_callback(const char* message) {
    if (message_count < 10) {
        received_messages[message_count] = mcp_strdup(message);
        message_count++;
    }
}

// 清理接收到的消息
void cleanup_received_messages() {
    for (int i = 0; i < message_count; i++) {
        if (received_messages[i]) {
            free(received_messages[i]);
            received_messages[i] = NULL;
        }
    }
    message_count = 0;
}

// 计算器工具回调函数
mcp_return_value_t calculator_add_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_INT;
    result.value.int_val = 0;
    
    if (properties && properties->count >= 2) {
        const mcp_property_t* a_prop = mcp_property_list_find(properties, "a");
        const mcp_property_t* b_prop = mcp_property_list_find(properties, "b");
        
        if (a_prop && b_prop && 
            a_prop->type == MCP_PROPERTY_TYPE_INTEGER && 
            b_prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            result.value.int_val = mcp_property_get_int_value(a_prop) + mcp_property_get_int_value(b_prop);
        }
    }
    
    return result;
}

// 字符串处理工具回调函数
mcp_return_value_t string_upper_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    result.value.string_val = mcp_strdup("Error: No input");
    
    if (properties && properties->count > 0) {
        const mcp_property_t* input_prop = mcp_property_list_find(properties, "input");
        if (input_prop && input_prop->type == MCP_PROPERTY_TYPE_STRING) {
            const char* input = mcp_property_get_string_value(input_prop);
            if (input) {
                size_t len = strlen(input);
                char* upper_str = malloc(len + 1);
                if (upper_str) {
                    for (size_t i = 0; i < len; i++) {
                        upper_str[i] = (input[i] >= 'a' && input[i] <= 'z') ? 
                                      input[i] - 'a' + 'A' : input[i];
                    }
                    upper_str[len] = '\0';
                    free(result.value.string_val);
                    result.value.string_val = upper_str;
                }
            }
        }
    }
    
    return result;
}

// 图像处理工具回调函数
mcp_return_value_t image_info_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (properties && properties->count > 0) {
        const mcp_property_t* image_prop = mcp_property_list_find(properties, "image_data");
        if (image_prop && image_prop->type == MCP_PROPERTY_TYPE_STRING) {
            const char* image_data = mcp_property_get_string_value(image_prop);
            if (image_data) {
                // 模拟图像信息分析
                char* info = malloc(256);
                snprintf(info, 256, "Image analysis: %zu bytes of data received", strlen(image_data));
                result.value.string_val = info;
            } else {
                result.value.string_val = mcp_strdup("Error: Invalid image data");
            }
        } else {
            result.value.string_val = mcp_strdup("Error: No image data provided");
        }
    } else {
        result.value.string_val = mcp_strdup("Error: No parameters provided");
    }
    
    return result;
}

// 测试完整的MCP服务器工作流程
void test_complete_mcp_workflow() {
    printf("Testing complete MCP workflow...\n");
    
    // 1. 创建服务器
    mcp_server_t* server = mcp_server_create("Integration Test Server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 2. 设置消息回调
    mcp_server_set_send_callback(integration_test_send_callback);
    
    // 3. 创建并添加计算器工具
    mcp_property_list_t* calc_props = mcp_property_list_create();
    mcp_property_t* a_prop = mcp_property_create_integer("a", 0, false, false, 0, 0);
    mcp_property_t* b_prop = mcp_property_create_integer("b", 0, false, false, 0, 0);
    mcp_property_list_add(calc_props, a_prop);
    mcp_property_list_add(calc_props, b_prop);
    
    bool result = mcp_server_add_simple_tool(server, "calculator_add", "Add two numbers", calc_props, calculator_add_callback);
    TEST_ASSERT(result == true, "Adding calculator tool failed");
    
    // 4. 创建并添加字符串处理工具
    mcp_property_list_t* str_props = mcp_property_list_create();
    mcp_property_t* input_prop = mcp_property_create_string("input", NULL, false);
    mcp_property_list_add(str_props, input_prop);
    
    result = mcp_server_add_simple_tool(server, "string_upper", "Convert string to uppercase", str_props, string_upper_callback);
    TEST_ASSERT(result == true, "Adding string tool failed");
    
    // 5. 创建并添加图像处理工具
    mcp_property_list_t* img_props = mcp_property_list_create();
    mcp_property_t* img_prop = mcp_property_create_string("image_data", NULL, false);
    mcp_property_list_add(img_props, img_prop);
    
    result = mcp_server_add_simple_tool(server, "image_info", "Analyze image information", img_props, image_info_callback);
    TEST_ASSERT(result == true, "Adding image tool failed");
    
    // 6. 测试初始化流程
    const char* init_msg = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{}}}";
    mcp_server_parse_message(server, init_msg);
    
    TEST_ASSERT(message_count > 0, "No initialization response received");
    TEST_ASSERT(strstr(received_messages[0], "Integration Test Server") != NULL, "Server name not in init response");
    
    // 7. 测试工具列表获取
    const char* tools_msg = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\",\"params\":{}}";
    mcp_server_parse_message(server, tools_msg);
    
    TEST_ASSERT(message_count > 1, "No tools list response received");
    TEST_ASSERT(strstr(received_messages[1], "calculator_add") != NULL, "Calculator tool not in list");
    TEST_ASSERT(strstr(received_messages[1], "string_upper") != NULL, "String tool not in list");
    TEST_ASSERT(strstr(received_messages[1], "image_info") != NULL, "Image tool not in list");
    
    // 8. 测试计算器工具调用
    const char* calc_msg = "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"calculator_add\",\"arguments\":{\"a\":15,\"b\":27}}}";
    mcp_server_parse_message(server, calc_msg);
    
    TEST_ASSERT(message_count > 2, "No calculator response received");
    TEST_ASSERT(strstr(received_messages[2], "42") != NULL, "Calculator result incorrect");
    
    // 9. 测试字符串工具调用
    const char* str_msg = "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"string_upper\",\"arguments\":{\"input\":\"hello world\"}}}";
    mcp_server_parse_message(server, str_msg);
    
    TEST_ASSERT(message_count > 3, "No string response received");
    TEST_ASSERT(strstr(received_messages[3], "HELLO WORLD") != NULL, "String conversion incorrect");
    
    // 10. 测试图像工具调用
    const char* img_msg = "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\",\"params\":{\"name\":\"image_info\",\"arguments\":{\"image_data\":\"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg==\"}}}";
    mcp_server_parse_message(server, img_msg);
    
    TEST_ASSERT(message_count > 4, "No image response received");
    TEST_ASSERT(strstr(received_messages[4], "Image analysis") != NULL, "Image analysis response incorrect");
    
    // 清理
    // 注意：属性列表的所有权已经转移给工具，会在服务器销毁时自动清理
    // 不需要手动销毁属性列表，否则会导致双重释放
    mcp_server_destroy(server);
    cleanup_received_messages();
}

// 测试属性验证集成
void test_property_validation_integration() {
    printf("Testing property validation integration...\n");
    
    mcp_server_t* server = mcp_server_create("Validation Test Server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    mcp_server_set_send_callback(integration_test_send_callback);
    
    // 创建带有复杂验证规则的工具
    mcp_property_list_t* props = mcp_property_list_create();
    
    // 必需的字符串参数
    mcp_property_t* name_prop = mcp_property_create_string("name", NULL, false);
    mcp_property_list_add(props, name_prop);
    
    // 有范围限制的整数参数
    mcp_property_t* age_prop = mcp_property_create_integer("age", 25, true, true, 0, 120);
    mcp_property_list_add(props, age_prop);
    
    // 可选的布尔参数
    mcp_property_t* active_prop = mcp_property_create_boolean("active", true, true);
    mcp_property_list_add(props, active_prop);
    
    bool result = mcp_server_add_simple_tool(server, "user_profile", "Create user profile", props, string_upper_callback);
    TEST_ASSERT(result == true, "Adding validation tool failed");
    
    // 测试有效参数调用
    const char* valid_msg = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"user_profile\",\"arguments\":{\"name\":\"John Doe\",\"age\":30,\"active\":true}}}";
    mcp_server_parse_message(server, valid_msg);
    
    TEST_ASSERT(message_count > 0, "No response for valid parameters");
    
    // 测试缺少必需参数
    const char* missing_msg = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"user_profile\",\"arguments\":{\"age\":30}}}";
    mcp_server_parse_message(server, missing_msg);
    
    TEST_ASSERT(message_count > 1, "No response for missing parameters");
    
    // 测试超出范围的参数
    const char* invalid_msg = "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"user_profile\",\"arguments\":{\"name\":\"John\",\"age\":150}}}";
    mcp_server_parse_message(server, invalid_msg);
    
    TEST_ASSERT(message_count > 2, "No response for invalid parameters");
    
    // 清理
    // 注意：属性列表的所有权已经转移给工具，会在服务器销毁时自动清理
    mcp_server_destroy(server);
    cleanup_received_messages();
}

// 测试内存管理集成
void test_memory_management_integration() {
    printf("Testing memory management integration...\n");
    
    // 创建大量对象并确保正确清理
    for (int i = 0; i < 100; i++) {
        mcp_server_t* server = mcp_server_create("Memory Test Server", "1.0.0");
        TEST_ASSERT(server != NULL, "Server creation failed in memory test");
        
        // 添加多个工具
        for (int j = 0; j < 10; j++) {
            char tool_name[32];
            snprintf(tool_name, sizeof(tool_name), "tool_%d", j);
            
            mcp_property_list_t* props = mcp_property_list_create();
            mcp_property_t* prop = mcp_property_create_string("input", NULL, false);
            mcp_property_list_add(props, prop);
            
            mcp_server_add_simple_tool(server, tool_name, "Test tool", props, string_upper_callback);
            
            // 注意：属性列表的所有权已经转移给工具，会在服务器销毁时自动清理
            // 不需要手动销毁属性列表，否则会导致双重释放
        }
        
        mcp_server_destroy(server);
    }
    
    // 测试图像内容的内存管理
    for (int i = 0; i < 50; i++) {
        const char* test_data = "test image data";
        mcp_image_content_t* image = mcp_image_content_create("image/png", test_data, strlen(test_data));
        TEST_ASSERT(image != NULL, "Image content creation failed");
        
        char* json = mcp_image_content_to_json(image);
        TEST_ASSERT(json != NULL, "Image JSON conversion failed");
        
        free(json);
        mcp_image_content_destroy(image);
    }
    
    // 测试Base64编码的内存管理
    for (int i = 0; i < 100; i++) {
        char test_data[256];
        snprintf(test_data, sizeof(test_data), "test data %d", i);
        
        char* encoded = mcp_base64_encode(test_data, strlen(test_data));
        TEST_ASSERT(encoded != NULL, "Base64 encoding failed");
        
        free(encoded);
    }
}

// 测试错误处理集成
void test_error_handling_integration() {
    printf("Testing error handling integration...\n");
    
    mcp_server_t* server = mcp_server_create("Error Test Server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    mcp_server_set_send_callback(integration_test_send_callback);
    
    // 测试调用不存在的工具
    const char* nonexistent_msg = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"nonexistent_tool\",\"arguments\":{}}}";
    mcp_server_parse_message(server, nonexistent_msg);
    
    TEST_ASSERT(message_count > 0, "No error response for nonexistent tool");
    TEST_ASSERT(strstr(received_messages[0], "error") != NULL || strstr(received_messages[0], "Error") != NULL, "Error not indicated in response");
    
    // 测试无效的JSON-RPC消息
    const char* invalid_jsonrpc = "{\"id\":2,\"method\":\"tools/call\"}";  // 缺少jsonrpc字段
    mcp_server_parse_message(server, invalid_jsonrpc);
    
    // 测试无效的方法名
    const char* invalid_method = "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"invalid/method\",\"params\":{}}";
    mcp_server_parse_message(server, invalid_method);
    
    // 测试格式错误的JSON
    const char* malformed_json = "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/list\",\"params\":{";
    mcp_server_parse_message(server, malformed_json);
    
    mcp_server_destroy(server);
    cleanup_received_messages();
}

// 测试并发安全性（简单测试）
void test_concurrency_safety() {
    printf("Testing basic concurrency safety...\n");
    
    mcp_server_t* server = mcp_server_create("Concurrency Test Server", "1.0.0");
    TEST_ASSERT(server != NULL, "Server creation failed");
    
    // 模拟并发添加工具
    for (int i = 0; i < 20; i++) {
        char tool_name[32];
        snprintf(tool_name, sizeof(tool_name), "concurrent_tool_%d", i);
        
        bool result = mcp_server_add_simple_tool(server, tool_name, "Concurrent tool", NULL, string_upper_callback);
        TEST_ASSERT(result == true, "Concurrent tool addition failed");
    }
    
    TEST_ASSERT(server->tool_count == 20, "Incorrect tool count after concurrent additions");
    
    // 验证所有工具都能找到
    for (int i = 0; i < 20; i++) {
        char tool_name[32];
        snprintf(tool_name, sizeof(tool_name), "concurrent_tool_%d", i);
        
        const mcp_tool_t* found = mcp_server_find_tool(server, tool_name);
        TEST_ASSERT(found != NULL, "Concurrent tool not found");
    }
    
    mcp_server_destroy(server);
}

// 运行所有集成测试
void run_integration_tests() {
    printf("\n=== Running Integration Tests ===\n");
    
    test_complete_mcp_workflow();
    test_property_validation_integration();
    test_memory_management_integration();
    test_error_handling_integration();
    test_concurrency_safety();
    
    printf("=== Integration Tests Complete ===\n\n");
}


int main() {
    test_init();
    run_integration_tests();
    test_summary();
     // 检查内存泄漏
    test_check_memory_leaks();
    
    return (g_test_stats.failed_tests > 0) ? 1 : 0;
}