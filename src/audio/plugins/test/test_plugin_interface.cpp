/**
 * @file test_plugin_interface.cpp
 * @brief 插件接口单元测试
 */

#include "test_common.h"

// ============================================================================
// 插件配置测试
// ============================================================================

TEST_F(PluginTestBase, ConfigCreateDestroy) {
    linx_plugin_config_t config;

    // 测试创建
    linx_audio_result_t result = linx_plugin_config_create(&config);
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_EQ(config.param_count, 0);
    EXPECT_EQ(config.params, nullptr);
    EXPECT_EQ(config.custom_data, nullptr);
    EXPECT_EQ(config.custom_data_size, 0);

    // 测试销毁
    linx_plugin_config_destroy(&config);
}

TEST_F(PluginTestBase, ConfigAddGetParam) {
    linx_plugin_config_t config;
    linx_plugin_config_create(&config);

    // 添加参数
    linx_audio_result_t result = linx_plugin_config_add_param(&config, "gain", "0.5");
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_EQ(config.param_count, 1);

    // 获取参数
    const char* value = nullptr;
    result = linx_plugin_config_get_param(&config, "gain", &value);
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    ASSERT_NE(value, nullptr);
    EXPECT_STREQ(value, "0.5");

    // 获取不存在的参数
    result = linx_plugin_config_get_param(&config, "nonexistent", &value);
    EXPECT_EQ(result, LINX_AUDIO_ERROR_NOT_FOUND);

    linx_plugin_config_destroy(&config);
}

TEST_F(PluginTestBase, ConfigCustomData) {
    linx_plugin_config_t config;
    linx_plugin_config_create(&config);

    // 测试数据
    uint32_t test_data[] = {1, 2, 3, 4, 5};
    size_t test_size = sizeof(test_data);

    // 设置自定义数据
    linx_audio_result_t result = linx_plugin_config_set_custom_data(&config, test_data, test_size);
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_EQ(config.custom_data_size, test_size);
    ASSERT_NE(config.custom_data, nullptr);

    // 验证数据
    uint32_t* stored_data = (uint32_t*)config.custom_data;
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(stored_data[i], test_data[i]);
    }

    linx_plugin_config_destroy(&config);
}

// ============================================================================
// 版本测试
// ============================================================================

TEST_F(PluginTestBase, VersionCompare) {
    linx_plugin_version_t v1 = {1, 0, 0, "stable"};
    linx_plugin_version_t v2 = {1, 0, 1, "stable"};
    linx_plugin_version_t v3 = {1, 0, 0, "stable"};

    EXPECT_LT(linx_plugin_version_compare(&v1, &v2), 0);
    EXPECT_GT(linx_plugin_version_compare(&v2, &v1), 0);
    EXPECT_EQ(linx_plugin_version_compare(&v1, &v3), 0);
}

TEST_F(PluginTestBase, VersionToString) {
    linx_plugin_version_t version = {1, 2, 3, "beta"};
    char buffer[64];

    linx_audio_result_t result = linx_plugin_version_to_string(&version, buffer, sizeof(buffer));
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_STREQ(buffer, "1.2.3-beta");

    // 测试没有build信息的版本
    version.build = nullptr;
    result = linx_plugin_version_to_string(&version, buffer, sizeof(buffer));
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_STREQ(buffer, "1.2.3");
}

TEST_F(PluginTestBase, VersionFromString) {
    linx_plugin_version_t version;

    // 测试完整版本字符串
    linx_audio_result_t result = linx_plugin_version_from_string("2.1.0-rc1", &version);
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_EQ(version.major, 2);
    EXPECT_EQ(version.minor, 1);
    EXPECT_EQ(version.patch, 0);
    ASSERT_NE(version.build, nullptr);
    EXPECT_STREQ(version.build, "rc1");

    free((void*)version.build);

    // 测试简单版本字符串
    result = linx_plugin_version_from_string("3.0.1", &version);
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_EQ(version.major, 3);
    EXPECT_EQ(version.minor, 0);
    EXPECT_EQ(version.patch, 1);
}

// ============================================================================
// 插件基础测试
// ============================================================================

TEST_F(PluginTestBase, PluginBaseInit) {
    linx_plugin_base_t plugin;

    linx_plugin_metadata_t metadata = {
        .name = "TestPlugin",
        .description = "Test Plugin Description",
        .author = "Test Author",
        .version = {1, 0, 0, "stable"},
        .type = LINX_AUDIO_PLUGIN_TYPE_EFFECT,
        .capabilities = PLUGIN_CAP_REALTIME
    };

    linx_audio_result_t result = linx_plugin_base_init(&plugin, nullptr, &metadata);
    ASSERT_EQ(result, LINX_AUDIO_SUCCESS);
    EXPECT_EQ(plugin.state, PLUGIN_STATE_LOADED);
    EXPECT_EQ(plugin.ref_count, 1);

    // 清理
    pthread_mutex_destroy(&plugin.ref_count_mutex);
}

TEST_F(PluginTestBase, PluginRefCounting) {
    linx_plugin_base_t plugin;

    linx_plugin_metadata_t metadata = {
        .name = "TestPlugin",
        .description = "Test",
        .author = "Test",
        .version = {1, 0, 0, nullptr},
        .type = LINX_AUDIO_PLUGIN_TYPE_EFFECT,
        .capabilities = 0
    };

    linx_plugin_base_init(&plugin, nullptr, &metadata);

    // 测试增加引用
    uint32_t ref = linx_plugin_base_ref(&plugin);
    EXPECT_EQ(ref, 2);
    EXPECT_EQ(plugin.ref_count, 2);

    // 测试减少引用
    ref = linx_plugin_base_unref(&plugin);
    EXPECT_EQ(ref, 1);
    EXPECT_EQ(plugin.ref_count, 1);

    // 测试引用归零
    ref = linx_plugin_base_unref(&plugin);
    EXPECT_EQ(ref, 0);

    // 注意:互斥锁已在unref中销毁,不需要再次销毁
}

// ============================================================================
// 错误处理测试
// ============================================================================

TEST_F(PluginTestBase, NullParameterHandling) {
    // 测试空指针处理
    EXPECT_EQ(linx_plugin_config_create(nullptr), LINX_AUDIO_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(linx_plugin_base_ref(nullptr), 0);
    EXPECT_EQ(linx_plugin_base_unref(nullptr), 0);

    linx_plugin_config_t config;
    linx_plugin_config_create(&config);
    EXPECT_EQ(linx_plugin_config_add_param(&config, nullptr, "value"), LINX_AUDIO_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(linx_plugin_config_add_param(&config, "name", nullptr), LINX_AUDIO_ERROR_INVALID_PARAMETER);
    linx_plugin_config_destroy(&config);
}

TEST_F(PluginTestBase, BufferOverflow) {
    linx_plugin_version_t version = {1, 2, 3, "very-long-build-string"};
    char small_buffer[5];

    linx_audio_result_t result = linx_plugin_version_to_string(&version, small_buffer, sizeof(small_buffer));
    EXPECT_EQ(result, LINX_AUDIO_ERROR_BUFFER_OVERFLOW);
}
