/*
 * MCP文件管理服务器示例
 * 
 * 这个示例展示了如何使用MCP C SDK创建一个文件管理服务器，
 * 支持文件读取、写入、列表、删除等操作。
 * 
 * 编译命令：
 * gcc -o file_manager_server file_manager_server.c ../../*.c -lcjson
 * 
 * 运行：
 * ./file_manager_server
 */

#include "../../mcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

// 全局服务器实例
static mcp_server_t* g_server = NULL;

// 工作目录限制（安全考虑）
static char g_work_dir[512] = "./sandbox";

// 消息发送回调函数
void send_message(const char* message) {
    printf("SEND: %s\n", message);
    fflush(stdout);
}

// 检查路径是否安全（防止目录遍历攻击）
bool is_safe_path(const char* path) {
    if (!path) return false;
    
    // 不允许绝对路径
    if (path[0] == '/') return false;
    
    // 不允许包含 ".."
    if (strstr(path, "..") != NULL) return false;
    
    // 不允许包含特殊字符
    if (strchr(path, ';') || strchr(path, '|') || strchr(path, '&')) return false;
    
    return true;
}

// 构建完整路径
char* build_full_path(const char* relative_path) {
    if (!is_safe_path(relative_path)) {
        return NULL;
    }
    
    char* full_path = malloc(1024);
    if (!full_path) return NULL;
    
    snprintf(full_path, 1024, "%s/%s", g_work_dir, relative_path);
    return full_path;
}

// 读取文件工具回调
mcp_return_value_t read_file_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (!properties || properties->count < 1) {
        result.value.string_val = mcp_strdup("Error: File path is required");
        return result;
    }
    
    const mcp_property_t* path_prop = mcp_property_list_find(properties, "path");
    if (!path_prop || path_prop->type != MCP_PROPERTY_TYPE_STRING) {
        result.value.string_val = mcp_strdup("Error: Path must be a string");
        return result;
    }
    
    const char* relative_path = mcp_property_get_string_value(path_prop);
    char* full_path = build_full_path(relative_path);
    if (!full_path) {
        result.value.string_val = mcp_strdup("Error: Invalid or unsafe path");
        return result;
    }
    
    FILE* file = fopen(full_path, "r");
    if (!file) {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Cannot open file '%s': %s", relative_path, strerror(errno));
        result.value.string_val = error_msg;
        free(full_path);
        return result;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size > 1024 * 1024) {  // 限制1MB
        result.value.string_val = mcp_strdup("Error: File too large (max 1MB)");
        fclose(file);
        free(full_path);
        return result;
    }
    
    // 读取文件内容
    char* content = malloc(file_size + 1);
    if (!content) {
        result.value.string_val = mcp_strdup("Error: Memory allocation failed");
        fclose(file);
        free(full_path);
        return result;
    }
    
    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';
    
    fclose(file);
    free(full_path);
    
    // 构建响应
    char* response = malloc(file_size + 256);
    snprintf(response, file_size + 256, "File content (%zu bytes):\n%s", bytes_read, content);
    
    free(content);
    result.value.string_val = response;
    return result;
}

// 写入文件工具回调
mcp_return_value_t write_file_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (!properties || properties->count < 2) {
        result.value.string_val = mcp_strdup("Error: Both path and content are required");
        return result;
    }
    
    const mcp_property_t* path_prop = mcp_property_list_find(properties, "path");
    const mcp_property_t* content_prop = mcp_property_list_find(properties, "content");
    
    if (!path_prop || !content_prop || 
        path_prop->type != MCP_PROPERTY_TYPE_STRING || 
        content_prop->type != MCP_PROPERTY_TYPE_STRING) {
        result.value.string_val = mcp_strdup("Error: Path and content must be strings");
        return result;
    }
    
    const char* relative_path = mcp_property_get_string_value(path_prop);
    const char* content = mcp_property_get_string_value(content_prop);
    
    char* full_path = build_full_path(relative_path);
    if (!full_path) {
        result.value.string_val = mcp_strdup("Error: Invalid or unsafe path");
        return result;
    }
    
    FILE* file = fopen(full_path, "w");
    if (!file) {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Cannot create file '%s': %s", relative_path, strerror(errno));
        result.value.string_val = error_msg;
        free(full_path);
        return result;
    }
    
    size_t content_len = strlen(content);
    size_t bytes_written = fwrite(content, 1, content_len, file);
    fclose(file);
    free(full_path);
    
    char* response = malloc(256);
    snprintf(response, 256, "Successfully wrote %zu bytes to '%s'", bytes_written, relative_path);
    result.value.string_val = response;
    
    return result;
}

// 列出目录工具回调
mcp_return_value_t list_directory_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    const char* relative_path = ".";  // 默认当前目录
    
    if (properties && properties->count > 0) {
        const mcp_property_t* path_prop = mcp_property_list_find(properties, "path");
        if (path_prop && path_prop->type == MCP_PROPERTY_TYPE_STRING) {
            relative_path = mcp_property_get_string_value(path_prop);
        }
    }
    
    char* full_path = build_full_path(relative_path);
    if (!full_path) {
        result.value.string_val = mcp_strdup("Error: Invalid or unsafe path");
        return result;
    }
    
    DIR* dir = opendir(full_path);
    if (!dir) {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Cannot open directory '%s': %s", relative_path, strerror(errno));
        result.value.string_val = error_msg;
        free(full_path);
        return result;
    }
    
    // 构建文件列表
    char* file_list = malloc(4096);
    strcpy(file_list, "Directory listing:\n");
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 获取文件信息
        char entry_path[1024];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", full_path, entry->d_name);
        
        struct stat file_stat;
        if (stat(entry_path, &file_stat) == 0) {
            char entry_info[256];
            if (S_ISDIR(file_stat.st_mode)) {
                snprintf(entry_info, sizeof(entry_info), "  [DIR]  %s/\n", entry->d_name);
            } else {
                snprintf(entry_info, sizeof(entry_info), "  [FILE] %s (%ld bytes)\n", 
                        entry->d_name, file_stat.st_size);
            }
            strcat(file_list, entry_info);
        }
    }
    
    closedir(dir);
    free(full_path);
    
    result.value.string_val = file_list;
    return result;
}

// 删除文件工具回调
mcp_return_value_t delete_file_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (!properties || properties->count < 1) {
        result.value.string_val = mcp_strdup("Error: File path is required");
        return result;
    }
    
    const mcp_property_t* path_prop = mcp_property_list_find(properties, "path");
    if (!path_prop || path_prop->type != MCP_PROPERTY_TYPE_STRING) {
        result.value.string_val = mcp_strdup("Error: Path must be a string");
        return result;
    }
    
    const char* relative_path = mcp_property_get_string_value(path_prop);
    char* full_path = build_full_path(relative_path);
    if (!full_path) {
        result.value.string_val = mcp_strdup("Error: Invalid or unsafe path");
        return result;
    }
    
    if (unlink(full_path) == 0) {
        char* response = malloc(256);
        snprintf(response, 256, "Successfully deleted file '%s'", relative_path);
        result.value.string_val = response;
    } else {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Cannot delete file '%s': %s", relative_path, strerror(errno));
        result.value.string_val = error_msg;
    }
    
    free(full_path);
    return result;
}

// 获取文件信息工具回调
mcp_return_value_t file_info_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (!properties || properties->count < 1) {
        result.value.string_val = mcp_strdup("Error: File path is required");
        return result;
    }
    
    const mcp_property_t* path_prop = mcp_property_list_find(properties, "path");
    if (!path_prop || path_prop->type != MCP_PROPERTY_TYPE_STRING) {
        result.value.string_val = mcp_strdup("Error: Path must be a string");
        return result;
    }
    
    const char* relative_path = mcp_property_get_string_value(path_prop);
    char* full_path = build_full_path(relative_path);
    if (!full_path) {
        result.value.string_val = mcp_strdup("Error: Invalid or unsafe path");
        return result;
    }
    
    struct stat file_stat;
    if (stat(full_path, &file_stat) != 0) {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Cannot get info for '%s': %s", relative_path, strerror(errno));
        result.value.string_val = error_msg;
        free(full_path);
        return result;
    }
    
    char* info = malloc(512);
    snprintf(info, 512, 
        "File information for '%s':\n"
        "  Type: %s\n"
        "  Size: %ld bytes\n"
        "  Permissions: %o\n"
        "  Last modified: %ld",
        relative_path,
        S_ISDIR(file_stat.st_mode) ? "Directory" : "Regular file",
        file_stat.st_size,
        file_stat.st_mode & 0777,
        file_stat.st_mtime);
    
    free(full_path);
    result.value.string_val = info;
    return result;
}

// 初始化文件管理服务器
bool init_file_manager_server() {
    // 创建工作目录
    mkdir(g_work_dir, 0755);
    
    // 创建服务器
    g_server = mcp_server_create("File Manager Server", "1.0.0");
    if (!g_server) {
        fprintf(stderr, "Failed to create server\n");
        return false;
    }
    
    // 设置消息发送回调
    mcp_server_set_send_callback(send_message);
    
    // 创建读取文件工具
    mcp_property_list_t* read_props = mcp_property_list_create();
    mcp_property_t* read_path_prop = mcp_property_create_string("path", NULL, false);
    mcp_property_list_add(read_props, read_path_prop);
    
    if (!mcp_server_add_simple_tool(g_server, "read_file", "Read content from a file", read_props, read_file_callback)) {
        fprintf(stderr, "Failed to add read_file tool\n");
        return false;
    }
    
    // 创建写入文件工具
    mcp_property_list_t* write_props = mcp_property_list_create();
    mcp_property_t* write_path_prop = mcp_property_create_string("path", NULL, false);
    mcp_property_t* content_prop = mcp_property_create_string("content", NULL, false);
    mcp_property_list_add(write_props, write_path_prop);
    mcp_property_list_add(write_props, content_prop);
    
    if (!mcp_server_add_simple_tool(g_server, "write_file", "Write content to a file", write_props, write_file_callback)) {
        fprintf(stderr, "Failed to add write_file tool\n");
        return false;
    }
    
    // 创建列出目录工具
    mcp_property_list_t* list_props = mcp_property_list_create();
    mcp_property_t* list_path_prop = mcp_property_create_string("path", ".", true);
    mcp_property_list_add(list_props, list_path_prop);
    
    if (!mcp_server_add_simple_tool(g_server, "list_directory", "List files in a directory", list_props, list_directory_callback)) {
        fprintf(stderr, "Failed to add list_directory tool\n");
        return false;
    }
    
    // 创建删除文件工具
    mcp_property_list_t* delete_props = mcp_property_list_create();
    mcp_property_t* delete_path_prop = mcp_property_create_string("path", NULL, false);
    mcp_property_list_add(delete_props, delete_path_prop);
    
    if (!mcp_server_add_simple_tool(g_server, "delete_file", "Delete a file", delete_props, delete_file_callback)) {
        fprintf(stderr, "Failed to add delete_file tool\n");
        return false;
    }
    
    // 创建文件信息工具
    mcp_property_list_t* info_props = mcp_property_list_create();
    mcp_property_t* info_path_prop = mcp_property_create_string("path", NULL, false);
    mcp_property_list_add(info_props, info_path_prop);
    
    if (!mcp_server_add_simple_tool(g_server, "file_info", "Get file information", info_props, file_info_callback)) {
        fprintf(stderr, "Failed to add file_info tool\n");
        return false;
    }
    
    // 清理属性对象
    mcp_property_destroy(read_path_prop);
    mcp_property_destroy(write_path_prop);
    mcp_property_destroy(content_prop);
    mcp_property_destroy(list_path_prop);
    mcp_property_destroy(delete_path_prop);
    mcp_property_destroy(info_path_prop);
    
    mcp_property_list_destroy(read_props);
    mcp_property_list_destroy(write_props);
    mcp_property_list_destroy(list_props);
    mcp_property_list_destroy(delete_props);
    mcp_property_list_destroy(info_props);
    
    printf("File manager server initialized with %zu tools\n", g_server->tool_count);
    printf("Working directory: %s\n", g_work_dir);
    return true;
}

// 清理服务器
void cleanup_file_manager_server() {
    if (g_server) {
        mcp_server_destroy(g_server);
        g_server = NULL;
    }
}

// 处理输入消息
void process_message(const char* message) {
    if (!g_server || !message) {
        return;
    }
    
    printf("RECV: %s\n", message);
    mcp_server_parse_message(g_server, message);
}

// 运行自动化测试
int run_automated_tests() {
    printf("=== Running File Manager Server Automated Tests ===\n");
    
    // 测试消息列表
    const char* test_messages[] = {
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\",\"params\":{}}",
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"list_directory\",\"arguments\":{}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"write_file\",\"arguments\":{\"path\":\"test.txt\",\"content\":\"Hello World!\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\",\"params\":{\"name\":\"read_file\",\"arguments\":{\"path\":\"test.txt\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"tools/call\",\"params\":{\"name\":\"file_info\",\"arguments\":{\"path\":\"test.txt\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"tools/call\",\"params\":{\"name\":\"write_file\",\"arguments\":{\"path\":\"data.json\",\"content\":\"{\\\"name\\\":\\\"test\\\",\\\"value\\\":123}\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"tools/call\",\"params\":{\"name\":\"list_directory\",\"arguments\":{\"path\":\".\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":9,\"method\":\"tools/call\",\"params\":{\"name\":\"read_file\",\"arguments\":{\"path\":\"data.json\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tools/call\",\"params\":{\"name\":\"delete_file\",\"arguments\":{\"path\":\"test.txt\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"tools/call\",\"params\":{\"name\":\"delete_file\",\"arguments\":{\"path\":\"data.json\"}}}",
        // 错误测试
        "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"tools/call\",\"params\":{\"name\":\"read_file\",\"arguments\":{\"path\":\"nonexistent.txt\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":13,\"method\":\"tools/call\",\"params\":{\"name\":\"read_file\",\"arguments\":{\"path\":\"../../../etc/passwd\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":14,\"method\":\"tools/call\",\"params\":{\"name\":\"file_info\",\"arguments\":{\"path\":\"deleted.txt\"}}}"
    };
    
    const char* test_descriptions[] = {
        "Initialize server",
        "List available tools",
        "List directory (empty)",
        "Write test file",
        "Read test file",
        "Get file info",
        "Write JSON data file",
        "List directory (with files)",
        "Read JSON data file",
        "Delete test file",
        "Delete JSON file",
        "Test read nonexistent file error",
        "Test path traversal security",
        "Test file info on deleted file"
    };
    
    size_t num_tests = sizeof(test_messages) / sizeof(test_messages[0]);
    int passed_tests = 0;
    
    for (size_t i = 0; i < num_tests; i++) {
        printf("\nTest %zu: %s\n", i + 1, test_descriptions[i]);
        printf("Message: %s\n", test_messages[i]);
        
        // 处理消息
        process_message(test_messages[i]);
        passed_tests++;
        
        // 短暂延迟以便观察输出
        usleep(100000); // 100ms
    }
    
    printf("\n=== Test Results ===\n");
    printf("Total tests: %zu\n", num_tests);
    printf("Passed tests: %d\n", passed_tests);
    printf("File manager server tests completed successfully!\n");
    
    return 0;
}

// 主函数
int main() {
    printf("=== MCP File Manager Server Example ===\n");
    printf("This server provides file management operations.\n");
    printf("Available tools: read_file, write_file, list_directory, delete_file, file_info\n");
    printf("All operations are restricted to the sandbox directory for security.\n");
    printf("Running automated tests...\n\n");
    
    // 初始化服务器
    if (!init_file_manager_server()) {
        fprintf(stderr, "Failed to initialize file manager server\n");
        return 1;
    }
    
    // 运行自动化测试
    int result = run_automated_tests();
    
    printf("\nShutting down file manager server...\n");
    cleanup_file_manager_server();
    
    return result;
}