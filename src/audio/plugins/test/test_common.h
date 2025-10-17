/**
 * @file test_common.h
 * @brief 测试公共头文件
 * @details 包含所有测试文件共用的定义和基类
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>

extern "C" {
    #include "plugin_interface.h"
    #include "builtin/gain.h"
    #include "core/types.h"
}

/**
 * @brief 插件测试基类
 * @details 提供通用的测试工具函数
 */
class PluginTestBase : public ::testing::Test {
protected:
    /**
     * @brief 创建默认配置
     */
    linx_plugin_config_t* CreateDefaultConfig() {
        linx_plugin_config_t* config = (linx_plugin_config_t*)malloc(sizeof(linx_plugin_config_t));
        if (config) {
            linx_audio_result_t result = linx_plugin_config_create(config);
            if (result != LINX_AUDIO_SUCCESS) {
                free(config);
                return nullptr;
            }
            
            // 添加默认参数
            linx_plugin_config_add_param(config, "sample_rate", "44100");
            linx_plugin_config_add_param(config, "channels", "2");
            linx_plugin_config_add_param(config, "format", "f32le");
            linx_plugin_config_add_param(config, "frames_per_buffer", "1024");
        }
        return config;
    }
    
    /**
     * @brief 销毁插件配置
     */
    void DestroyConfig(linx_plugin_config_t* config) {
        if (config) {
            linx_plugin_config_destroy(config);
            free(config);
        }
    }
    
    /**
     * @brief 创建音频缓冲区
     */
    linx_audio_buffer_t* CreateAudioBuffer(void* data, size_t size, size_t frames) {
        linx_audio_buffer_t* buffer = (linx_audio_buffer_t*)malloc(sizeof(linx_audio_buffer_t));
        if (buffer) {
            memset(buffer, 0, sizeof(linx_audio_buffer_t));
            buffer->data = data;
            buffer->size = size;
            buffer->frames = frames;
            
            // 设置音频参数
            buffer->params.sample_rate = 44100;
            buffer->params.channels = 2;
            buffer->params.format = LINX_AUDIO_FORMAT_F32LE;
            buffer->params.bits_per_sample = 32;
            buffer->params.frame_size = 8; // 2 channels * 4 bytes per sample
            buffer->params.buffer_size = frames;
        }
        return buffer;
    }
    
    /**
     * @brief 创建测试缓冲区
     */
    linx_audio_buffer_t* CreateTestBuffer(uint32_t frames = 1024) {
        linx_audio_buffer_t* buffer = (linx_audio_buffer_t*)malloc(sizeof(linx_audio_buffer_t));
        if (buffer) {
            memset(buffer, 0, sizeof(linx_audio_buffer_t));
            
            // 设置音频参数
            buffer->params.sample_rate = 44100;
            buffer->params.channels = 2;
            buffer->params.format = LINX_AUDIO_FORMAT_F32LE;
            buffer->params.bits_per_sample = 32;
            buffer->params.frame_size = 8; // 2 channels * 4 bytes per sample
            buffer->params.buffer_size = frames;
            
            // 分配数据缓冲区
            size_t buffer_size = frames * 2 * sizeof(float);
            buffer->data = malloc(buffer_size);
            buffer->size = buffer_size;
            buffer->frames = frames;
            
            // 初始化为静音
            if (buffer->data) {
                memset(buffer->data, 0, buffer_size);
            }
        }
        return buffer;
    }
    
    /**
     * @brief 销毁音频缓冲区
     */
    void DestroyAudioBuffer(linx_audio_buffer_t* buffer) {
        if (buffer) {
            if (buffer->data) {
                free(buffer->data);
            }
            free(buffer);
        }
    }
    
    /**
     * @brief 验证插件元数据
     */
    void ValidatePluginMetadata(const linx_plugin_metadata_t* metadata) {
        ASSERT_NE(metadata, nullptr);
        ASSERT_NE(metadata->name, nullptr);
        ASSERT_GT(strlen(metadata->name), 0);
        ASSERT_NE(metadata->description, nullptr);
        ASSERT_NE(metadata->author, nullptr);
        ASSERT_GE(metadata->version.major, 0);
        ASSERT_GE(metadata->version.minor, 0);
        ASSERT_GE(metadata->version.patch, 0);
    }
};

#endif // TEST_COMMON_H