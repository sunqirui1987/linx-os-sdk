# Mac版本唤醒词检测 - Porcupine实现

基于Picovoice Porcupine库的Mac平台唤醒词检测实现。

## 特性

- 🎯 高精度唤醒词检测
- 🚀 轻量级，适合实时应用
- 🔧 支持自定义唤醒词
- 🌍 多语言支持
- 📱 跨平台兼容
- 🔒 本地处理，保护隐私

## 依赖要求

### 系统要求
- macOS 10.14 或更高版本
- CMake 3.10+
- C99兼容编译器

### 第三方库
- Picovoice Porcupine库

## 安装

### 1. 安装Porcupine库

使用Homebrew安装（推荐）：
```bash
# 添加Picovoice tap
brew tap picovoice/picovoice

# 安装Porcupine
brew install porcupine
```

或者手动安装：
```bash
# 下载并安装Porcupine C库
git clone https://github.com/Picovoice/porcupine.git
cd porcupine
# 按照官方文档进行编译和安装
```

### 2. 获取访问密钥

1. 访问 [Picovoice Console](https://console.picovoice.ai/)
2. 注册或登录账户
3. 获取免费的AccessKey

### 3. 编译项目

```bash
cd /path/to/linx-os-sdk/board/mac/common/audio/wake_words
mkdir build
cd build
cmake ..
make
```

## 使用方法

### 基本使用

```c
#include "wake_word_porcupine.h"

// 1. 创建配置
porcupine_config_t config;
porcupine_config_set_default(&config, "YOUR_ACCESS_KEY");

// 2. 添加关键词
porcupine_config_add_keyword(&config, "/path/to/keyword.ppn", 0.5f);

// 3. 创建唤醒词接口
WakeWordInterface* wake_word = wake_word_porcupine_create(&config);

// 4. 初始化
wake_word_interface_initialize(wake_word, NULL, NULL);

// 5. 设置回调
wake_word_interface_set_callback(wake_word, callback_function, user_data);

// 6. 启动检测
wake_word_interface_start(wake_word);

// 7. 输入音频数据
wake_word_interface_feed(wake_word, audio_data, frame_size);

// 8. 清理资源
wake_word_interface_destroy(wake_word);
porcupine_config_destroy(&config);
```

### 运行示例

```bash
# 使用默认关键词
./wake_word_example YOUR_ACCESS_KEY

# 使用自定义关键词
./wake_word_example YOUR_ACCESS_KEY porcupine picovoice bumblebee
```

## 支持的关键词

### 内置关键词
- porcupine
- picovoice
- bumblebee
- alexa
- computer
- hey google
- hey siri
- jarvis
- smart mirror
- snowboy
- terminator
- view glass

### 自定义关键词

1. 访问 [Picovoice Console](https://console.picovoice.ai/)
2. 创建自定义关键词模型
3. 下载 `.ppn` 文件
4. 在代码中使用文件路径

## API参考

### 配置函数

```c
// 设置默认配置
int porcupine_config_set_default(porcupine_config_t* config, const char* access_key);

// 添加关键词
int porcupine_config_add_keyword(porcupine_config_t* config, const char* keyword_path, float sensitivity);

// 销毁配置
void porcupine_config_destroy(porcupine_config_t* config);
```

### 接口函数

```c
// 创建接口
WakeWordInterface* wake_word_porcupine_create(const porcupine_config_t* config);

// 初始化
int wake_word_interface_initialize(WakeWordInterface* self, audio_codec_t* codec, void* user_data);

// 设置回调
void wake_word_interface_set_callback(WakeWordInterface* self, wake_word_callback_t callback, void* user_data);

// 启动/停止
void wake_word_interface_start(WakeWordInterface* self);
void wake_word_interface_stop(WakeWordInterface* self);

// 输入音频
void wake_word_interface_feed(WakeWordInterface* self, const int16_t* data, size_t size);

// 销毁
void wake_word_interface_destroy(WakeWordInterface* self);
```

## 音频格式要求

- 采样率：16 kHz
- 位深度：16位
- 声道：单声道
- 格式：PCM

## 性能优化

### 敏感度调整
- 范围：0.0 - 1.0
- 较高值：减少漏检，增加误检
- 较低值：减少误检，增加漏检
- 推荐值：0.5

### 内存使用
- 基础内存：~1.5MB
- 每个关键词：~50KB
- 音频缓冲：可配置

## 故障排除

### 常见问题

1. **编译错误：找不到pv_porcupine.h**
   ```bash
   # 确保Porcupine已正确安装
   brew install porcupine
   # 或检查include路径
   ```

2. **运行时错误：Invalid access key**
   ```bash
   # 检查访问密钥是否正确
   # 确保网络连接正常
   ```

3. **关键词文件不存在**
   ```bash
   # 检查.ppn文件路径
   # 确保文件权限正确
   ```

### 调试模式

编译时启用调试：
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 许可证

本项目遵循Apache 2.0许可证。

Porcupine库有自己的许可证条款，请参考：
- [Porcupine许可证](https://github.com/Picovoice/porcupine/blob/master/LICENSE)

## 贡献

欢迎提交Issue和Pull Request！

## 相关链接

- [Picovoice官网](https://picovoice.ai/)
- [Porcupine文档](https://picovoice.ai/docs/porcupine/)
- [Porcupine GitHub](https://github.com/Picovoice/porcupine)
- [获取AccessKey](https://console.picovoice.ai/)