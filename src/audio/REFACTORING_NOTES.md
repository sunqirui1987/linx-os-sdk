# 音频模块重构说明

## 概述

本次重构简化了LinxOS音频模块的插件系统,移除了复杂的全局注册机制,采用更简洁的描述符模式。

## 主要改动

### 1. 移除 `builtin_plugins.h/c`

**原因:** 旧的注册系统过于复杂,包含大量不必要的功能。

**变化:**
- 移除了全局插件注册表
- 移除了 `linx_builtin_plugins_init()` 和 `linx_builtin_plugins_cleanup()`
- 移除了复杂的插件查找和统计功能

### 2. 简化 `plugin_interface.h`

**简化内容:**
- 移除了 `PLUGIN_STATE_PAUSED` 状态
- 移除了 `process_inplace`、`pause`、`resume` 等不常用的接口
- 移除了 `set_config`、`get_config`、`set_format` 等冗余接口
- 移除了 `get_latency`、`on_event` 等高级功能
- 简化了能力标志 (从10个减少到4个)

**保留核心功能:**
- 基本生命周期管理 (init, deinit, start, stop)
- 音频处理 (process)
- 参数管理 (set_parameter, get_parameter)
- 状态查询 (get_state)

### 3. 新增 `plugin_registry.h/c`

**特点:**
- 轻量级的插件注册系统
- 使用 `__attribute__((constructor))` 自动注册
- 支持按需查找插件描述符
- 无需全局初始化/清理

**使用方式:**
```c
// 获取插件描述符
const linx_plugin_descriptor_t* desc = linx_find_plugin_descriptor("Gain");

// 创建插件实例
linx_plugin_base_t* plugin = desc->create(config);
```

### 4. 改进的插件描述符

**新的描述符结构:**
```c
typedef struct {
    linx_plugin_metadata_t metadata;  // 内嵌元数据
    linx_plugin_create_func_t create;
    linx_plugin_destroy_func_t destroy;
    linx_plugin_get_metadata_func_t get_metadata;
} linx_plugin_descriptor_t;
```

**插件实现示例 (gain.c):**
```c
const linx_plugin_descriptor_t* linx_gain_plugin_get_descriptor(void) {
    static const linx_plugin_descriptor_t descriptor = {
        .metadata = {
            .name = "Gain",
            .description = "Audio gain control plugin",
            .author = "LinxOS Audio Team",
            .version = {1, 0, 0, "stable"},
            .type = LINX_AUDIO_PLUGIN_TYPE_EFFECT,
            .capabilities = PLUGIN_CAP_REALTIME | PLUGIN_CAP_INPLACE
        },
        .create = create_gain_plugin,
        .destroy = destroy_gain_plugin,
        .get_metadata = get_gain_plugin_metadata
    };
    return &descriptor;
}
```

## Bug 修复

### 1. 内存泄漏修复 (plugin_interface.c:229)
```c
// 修复前
memcpy(config->custom_data, data, size);

// 修复后
memcpy(config->custom_data, data, size);
config->custom_data_size = size;  // 添加此行
```

### 2. 返回值错误修复 (plugin_manager.c:212)
```c
// 修复前
return LINX_AUDIO_ERROR_NOT_INITIALIZED;

// 修复后
return LINX_AUDIO_SUCCESS;
```

### 3. 内存清理改进 (plugin_interface.c)
```c
void linx_plugin_config_destroy(linx_plugin_config_t* config) {
    if (!config) return;

    // 添加空指针检查
    if (config->params) {
        for (size_t i = 0; i < config->param_count; i++) {
            free((void*)config->params[i].name);
            free((void*)config->params[i].value);
        }
        free(config->params);
    }

    // 添加 custom_data 清理
    if (config->custom_data) {
        free(config->custom_data);
    }

    memset(config, 0, sizeof(linx_plugin_config_t));
}
```

## 线程安全改进

### 引用计数线程安全

**添加互斥锁保护:**
```c
struct linx_plugin_base {
    // ...
    uint32_t ref_count;
    pthread_mutex_t ref_count_mutex;  // 新增
};

uint32_t linx_plugin_base_ref(linx_plugin_base_t* plugin) {
    pthread_mutex_lock(&plugin->ref_count_mutex);
    uint32_t new_count = ++plugin->ref_count;
    pthread_mutex_unlock(&plugin->ref_count_mutex);
    return new_count;
}

uint32_t linx_plugin_base_unref(linx_plugin_base_t* plugin) {
    pthread_mutex_lock(&plugin->ref_count_mutex);
    if (plugin->ref_count == 0) {
        pthread_mutex_unlock(&plugin->ref_count_mutex);
        return 0;
    }
    uint32_t new_count = --plugin->ref_count;
    pthread_mutex_unlock(&plugin->ref_count_mutex);

    if (new_count == 0) {
        pthread_mutex_destroy(&plugin->ref_count_mutex);
    }
    return new_count;
}
```

## 测试覆盖

### 新增单元测试

1. **test_plugin_interface.cpp** - 插件接口测试
   - 配置创建/销毁
   - 参数添加/获取
   - 自定义数据管理
   - 版本比较和转换
   - 引用计数
   - 错误处理

2. **test_gain_plugin.cpp** - Gain插件测试
   - 插件创建和元数据
   - 初始化/反初始化
   - 启动/停止
   - 音频处理验证
   - 增益参数控制
   - 统计信息跟踪
   - 引用计数管理

## 迁移指南

### 旧代码迁移

**之前:**
```c
#include "builtin/builtin_plugins.h"

// 初始化
linx_builtin_plugins_init();

// 创建插件
linx_plugin_base_t* plugin = linx_create_builtin_plugin_by_name("Gain", config);

// 清理
linx_builtin_plugins_cleanup();
```

**现在:**
```c
#include "builtin/gain.h"
#include "builtin/plugin_registry.h"

// 无需初始化,插件自动注册

// 方式1: 直接创建
linx_plugin_base_t* plugin = create_gain_plugin(config);

// 方式2: 通过注册表
const linx_plugin_descriptor_t* desc = linx_find_plugin_descriptor("Gain");
if (desc) {
    linx_plugin_base_t* plugin = desc->create(config);
}

// 无需全局清理
```

### 插件开发

**实现新插件的步骤:**

1. 创建插件头文件 `my_plugin.h`
```c
#ifndef LINX_AUDIO_MY_PLUGIN_H
#define LINX_AUDIO_MY_PLUGIN_H

#include "../plugin_interface.h"

linx_plugin_base_t* create_my_plugin(const linx_plugin_config_t* config);
void destroy_my_plugin(linx_plugin_base_t* plugin);
linx_audio_result_t get_my_plugin_metadata(linx_plugin_metadata_t* metadata);
const linx_plugin_descriptor_t* linx_my_plugin_get_descriptor(void);

#endif
```

2. 实现插件 `my_plugin.c`
```c
#include "my_plugin.h"

// ... 实现create/destroy/process等函数 ...

const linx_plugin_descriptor_t* linx_my_plugin_get_descriptor(void) {
    static const linx_plugin_descriptor_t descriptor = {
        .metadata = { /* ... */ },
        .create = create_my_plugin,
        .destroy = destroy_my_plugin,
        .get_metadata = get_my_plugin_metadata
    };
    return &descriptor;
}
```

3. 在 `plugin_registry.c` 中注册
```c
static void __attribute__((constructor)) register_all_builtin_plugins(void) {
    // ...existing registrations...

    extern const linx_plugin_descriptor_t* linx_my_plugin_get_descriptor(void);
    linx_register_builtin_plugin(linx_my_plugin_get_descriptor());
}
```

## 性能影响

### 改进

- **减少内存占用**: 移除全局注册表,按需加载
- **更快的初始化**: 无需全局初始化步骤
- **线程安全**: 引用计数现在是线程安全的

### 权衡

- 失去了全局插件统计功能 (可通过其他方式实现)
- 不再支持运行时插件发现 (静态注册)

## 兼容性

### 不兼容的变更

- 移除 `linx_builtin_plugins_init/cleanup`
- 移除 `linx_create_builtin_plugin_by_name`
- 移除 `LINX_REGISTER_BUILTIN_PLUGIN` 宏
- 插件接口简化 (移除部分虚函数)

### 推荐的升级路径

1. 更新所有 `#include "builtin_plugins.h"` 为具体插件头文件
2. 移除初始化/清理调用
3. 使用新的创建方式
4. 重新编译和测试

## 未来计划

- [ ] 添加动态插件加载支持
- [ ] 实现插件热重载
- [ ] 添加插件依赖管理
- [ ] 扩展插件能力标志
- [ ] 添加插件性能分析工具

## 参考

- `src/audio/plugins/plugin_interface.h` - 核心插件接口
- `src/audio/plugins/builtin/plugin_registry.h` - 插件注册表
- `src/audio/plugins/builtin/gain.c` - 参考实现
- `src/audio/plugins/test/` - 单元测试示例
