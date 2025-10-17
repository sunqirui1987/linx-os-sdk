/**
 * @file test_main.cpp
 * @brief GoogleTest主文件 - LinxOS Audio Plugins
 * @details 包含测试环境设置和通用测试基类
 */

#include "test_common.h"

/**
 * @brief 插件测试环境
 * @details 负责全局初始化和清理
 */
class PluginTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // 插件系统不需要全局初始化
        // 每个插件通过描述符独立管理
    }

    void TearDown() override {
        // 插件系统不需要全局清理
    }
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // 添加全局测试环境
    ::testing::AddGlobalTestEnvironment(new PluginTestEnvironment);
    
    return RUN_ALL_TESTS();
}