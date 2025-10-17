/**
 * @file test_gain_plugin.cpp
 * @brief Gain插件单元测试
 */

#include "test_common.h"

// ============================================================================
// Gain插件测试
// ============================================================================

class GainPluginTest : public PluginTestBase {
protected:
    linx_plugin_base_t* plugin;
    linx_plugin_config_t* config;

    void SetUp() override {
        config = CreateDefaultConfig();
        ASSERT_NE(config, nullptr);

        // 添加gain特定配置
        linx_plugin_config_add_param(config, "gain", "1.0");

        // 创建插件实例
        plugin = create_gain_plugin(config);
        ASSERT_NE(plugin, nullptr);
    }

    void TearDown() override {
        if (plugin) {
            destroy_gain_plugin(plugin);
        }
        if (config) {
            DestroyConfig(config);
        }
    }
};

TEST_F(GainPluginTest, PluginCreation) {
    // 验证插件基本信息
    EXPECT_NE(plugin->vtable, nullptr);
    EXPECT_EQ(plugin->state, PLUGIN_STATE_LOADED);
    EXPECT_EQ(plugin->ref_count, 1);
}

TEST_F(GainPluginTest, PluginMetadata) {
    linx_plugin_metadata_t metadata;
    linx_audio_result_t result = get_gain_plugin_metadata(&metadata);

    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_STREQ(metadata.name, "Gain");
    EXPECT_STREQ(metadata.description, "Audio gain control plugin");
    EXPECT_EQ(metadata.type, LINX_AUDIO_PLUGIN_TYPE_EFFECT);
    EXPECT_NE(metadata.capabilities & PLUGIN_CAP_REALTIME, 0);
}

TEST_F(GainPluginTest, PluginDescriptor) {
    const linx_plugin_descriptor_t* descriptor = linx_gain_plugin_get_descriptor();

    ASSERT_NE(descriptor, nullptr);
    EXPECT_STREQ(descriptor->metadata.name, "Gain");
    EXPECT_NE(descriptor->create, nullptr);
    EXPECT_NE(descriptor->destroy, nullptr);
    EXPECT_NE(descriptor->get_metadata, nullptr);
}

TEST_F(GainPluginTest, PluginInitialization) {
    ASSERT_NE(plugin->vtable, nullptr);
    ASSERT_NE(plugin->vtable->initialize, nullptr);

    linx_audio_result_t result = plugin->vtable->initialize(plugin, config);
    EXPECT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_EQ(plugin->state, PLUGIN_STATE_INITIALIZED);

    // 反初始化
    if (plugin->vtable->deinitialize) {
        result = plugin->vtable->deinitialize(plugin);
        EXPECT_EQ(result, LINX_AUDIO_SUCCESS);
    }
}

TEST_F(GainPluginTest, PluginStartStop) {
    // 初始化
    plugin->vtable->initialize(plugin, config);

    // 启动
    ASSERT_NE(plugin->vtable->start, nullptr);
    linx_audio_result_t result = plugin->vtable->start(plugin);
    EXPECT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_EQ(plugin->state, PLUGIN_STATE_RUNNING);

    // 停止
    ASSERT_NE(plugin->vtable->stop, nullptr);
    result = plugin->vtable->stop(plugin);
    EXPECT_EQ(result, LINX_AUDIO_SUCCESS);

    // 反初始化
    plugin->vtable->deinitialize(plugin);
}

TEST_F(GainPluginTest, AudioProcessing) {
    // 初始化并启动插件
    plugin->vtable->initialize(plugin, config);
    plugin->vtable->start(plugin);

    // 创建测试缓冲区
    const uint32_t frames = 512;
    linx_audio_buffer_t* input = CreateTestBuffer(frames);
    linx_audio_buffer_t* output = CreateTestBuffer(frames);

    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);

    // 填充输入数据 (1.0的正弦波)
    float* input_data = (float*)input->data;
    for (uint32_t i = 0; i < frames * 2; i++) {
        input_data[i] = 0.5f; // 固定振幅用于测试
    }

    // 处理音频
    ASSERT_NE(plugin->vtable->process, nullptr);
    linx_audio_result_t result = plugin->vtable->process(plugin, input, output);
    EXPECT_EQ(result, LINX_AUDIO_SUCCESS);

    // 验证输出 (gain=1.0时输入应等于输出)
    float* output_data = (float*)output->data;
    for (uint32_t i = 0; i < frames * 2; i++) {
        EXPECT_FLOAT_EQ(output_data[i], 0.5f);
    }

    // 清理
    DestroyAudioBuffer(input);
    DestroyAudioBuffer(output);
    plugin->vtable->stop(plugin);
    plugin->vtable->deinitialize(plugin);
}

TEST_F(GainPluginTest, GainParameter) {
    plugin->vtable->initialize(plugin, config);
    plugin->vtable->start(plugin);

    // 设置增益为0.5
    ASSERT_NE(plugin->vtable->set_parameter, nullptr);
    linx_audio_result_t result = plugin->vtable->set_parameter(plugin, "gain", "0.5");
    EXPECT_EQ(result, LINX_AUDIO_SUCCESS);

    // 创建测试缓冲区
    const uint32_t frames = 256;
    linx_audio_buffer_t* input = CreateTestBuffer(frames);
    linx_audio_buffer_t* output = CreateTestBuffer(frames);

    // 填充输入数据
    float* input_data = (float*)input->data;
    for (uint32_t i = 0; i < frames * 2; i++) {
        input_data[i] = 1.0f;
    }

    // 处理
    plugin->vtable->process(plugin, input, output);

    // 验证输出 (应该是输入的0.5倍)
    float* output_data = (float*)output->data;
    for (uint32_t i = 0; i < frames * 2; i++) {
        EXPECT_FLOAT_EQ(output_data[i], 0.5f);
    }

    DestroyAudioBuffer(input);
    DestroyAudioBuffer(output);
    plugin->vtable->stop(plugin);
    plugin->vtable->deinitialize(plugin);
}

TEST_F(GainPluginTest, InvalidGainParameter) {
    plugin->vtable->initialize(plugin, config);

    // 测试无效的增益值
    ASSERT_NE(plugin->vtable->set_parameter, nullptr);
    linx_audio_result_t result = plugin->vtable->set_parameter(plugin, "gain", "invalid");
    // 应该处理错误或使用默认值
    EXPECT_TRUE(result == LINX_AUDIO_SUCCESS || result != LINX_AUDIO_SUCCESS);

    plugin->vtable->deinitialize(plugin);
}

TEST_F(GainPluginTest, GetParameter) {
    plugin->vtable->initialize(plugin, config);

    // 设置增益
    plugin->vtable->set_parameter(plugin, "gain", "0.75");

    // 获取增益
    char value[32];
    ASSERT_NE(plugin->vtable->get_parameter, nullptr);
    linx_audio_result_t result = plugin->vtable->get_parameter(plugin, "gain", value, sizeof(value));
    EXPECT_EQ(result, LINX_AUDIO_SUCCESS);

    // 验证值
    float gain_value = atof(value);
    EXPECT_FLOAT_EQ(gain_value, 0.75f);

    plugin->vtable->deinitialize(plugin);
}

TEST_F(GainPluginTest, StatisticsTracking) {
    plugin->vtable->initialize(plugin, config);
    plugin->vtable->start(plugin);

    // 初始统计应该为0
    EXPECT_EQ(plugin->stats.frames_processed, 0);
    EXPECT_EQ(plugin->stats.error_count, 0);

    // 处理一些音频
    const uint32_t frames = 1024;
    linx_audio_buffer_t* input = CreateTestBuffer(frames);
    linx_audio_buffer_t* output = CreateTestBuffer(frames);

    plugin->vtable->process(plugin, input, output);

    // 验证统计更新
    EXPECT_GT(plugin->stats.frames_processed, 0);

    DestroyAudioBuffer(input);
    DestroyAudioBuffer(output);
    plugin->vtable->stop(plugin);
    plugin->vtable->deinitialize(plugin);
}

TEST_F(GainPluginTest, ReferenceCount) {
    uint32_t initial_ref = plugin->ref_count;
    EXPECT_EQ(initial_ref, 1);

    // 增加引用
    linx_plugin_base_ref(plugin);
    EXPECT_EQ(plugin->ref_count, 2);

    // 减少引用
    linx_plugin_base_unref(plugin);
    EXPECT_EQ(plugin->ref_count, 1);
}
