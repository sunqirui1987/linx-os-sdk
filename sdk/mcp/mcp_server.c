/*
 * MCP服务器实现文件
 * 实现MCP服务器的核心功能，包括工具管理、消息处理和能力配置
 */

#include "mcp_server.h"
#include "mcp.h"        // 包含MCP协议版本定义
#include "../log/linx_log.h"  // 日志模块
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 全局消息发送回调函数 */
static mcp_send_message_callback_t g_send_callback = NULL;

/**
 * 创建MCP服务器实例
 */
mcp_server_t* mcp_server_create(const char* server_name, const char* server_version) {
    if (!server_name || !server_version) {
        LOG_ERROR("Invalid parameters: server_name=%p, server_version=%p", server_name, server_version);
        return NULL;
    }
    
    LOG_INFO("Creating MCP server: name='%s', version='%s'", server_name, server_version);
    
    mcp_server_t* server = malloc(sizeof(mcp_server_t));
    if (!server) {
        LOG_ERROR("Failed to allocate memory for MCP server");
        return NULL;
    }
    
    // 初始化工具数组
    memset(server->tools, 0, sizeof(server->tools));
    server->tool_count = 0;
    
    // 复制服务器名称
    strncpy(server->server_name, server_name, MCP_MAX_NAME_LENGTH - 1);
    server->server_name[MCP_MAX_NAME_LENGTH - 1] = '\0';
    
    // 复制服务器版本
    strncpy(server->server_version, server_version, sizeof(server->server_version) - 1);
    server->server_version[sizeof(server->server_version) - 1] = '\0';
    
    /* 初始化能力回调结构体 */
    memset(&server->capability_callbacks, 0, sizeof(server->capability_callbacks));
    
    LOG_DEBUG("MCP server created successfully: %p", server);
    return server;
}

/**
 * 销毁MCP服务器实例
 */
void mcp_server_destroy(mcp_server_t* server) {
    if (server) {
        LOG_INFO("Destroying MCP server: %p (name='%s', tools=%zu)", server, server->server_name, server->tool_count);
        
        // 销毁所有工具
        for (size_t i = 0; i < server->tool_count; i++) {
            if (server->tools[i]) {
                LOG_DEBUG("Destroying tool %zu: '%s'", i, server->tools[i]->name);
                mcp_tool_destroy(server->tools[i]);
                server->tools[i] = NULL;  // 防止多次释放
            }
        }
        // 清理服务器状态
        server->tool_count = 0;
        memset(server->tools, 0, sizeof(server->tools));
        free(server);
        server = NULL;
        
        LOG_DEBUG("MCP server destroyed successfully");
    } else {
        LOG_WARN("Attempted to destroy NULL MCP server");
    }
}

/**
 * 向服务器添加工具
 */
bool mcp_server_add_tool(mcp_server_t* server, mcp_tool_t* tool) {
    if (!server || !tool || server->tool_count >= MCP_MAX_TOOLS) {
        LOG_ERROR("Invalid parameters or tool limit reached: server=%p, tool=%p, count=%zu/%d", 
                  server, tool, server ? server->tool_count : 0, MCP_MAX_TOOLS);
        return false;
    }
    
    LOG_DEBUG("Adding tool '%s' to server '%s'", tool->name, server->server_name);
    
    /* 检查重复的工具名称 */
    for (size_t i = 0; i < server->tool_count; i++) {
        if (strcmp(server->tools[i]->name, tool->name) == 0) {
            LOG_WARN("Tool with name '%s' already exists in server", tool->name);
            return false;
        }
    }
    
    server->tools[server->tool_count] = tool;
    server->tool_count++;
    
    LOG_INFO("Tool '%s' added successfully to server '%s' (total tools: %zu)", 
             tool->name, server->server_name, server->tool_count);
    return true;
}

/**
 * 向服务器添加简单工具
 */
bool mcp_server_add_simple_tool(mcp_server_t* server, const char* name, const char* description,
                                mcp_property_list_t* properties, mcp_tool_callback_t callback) {
    mcp_tool_t* tool = mcp_tool_create(name, description, properties, callback);
    if (!tool) {
        return false;
    }
    
    if (!mcp_server_add_tool(server, tool)) {
        mcp_tool_destroy(tool);
        return false;
    }
    
    return true;
}

/**
 * 向服务器添加仅用户可见的工具
 */
bool mcp_server_add_user_only_tool(mcp_server_t* server, const char* name, const char* description,
                                   mcp_property_list_t* properties, mcp_tool_callback_t callback) {
    mcp_tool_t* tool = mcp_tool_create(name, description, properties, callback);
    if (!tool) {
        return false;
    }
    
    // 设置为仅用户可见
    mcp_tool_set_user_only(tool, true);
    
    if (!mcp_server_add_tool(server, tool)) {
        mcp_tool_destroy(tool);
        return false;
    }
    
    return true;
}

/**
 * 根据名称查找工具
 */
const mcp_tool_t* mcp_server_find_tool(const mcp_server_t* server, const char* name) {
    if (!server || !name) {
        return NULL;
    }
    
    for (size_t i = 0; i < server->tool_count; i++) {
        if (strcmp(server->tools[i]->name, name) == 0) {
            return server->tools[i];
        }
    }
    
    return NULL;
}

/**
 * 设置消息发送回调函数
 */
void mcp_server_set_send_callback(mcp_send_message_callback_t callback) {
    g_send_callback = callback;
}

/**
 * 解析字符串消息
 */
void mcp_server_parse_message(mcp_server_t* server, const char* message) {
    if (!server || !message) {
        return;
    }
    
    cJSON* json = cJSON_Parse(message);
    if (!json) {
        return;
    }
    
    mcp_server_parse_json_message(server, json);
    cJSON_Delete(json);
}

/**
 * 解析JSON消息
 */
void mcp_server_parse_json_message(mcp_server_t* server, const cJSON* json) {
    if (!server || !json) {
        LOG_ERROR("Invalid parameters: server=%p, json=%p", server, json);
        return;
    }
    
    LOG_DEBUG("Parsing JSON message for server '%s'", server->server_name);
    
    /* 检查JSONRPC版本 */
    const cJSON* version = cJSON_GetObjectItem(json, "jsonrpc");
    if (!version || !cJSON_IsString(version) || strcmp(version->valuestring, "2.0") != 0) {
        LOG_WARN("Invalid or missing JSONRPC version");
        return;
    }
    
    /* 检查方法名 */
    const cJSON* method = cJSON_GetObjectItem(json, "method");
    if (!method || !cJSON_IsString(method)) {
        LOG_WARN("Invalid or missing method name");
        return;
    }
    
    LOG_DEBUG("Processing method: '%s'", method->valuestring);
    
    /* 跳过通知消息 */
    if (strstr(method->valuestring, "notifications") == method->valuestring) {
        LOG_DEBUG("Skipping notification message: '%s'", method->valuestring);
        return;
    }
    
    /* 检查参数 */
    const cJSON* params = cJSON_GetObjectItem(json, "params");
    if (params && !cJSON_IsObject(params)) {
        return;
    }
    
    /* 检查请求ID */
    const cJSON* id = cJSON_GetObjectItem(json, "id");
    if (!id || !cJSON_IsNumber(id)) {
        return;
    }
    
    int id_int = id->valueint;
    const char* method_str = method->valuestring;
    
    // 根据方法名分发处理
    LOG_INFO("Handling method '%s' with ID %d", method_str, id_int);
    
    if (strcmp(method_str, "initialize") == 0) {
        mcp_server_handle_initialize(server, id_int, params);
    } else if (strcmp(method_str, "tools/list") == 0) {
        mcp_server_handle_tools_list(server, id_int, params);
    } else if (strcmp(method_str, "tools/call") == 0) {
        mcp_server_handle_tools_call(server, id_int, params);
    } else {
        LOG_WARN("Method not implemented: %s", method_str);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Method not implemented: %s", method_str);
        mcp_server_reply_error(id_int, error_msg);
    }
}

/**
 * 回复成功结果
 */
void mcp_server_reply_result(int id, const char* result) {
    if (!g_send_callback || !result) {
        return;
    }
    
    char* payload = malloc(strlen(result) + 128);
    if (!payload) {
        return;
    }
    
    sprintf(payload, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}", id, result);
    g_send_callback(payload);
    free(payload);
}

/**
 * 回复错误信息
 */
void mcp_server_reply_error(int id, const char* message) {
    if (!g_send_callback || !message) {
        return;
    }
    
    char* payload = malloc(strlen(message) + 128);
    if (!payload) {
        return;
    }
    
    sprintf(payload, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"error\":{\"message\":\"%s\"}}", id, message);
    g_send_callback(payload);
    free(payload);
}

/**
 * 设置能力回调函数
 */
void mcp_server_set_capability_callbacks(mcp_server_t* server, const mcp_capability_callbacks_t* callbacks) {
    if (server && callbacks) {
        server->capability_callbacks = *callbacks;
    }
}

/**
 * 解析能力配置
 */
void mcp_server_parse_capabilities(mcp_server_t* server, const cJSON* capabilities) {
    if (!server || !capabilities) {
        return;
    }
    
    // 解析摄像头能力配置
    const cJSON* camera = cJSON_GetObjectItem(capabilities, "camera");
    if (camera && cJSON_IsObject(camera)) {
        const cJSON* explain_url = cJSON_GetObjectItem(camera, "explain_url");
        const cJSON* token = cJSON_GetObjectItem(camera, "token");
        
        if (explain_url && cJSON_IsString(explain_url) && 
            token && cJSON_IsString(token) &&
            server->capability_callbacks.camera_set_explain_url) {
            // 调用摄像头解释URL设置回调
            server->capability_callbacks.camera_set_explain_url(
                explain_url->valuestring, 
                token->valuestring
            );
        }
    }
    
    // 可以在这里添加其他能力的解析逻辑
}

/**
 * 处理初始化请求
 */
void mcp_server_handle_initialize(mcp_server_t* server, int id, const cJSON* params) {
    if (!server) {
        mcp_server_reply_error(id, "Server not initialized");
        return;
    }
    
    // 解析客户端能力配置
    if (params) {
        const cJSON* capabilities = cJSON_GetObjectItem(params, "capabilities");
        if (capabilities) {
            mcp_server_parse_capabilities(server, capabilities);
        }
    }
    
    // 构建初始化响应
    char result[512];
    snprintf(result, sizeof(result), 
        "{\"protocolVersion\":\"%s\",\"capabilities\":{\"tools\":{\"listChanged\":false}},\"serverInfo\":{\"name\":\"%s\",\"version\":\"%s\"}}",
        MCP_PROTOCOL_VERSION, server->server_name, server->server_version);
    
    mcp_server_reply_result(id, result);
}

/**
 * 处理工具列表请求
 */
void mcp_server_handle_tools_list(mcp_server_t* server, int id, const cJSON* params) {
    if (!server) {
        mcp_server_reply_error(id, "Server not initialized");
        return;
    }
    
    const char* cursor = NULL;
    bool list_user_only_tools = false;
    
    // 解析参数
    if (params) {
        const cJSON* cursor_json = cJSON_GetObjectItem(params, "cursor");
        if (cursor_json && cJSON_IsString(cursor_json)) {
            cursor = cursor_json->valuestring;
        }
        
        const cJSON* user_only = cJSON_GetObjectItem(params, "listUserOnlyTools");
        if (user_only && cJSON_IsBool(user_only)) {
            list_user_only_tools = cJSON_IsTrue(user_only);
        }
    }
    
    // 获取工具列表JSON
    char* tools_json = mcp_server_get_tools_list_json(server, cursor, list_user_only_tools);
    if (tools_json) {
        mcp_server_reply_result(id, tools_json);
        free(tools_json);
    } else {
        mcp_server_reply_error(id, "Failed to generate tools list");
    }
}

/**
 * 处理工具调用请求
 */
void mcp_server_handle_tools_call(mcp_server_t* server, int id, const cJSON* params) {
    if (!server || !params) {
        mcp_server_reply_error(id, "Invalid parameters");
        return;
    }
    
    // 获取工具名称
    const cJSON* name_json = cJSON_GetObjectItem(params, "name");
    if (!name_json || !cJSON_IsString(name_json)) {
        mcp_server_reply_error(id, "Tool name is required");
        return;
    }
    
    const char* tool_name = name_json->valuestring;
    
    // 查找工具
    const mcp_tool_t* tool = mcp_server_find_tool(server, tool_name);
    if (!tool) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Tool not found: %s", tool_name);
        mcp_server_reply_error(id, error_msg);
        return;
    }
    
    // 解析工具参数
    const cJSON* arguments = cJSON_GetObjectItem(params, "arguments");
    mcp_property_list_t* properties = NULL;
    
    if (arguments && cJSON_IsObject(arguments)) {
        // 将JSON参数转换为属性列表
        properties = mcp_property_list_create();
        if (properties) {
            // 遍历JSON对象的所有字段
            const cJSON* arg = NULL;
            cJSON_ArrayForEach(arg, arguments) {
                const char* key = arg->string;
                if (key) {
                    if (cJSON_IsBool(arg)) {
                        mcp_property_t* prop = mcp_property_create_boolean(key, cJSON_IsTrue(arg), true);
                        if (prop) {
                            mcp_property_list_add(properties, prop);
                            mcp_property_destroy(prop);
                        }
                    } else if (cJSON_IsNumber(arg)) {
                        mcp_property_t* prop = mcp_property_create_integer(key, arg->valueint, true, false, 0, 0);
                        if (prop) {
                            mcp_property_list_add(properties, prop);
                            mcp_property_destroy(prop);
                        }
                    } else if (cJSON_IsString(arg)) {
                        mcp_property_t* prop = mcp_property_create_string(key, arg->valuestring, true);
                        if (prop) {
                            mcp_property_list_add(properties, prop);
                            mcp_property_destroy(prop);
                        }
                    }
                }
            }
        }
    }
    
    // 调用工具回调函数
    mcp_return_value_t result = tool->callback(properties);
    
    // 清理属性列表
    if (properties) {
        mcp_property_list_destroy(properties);
    }
    
    // 构建响应
    char* response = NULL;
    bool is_error = false;
    
    switch (result.type) {
        case MCP_RETURN_TYPE_BOOL:
            response = malloc(128);
            if (response) {
                snprintf(response, 128, "{\"content\":[{\"type\":\"text\",\"text\":\"%s\"}],\"isError\":false}", 
                        result.value.bool_val ? "true" : "false");
            }
            break;
        case MCP_RETURN_TYPE_INT:
            response = malloc(128);
            if (response) {
                snprintf(response, 128, "{\"content\":[{\"type\":\"text\",\"text\":\"%d\"}],\"isError\":false}", 
                        result.value.int_val);
            }
            break;
        case MCP_RETURN_TYPE_STRING:
            if (result.value.string_val) {
                size_t len = strlen(result.value.string_val) + 128;
                response = malloc(len);
                if (response) {
                    snprintf(response, len, "{\"content\":[{\"type\":\"text\",\"text\":\"%s\"}],\"isError\":false}", 
                            result.value.string_val);
                }
            }
            break;
        case MCP_RETURN_TYPE_JSON:
            if (result.value.json_val) {
                char* json_str = cJSON_Print(result.value.json_val);
                if (json_str) {
                    size_t len = strlen(json_str) + 128;
                    response = malloc(len);
                    if (response) {
                        snprintf(response, len, "{\"content\":[{\"type\":\"text\",\"text\":%s}],\"isError\":false}", json_str);
                    }
                    free(json_str);
                }
            }
            break;
        case MCP_RETURN_TYPE_IMAGE:
            if (result.value.image_val) {
                char* image_json = mcp_image_content_to_json(result.value.image_val);
                if (image_json) {
                    size_t len = strlen(image_json) + 128;
                    response = malloc(len);
                    if (response) {
                        snprintf(response, len, "{\"content\":[%s],\"isError\":false}", image_json);
                    }
                    free(image_json);
                }
            }
            break;
        default:
            response = malloc(256);
            if (response) {
                strcpy(response, "{\"content\":[{\"type\":\"text\",\"text\":\"Unsupported return type\"}],\"isError\":true}");
            }
            is_error = true;
            break;
    }
    
    // 清理返回值资源
    mcp_return_value_cleanup(&result, result.type);
    
    if (response) {
        if (is_error) {
            mcp_server_reply_error(id, "Tool execution failed");
        } else {
            mcp_server_reply_result(id, response);
        }
        free(response);
    } else {
        mcp_server_reply_error(id, "Failed to process tool result - memory allocation error");
    }
}

/**
 * 获取工具列表的JSON字符串
 */
char* mcp_server_get_tools_list_json(const mcp_server_t* server, const char* cursor, bool list_user_only_tools) {
    if (!server) {
        return NULL;
    }
    
    cJSON* root = cJSON_CreateObject();
    if (!root) {
        return NULL;
    }
    
    cJSON* tools_array = cJSON_CreateArray();
    if (!tools_array) {
        cJSON_Delete(root);
        return NULL;
    }
    
    // 添加工具到数组
    for (size_t i = 0; i < server->tool_count; i++) {
        const mcp_tool_t* tool = server->tools[i];
        
        // 根据过滤条件决定是否包含此工具
        if (list_user_only_tools && !mcp_tool_is_user_only(tool)) {
            continue;
        }
        
        char* tool_json_str = mcp_tool_to_json(tool);
        if (tool_json_str) {
            cJSON* tool_json = cJSON_Parse(tool_json_str);
            if (tool_json) {
                cJSON_AddItemToArray(tools_array, tool_json);
            }
            free(tool_json_str);
        }
    }
    
    cJSON_AddItemToObject(root, "tools", tools_array);
    
    // 添加分页信息（如果需要）
    if (cursor) {
        cJSON_AddStringToObject(root, "nextCursor", cursor);
    }
    
    char* json_string = cJSON_Print(root);
    cJSON_Delete(root);
    
    return json_string;
}