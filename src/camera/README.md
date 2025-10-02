# Camera Module

相机模块提供了统一的相机接口，支持图像捕获、镜像翻转控制以及AI图像解释功能。

## 功能特性

- **图像捕获**: 支持从相机设备捕获图像帧
- **镜像控制**: 支持水平镜像和垂直翻转设置
- **AI解释**: 集成AI服务，可对捕获的图像进行智能分析和问答
- **多平台支持**: 通过虚函数表实现跨平台兼容
- **存根实现**: 提供测试用的存根实现

## 架构设计

相机模块采用面向对象的C语言设计，通过虚函数表（vtable）实现多态：

```
CameraInterface (基础接口)
├── camera_interface.h/c (通用接口实现)
└── camera_stub.h/c (存根实现)
```

## 编译与运行

### 编译

```bash
cd test
cmake -S . -B build
cmake --build build
./build/camera_test
```

## API 接口

### 核心结构体

```c
typedef struct {
    uint8_t* data;      // 图像数据缓冲区
    size_t size;        // 数据大小（字节）
    int width;          // 图像宽度（像素）
    int height;         // 图像高度（像素）
    int format;         // 图像格式（JPEG, RGB等）
} CameraFrameBuffer;

typedef struct {
    int width;          // 图像宽度
    int height;         // 图像高度
    int quality;        // JPEG质量 (1-100)
    int format;         // 图像格式
    bool h_mirror;      // 水平镜像
    bool v_flip;        // 垂直翻转
} CameraConfig;
```

### 主要函数

```c
// 初始化相机接口
int camera_interface_init(CameraInterface* self);

// 设置相机配置
int camera_interface_set_config(CameraInterface* self, const CameraConfig* config);

// 捕获图像帧
int camera_interface_capture(CameraInterface* self, CameraFrameBuffer* frame);

// 设置水平镜像
int camera_interface_set_h_mirror(CameraInterface* self, bool enabled);

// 设置垂直翻转
int camera_interface_set_v_flip(CameraInterface* self, bool enabled);

// 设置AI解释服务URL和令牌
int camera_interface_set_explain_url(CameraInterface* self, const char* url, const char* token);

// AI图像解释
int camera_interface_explain(CameraInterface* self, const char* question, char* response, size_t response_size);

// 释放图像帧
int camera_interface_release_frame(CameraInterface* self, CameraFrameBuffer* frame);

// 销毁相机接口
int camera_interface_destroy(CameraInterface* self);
```

## 使用示例

```c
#include "camera_stub.h"

int main() {
    // 创建相机实例
    CameraInterface* camera = camera_stub_create();
    if (!camera) {
        return -1;
    }
    
    // 初始化相机
    if (camera_interface_init(camera) != 0) {
        camera_interface_destroy(camera);
        return -1;
    }
    
    // 设置配置
    CameraConfig config = {
        .width = 1280,
        .height = 720,
        .quality = 90,
        .format = 1, // JPEG
        .h_mirror = false,
        .v_flip = false
    };
    camera_interface_set_config(camera, &config);
    
    // 设置AI解释服务
    camera_interface_set_explain_url(camera, "https://api.example.com/explain", "your_token");
    
    // 捕获图像
    CameraFrameBuffer frame;
    if (camera_interface_capture(camera, &frame) == 0) {
        printf("Captured frame: %dx%d, size=%zu\\n", frame.width, frame.height, frame.size);
        
        // AI解释
        char response[1024];
        if (camera_interface_explain(camera, "What do you see in this image?", response, sizeof(response)) == 0) {
            printf("AI Response: %s\\n", response);
        }
        
        // 释放帧
        camera_interface_release_frame(camera, &frame);
    }
    
    // 清理资源
    camera_interface_destroy(camera);
    return 0;
}
```

## 平台实现

### 存根实现 (Stub)

存根实现提供了一个测试用的相机接口，不依赖实际硬件：

- 生成虚拟图像数据用于测试
- 模拟所有相机操作
- 提供虚拟AI解释响应

### 扩展新平台

要为新平台添加相机支持，需要：

1. 创建平台特定的实现文件（如 `camera_esp32.c`）
2. 实现 `CameraInterfaceVTable` 中的所有函数
3. 提供创建函数（如 `camera_esp32_create()`）
4. 在 CMakeLists.txt 中添加平台特定的源文件

## 依赖项

- `linx_log`: 日志系统
- 标准C库: `stdlib.h`, `string.h`, `stdint.h`, `stdbool.h`

## 注意事项

1. 所有函数返回0表示成功，负值表示错误
2. 调用者负责检查返回值并处理错误
3. 图像帧使用完毕后必须调用 `camera_interface_release_frame()` 释放资源
4. AI解释功能需要网络连接和有效的服务URL/令牌
5. 存根实现仅用于测试，不提供真实的相机功能