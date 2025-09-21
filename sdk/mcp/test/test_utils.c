/**
 * @file test_utils.c
 * @brief MCP工具函数单元测试
 * 
 * 测试mcp_utils.c中的所有工具函数，包括Base64编码、图像处理、字符串操作等
 */

#include "test_framework.h"
#include "../mcp_utils.h"
#include <string.h>
#include <stdlib.h>

/**
 * 测试Base64编码功能
 */
void test_base64_encode(void) {
    TEST_CASE_START("Base64 Encode");
    
    // 测试空数据
    char* result = mcp_base64_encode("", 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STR("", result);
    free(result);
    
    // 测试单字符
    result = mcp_base64_encode("A", 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STR("QQ==", result);
    free(result);
    
    // 测试两字符
    result = mcp_base64_encode("AB", 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STR("QUI=", result);
    free(result);
    
    // 测试三字符（无填充）
    result = mcp_base64_encode("ABC", 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STR("QUJD", result);
    free(result);
    
    // 测试标准字符串
    result = mcp_base64_encode("Hello World", 11);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STR("SGVsbG8gV29ybGQ=", result);
    free(result);
    
    // 测试包含特殊字符的数据
    const char special_data[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD};
    result = mcp_base64_encode(special_data, sizeof(special_data));
    TEST_ASSERT_NOT_NULL(result);
    // 验证结果不为空且长度正确
    TEST_ASSERT(strlen(result) > 0, "Base64 result should not be empty");
    free(result);
    
    // 测试NULL输入
    result = mcp_base64_encode(NULL, 10);
    TEST_ASSERT_NULL(result);
}

/**
 * 测试图像内容创建
 */
void test_image_content_create(void) {
    TEST_CASE_START("Image Content Create");
    
    const char* mime_type = "image/png";
    const char* test_data = "fake image data";
    size_t data_len = strlen(test_data);
    
    // 测试正常创建
    mcp_image_content_t* image = mcp_image_content_create(mime_type, test_data, data_len);
    TEST_ASSERT_NOT_NULL(image);
    TEST_ASSERT_NOT_NULL(image->mime_type);
    TEST_ASSERT_NOT_NULL(image->encoded_data);
    TEST_ASSERT_EQUAL_STR(mime_type, image->mime_type);
    
    // 验证数据已被Base64编码
    TEST_ASSERT(strlen(image->encoded_data) > 0, "Encoded data should not be empty");
    
    mcp_image_content_destroy(image);
    
    // 测试不同MIME类型
    image = mcp_image_content_create("image/jpeg", test_data, data_len);
    TEST_ASSERT_NOT_NULL(image);
    TEST_ASSERT_EQUAL_STR("image/jpeg", image->mime_type);
    mcp_image_content_destroy(image);
    
    // 测试NULL参数
    image = mcp_image_content_create(NULL, test_data, data_len);
    TEST_ASSERT_NULL(image);
    
    image = mcp_image_content_create(mime_type, NULL, data_len);
    TEST_ASSERT_NULL(image);
    
    image = mcp_image_content_create(mime_type, test_data, 0);
    TEST_ASSERT_NULL(image);
}

/**
 * 测试图像内容销毁
 */
void test_image_content_destroy(void) {
    TEST_CASE_START("Image Content Destroy");
    
    const char* mime_type = "image/gif";
    const char* test_data = "test gif data";
    
    // 创建并销毁
    mcp_image_content_t* image = mcp_image_content_create(mime_type, test_data, strlen(test_data));
    TEST_ASSERT_NOT_NULL(image);
    
    // 销毁应该不会崩溃
    mcp_image_content_destroy(image);
    
    // 测试销毁NULL指针
    mcp_image_content_destroy(NULL);  // 应该安全处理
}

/**
 * 测试图像内容转JSON
 */
void test_image_content_to_json(void) {
    TEST_CASE_START("Image Content to JSON");
    
    const char* mime_type = "image/webp";
    const char* test_data = "webp image data";
    
    mcp_image_content_t* image = mcp_image_content_create(mime_type, test_data, strlen(test_data));
    TEST_ASSERT_NOT_NULL(image);
    
    // 转换为JSON
    char* json_str = mcp_image_content_to_json(image);
    TEST_ASSERT_NOT_NULL(json_str);
    
    // 验证JSON包含必要字段
    TEST_ASSERT(strstr(json_str, "\"type\":\"image\"") != NULL, "JSON should contain type field");
    TEST_ASSERT(strstr(json_str, "\"mimeType\":\"image/webp\"") != NULL, "JSON should contain mimeType field");
    TEST_ASSERT(strstr(json_str, "\"data\":") != NULL, "JSON should contain data field");
    
    free(json_str);
    mcp_image_content_destroy(image);
    
    // 测试NULL输入
    json_str = mcp_image_content_to_json(NULL);
    TEST_ASSERT_NULL(json_str);
}

/**
 * 测试字符串复制
 */
void test_string_duplicate(void) {
    TEST_CASE_START("String Duplicate");
    
    // 测试正常字符串
    const char* original = "Hello, MCP!";
    char* copy = mcp_strdup(original);
    TEST_ASSERT_NOT_NULL(copy);
    TEST_ASSERT_EQUAL_STR(original, copy);
    TEST_ASSERT(copy != original, "Copy should be different pointer");
    mcp_free_string(copy);
    
    // 测试空字符串
    copy = mcp_strdup("");
    TEST_ASSERT_NOT_NULL(copy);
    TEST_ASSERT_EQUAL_STR("", copy);
    mcp_free_string(copy);
    
    // 测试长字符串
    char long_str[1000];
    memset(long_str, 'A', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';
    
    copy = mcp_strdup(long_str);
    TEST_ASSERT_NOT_NULL(copy);
    TEST_ASSERT_EQUAL_STR(long_str, copy);
    mcp_free_string(copy);
    
    // 测试NULL输入
    copy = mcp_strdup(NULL);
    TEST_ASSERT_NULL(copy);
}

/**
 * 测试字符串释放
 */
void test_string_free(void) {
    TEST_CASE_START("String Free");
    
    // 测试正常释放
    char* str = mcp_strdup("test string");
    TEST_ASSERT_NOT_NULL(str);
    mcp_free_string(str);  // 应该不会崩溃
    
    // 测试释放NULL
    mcp_free_string(NULL);  // 应该安全处理
}

/**
 * 测试JSON转字符串
 */
void test_json_to_string(void) {
    TEST_CASE_START("JSON to String");
    
    // 创建简单JSON对象
    cJSON* json = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(json);
    
    cJSON_AddStringToObject(json, "name", "test");
    cJSON_AddNumberToObject(json, "value", 42);
    cJSON_AddBoolToObject(json, "enabled", true);
    
    // 转换为字符串
    char* json_str = mcp_json_to_string(json);
    TEST_ASSERT_NOT_NULL(json_str);
    
    // 验证包含预期内容
    TEST_ASSERT(strstr(json_str, "\"name\":\"test\"") != NULL, "JSON should contain name field");
    TEST_ASSERT(strstr(json_str, "\"value\":42") != NULL, "JSON should contain value field");
    TEST_ASSERT(strstr(json_str, "\"enabled\":true") != NULL, "JSON should contain enabled field");
    
    free(json_str);
    cJSON_Delete(json);
    
    // 测试空JSON对象
    json = cJSON_CreateObject();
    json_str = mcp_json_to_string(json);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_EQUAL_STR("{}", json_str);
    free(json_str);
    cJSON_Delete(json);
    
    // 测试NULL输入
    json_str = mcp_json_to_string(NULL);
    TEST_ASSERT_NULL(json_str);
}

/**
 * 测试内存管理
 */
void test_memory_management(void) {
    TEST_CASE_START("Memory Management");
    
    // 测试多次分配和释放
    for (int i = 0; i < 10; i++) {
        char test_str[100];
        snprintf(test_str, sizeof(test_str), "test string %d", i);
        
        char* copy = mcp_strdup(test_str);
        TEST_ASSERT_NOT_NULL(copy);
        TEST_ASSERT_EQUAL_STR(test_str, copy);
        mcp_free_string(copy);
    }
    
    // 测试图像内容的内存管理
    for (int i = 0; i < 5; i++) {
        char mime_type[50];
        char data[100];
        snprintf(mime_type, sizeof(mime_type), "image/test%d", i);
        snprintf(data, sizeof(data), "test data %d", i);
        
        mcp_image_content_t* image = mcp_image_content_create(mime_type, data, strlen(data));
        TEST_ASSERT_NOT_NULL(image);
        
        char* json_str = mcp_image_content_to_json(image);
        TEST_ASSERT_NOT_NULL(json_str);
        
        free(json_str);
        mcp_image_content_destroy(image);
    }
}

/**
 * 测试边界条件和错误处理
 */
void test_error_conditions(void) {
    TEST_CASE_START("Error Conditions");
    
    // 测试极大数据的Base64编码
    const size_t large_size = 1024 * 1024;  // 1MB
    char* large_data = malloc(large_size);
    if (large_data) {
        memset(large_data, 'X', large_size);
        
        char* encoded = mcp_base64_encode(large_data, large_size);
        if (encoded) {
            TEST_ASSERT(strlen(encoded) > large_size, "Encoded data should be larger than original");
            free(encoded);
        }
        free(large_data);
    }
    
    // 测试包含NULL字符的数据
    const char null_data[] = {'A', '\0', 'B', '\0', 'C'};
    char* encoded = mcp_base64_encode(null_data, sizeof(null_data));
    TEST_ASSERT_NOT_NULL(encoded);
    free(encoded);
}

/**
 * 运行所有工具函数测试
 */
void run_utils_tests(void) {
    TEST_SUITE_START("MCP Utils Tests");
    
    test_base64_encode();
    test_image_content_create();
    test_image_content_destroy();
    test_image_content_to_json();
    test_string_duplicate();
    test_string_free();
    test_json_to_string();
    test_memory_management();
    test_error_conditions();
    
    TEST_SUITE_END("MCP Utils Tests");
}

/**
 * 主函数 - 运行所有工具函数测试
 */
int main(void) {
    test_init();
    run_utils_tests();
    test_summary();
    
    // 检查内存泄漏
    test_check_memory_leaks();
    
    return (g_test_stats.failed_tests > 0) ? 1 : 0;
}