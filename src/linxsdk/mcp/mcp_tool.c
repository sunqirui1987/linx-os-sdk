/*
 * MCP工具管理实现文件
 * 实现工具的创建、销毁、调用和返回值处理功能
 */

#include "mcp_tool.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * 创建工具
 */
mcp_tool_t* mcp_tool_create(const char* name, const char* description,
                           mcp_property_list_t* properties, mcp_tool_callback_t callback) {
    // 检查参数有效性
    if (!name || !description || !callback) {
        LOG_ERROR("Invalid parameters: name=%p, description=%p, callback=%p", name, description, callback);
        return NULL;
    }
    
    // 检查名称不能为空字符串
    if (strlen(name) == 0) {
        LOG_ERROR("Tool name cannot be empty");
        return NULL;
    }
    
    // 检查名称和描述长度
    if (strlen(name) >= MCP_MAX_NAME_LENGTH || strlen(description) >= MCP_MAX_DESCRIPTION_LENGTH) {
        LOG_ERROR("Tool name or description too long: name_len=%zu, desc_len=%zu", 
                  strlen(name), strlen(description));
        return NULL;
    }
    
    LOG_INFO("Creating tool: '%s'", name);
    
    // 分配内存
    mcp_tool_t* tool = malloc(sizeof(mcp_tool_t));
    if (!tool) {
        LOG_ERROR("Failed to allocate memory for tool '%s'", name);
        return NULL;
    }
    
    // 初始化工具
    strncpy(tool->name, name, MCP_MAX_NAME_LENGTH - 1);
    tool->name[MCP_MAX_NAME_LENGTH - 1] = '\0';
    
    strncpy(tool->description, description, MCP_MAX_DESCRIPTION_LENGTH - 1);
    tool->description[MCP_MAX_DESCRIPTION_LENGTH - 1] = '\0';
    
    // 如果没有提供属性列表，创建一个空的属性列表
    if (properties == NULL) {
        LOG_DEBUG("Creating empty property list for tool '%s'", name);
        tool->properties = mcp_property_list_create();
        if (!tool->properties) {
            LOG_ERROR("Failed to create property list for tool '%s'", name);
            free(tool);
            return NULL;
        }
    } else {
        // 创建属性列表的深拷贝，避免所有权问题
        LOG_DEBUG("Cloning property list for tool '%s'", name);
        tool->properties = mcp_property_list_clone(properties);
        if (!tool->properties) {
            LOG_ERROR("Failed to clone property list for tool '%s'", name);
            free(tool);
            return NULL;
        }
    }
    
    tool->callback = callback;
    tool->user_only = false;
    
    LOG_INFO("Tool '%s' created successfully", name);
    return tool;
}

/**
 * 销毁工具并释放内存
 */
void mcp_tool_destroy(mcp_tool_t* tool) {
    if (tool) {
        LOG_INFO("Destroying tool: '%s'", tool->name);
        
        // 销毁属性列表
        if (tool->properties) {
            LOG_DEBUG("Destroying property list for tool '%s'", tool->name);
            mcp_property_list_destroy(tool->properties);
            tool->properties = NULL;  // 防止多次释放
        }
        
        // 清理工具状态
        memset(tool->name, 0, sizeof(tool->name));
        memset(tool->description, 0, sizeof(tool->description));
        tool->callback = NULL;
        tool->user_only = false;
        
        // 释放工具本身
        free(tool);
        tool = NULL;  // 防止野指针
        
        LOG_DEBUG("Tool destroyed successfully");
    } else {
        LOG_WARN("Attempted to destroy NULL tool");
    }
}

/**
 * 设置工具是否仅限用户使用
 */
void mcp_tool_set_user_only(mcp_tool_t* tool, bool user_only) {
    if (tool) {
        tool->user_only = user_only;
    }
}

/**
 * 检查工具是否仅限用户使用
 */
bool mcp_tool_is_user_only(const mcp_tool_t* tool) {
    if (!tool) {
        return false;
    }
    return tool->user_only;
}

/**
 * 将工具转换为JSON字符串
 * @param tool 工具指针
 * @return JSON字符串，需要调用者释放内存
 */
char* mcp_tool_to_json(const mcp_tool_t* tool) {
    if (!tool) {
        return NULL;
    }
    
    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return NULL;
    }
    
    // 添加基本信息
    cJSON_AddStringToObject(json, "name", tool->name);
    cJSON_AddStringToObject(json, "description", tool->description);
    
    /* 创建输入模式 */
    cJSON* input_schema = cJSON_CreateObject();
    cJSON_AddStringToObject(input_schema, "type", "object");
    
    // 添加属性信息
    if (tool->properties) {
        char* properties_json_str = mcp_property_list_to_json(tool->properties);
        if (properties_json_str) {
            cJSON* properties_json = cJSON_Parse(properties_json_str);
            if (properties_json) {
                cJSON_AddItemToObject(input_schema, "properties", properties_json);
            }
            free(properties_json_str);
        }
        
        // 添加必需属性
        char* required_json_str = mcp_property_list_get_required_json(tool->properties);
        if (required_json_str) {
            cJSON* required_json = cJSON_Parse(required_json_str);
            if (required_json && cJSON_GetArraySize(required_json) > 0) {
                cJSON_AddItemToObject(input_schema, "required", required_json);
            } else {
                cJSON_Delete(required_json);
            }
            free(required_json_str);
        }
    }
    
    cJSON_AddItemToObject(json, "inputSchema", input_schema);
    
    /* 如果仅限用户使用，添加注解 */
    if (tool->user_only) {
        cJSON* annotations = cJSON_CreateObject();
        cJSON* audience = cJSON_CreateArray();
        cJSON_AddItemToArray(audience, cJSON_CreateString("user"));
        cJSON_AddItemToObject(annotations, "audience", audience);
        cJSON_AddItemToObject(json, "annotations", annotations);
    }
    
    // 转换为字符串
    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    
    return json_str;
}

/**
 * 调用工具并获取结果
 */
char* mcp_tool_call(const mcp_tool_t* tool, const mcp_property_list_t* properties) {
    if (!tool || !tool->callback) {
        LOG_ERROR("Invalid tool or callback: tool=%p, callback=%p", tool, tool ? tool->callback : NULL);
        return NULL;
    }
    
    LOG_INFO("Calling tool: '%s'", tool->name);
    
    // 调用工具回调函数
    mcp_return_value_t result = tool->callback(properties);
    
    LOG_DEBUG("Tool '%s' callback completed, result type: %d", tool->name, result.type);
    
    // 创建结果JSON对象
    cJSON* json = cJSON_CreateObject();
    if (!json) {
        LOG_ERROR("Failed to create JSON object for tool '%s' result", tool->name);
        return NULL;
    }
    
    // 根据返回值类型处理结果
    LOG_DEBUG("Processing result for tool '%s', type: %d", tool->name, result.type);
    switch (result.type) {
        case MCP_RETURN_TYPE_BOOL:
            cJSON_AddBoolToObject(json, "result", result.value.bool_val);
            break;
            
        case MCP_RETURN_TYPE_INT:
            cJSON_AddNumberToObject(json, "result", result.value.int_val);
            break;
            
        case MCP_RETURN_TYPE_STRING:
            if (result.value.string_val) {
                cJSON_AddStringToObject(json, "result", result.value.string_val);
            } else {
                cJSON_AddNullToObject(json, "result");
            }
            break;
            
        case MCP_RETURN_TYPE_JSON:
            if (result.value.json_val) {
                cJSON_AddItemToObject(json, "result", cJSON_Duplicate(result.value.json_val, 1));
            } else {
                cJSON_AddNullToObject(json, "result");
            }
            break;
            
        case MCP_RETURN_TYPE_IMAGE:
            if (result.value.image_val) {
                cJSON* image_json = cJSON_CreateObject();
                cJSON_AddStringToObject(image_json, "type", "image");
                cJSON_AddStringToObject(image_json, "data", result.value.image_val->encoded_data);
                cJSON_AddStringToObject(image_json, "mimeType", result.value.image_val->mime_type);
                cJSON_AddItemToObject(json, "result", image_json);
            } else {
                cJSON_AddNullToObject(json, "result");
            }
            break;
            
        default:
            cJSON_AddNullToObject(json, "result");
            break;
    }
    
    // 转换为字符串
    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    
    // 清理返回值
    mcp_return_value_cleanup((mcp_return_value_t*)&result, result.type);
    
    if (json_str) {
        LOG_INFO("Tool '%s' call completed successfully", tool->name);
        LOG_DEBUG("Tool '%s' result: %s", tool->name, json_str);
    } else {
        LOG_ERROR("Failed to serialize result for tool '%s'", tool->name);
    }
    
    return json_str;
}

/**
 * 创建布尔类型返回值
 */
mcp_return_value_t mcp_return_bool(bool value) {
    mcp_return_value_t ret_val;
    ret_val.type = MCP_RETURN_TYPE_BOOL;
    ret_val.value.bool_val = value;
    return ret_val;
}

/**
 * 创建整数类型返回值
 */
mcp_return_value_t mcp_return_int(int value) {
    mcp_return_value_t ret_val;
    ret_val.type = MCP_RETURN_TYPE_INT;
    ret_val.value.int_val = value;
    return ret_val;
}

/**
 * 创建字符串类型返回值
 */
mcp_return_value_t mcp_return_string(const char* value) {
    mcp_return_value_t ret_val;
    ret_val.type = MCP_RETURN_TYPE_STRING;
    ret_val.value.string_val = value ? mcp_strdup(value) : NULL;
    return ret_val;
}

/**
 * 创建JSON类型返回值
 */
mcp_return_value_t mcp_return_json(cJSON* value) {
    mcp_return_value_t ret_val;
    ret_val.type = MCP_RETURN_TYPE_JSON;
    ret_val.value.json_val = value;
    return ret_val;
}

/**
 * 创建图像类型返回值
 */
mcp_return_value_t mcp_return_image(mcp_image_content_t* value) {
    mcp_return_value_t ret_val;
    ret_val.type = MCP_RETURN_TYPE_IMAGE;
    ret_val.value.image_val = value;
    return ret_val;
}

/**
 * 清理返回值并释放内存
 */
void mcp_return_value_cleanup(mcp_return_value_t* ret_val, mcp_return_type_t type) {
    if (!ret_val) {
        return;
    }
    
    // 根据类型清理相应的资源
    switch (type) {
        case MCP_RETURN_TYPE_STRING:
            if (ret_val->value.string_val) {
                free((void*)ret_val->value.string_val);
                ret_val->value.string_val = NULL;
            }
            break;
            
        case MCP_RETURN_TYPE_JSON:
            if (ret_val->value.json_val) {
                cJSON_Delete(ret_val->value.json_val);
                ret_val->value.json_val = NULL;
            }
            break;
            
        case MCP_RETURN_TYPE_IMAGE:
            if (ret_val->value.image_val) {
                if (ret_val->value.image_val->encoded_data) {
                    free(ret_val->value.image_val->encoded_data);
                }
                if (ret_val->value.image_val->mime_type) {
                    free(ret_val->value.image_val->mime_type);
                }
                free(ret_val->value.image_val);
                ret_val->value.image_val = NULL;
            }
            break;
            
        case MCP_RETURN_TYPE_BOOL:
        case MCP_RETURN_TYPE_INT:
        default:
            // 这些类型不需要特殊清理
            break;
    }
}