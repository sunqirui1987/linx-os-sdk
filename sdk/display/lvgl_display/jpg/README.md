# JPEG Encoder Library

This is a C99-compatible JPEG encoding library converted from the original C++ implementation.

## Features

- Efficient JPEG encoding for various image formats
- Support for multiple pixel formats (RGB888, RGB565, YUV422, Grayscale)
- Configurable quality settings
- Memory and callback-based output options
- C99 standard compliance

## Supported Pixel Formats

- `PIXFORMAT_GRAYSCALE` - 8-bit grayscale
- `PIXFORMAT_RGB888` - 24-bit RGB
- `PIXFORMAT_RGB565` - 16-bit RGB
- `PIXFORMAT_YUV422` - YUV 4:2:2 format

## Usage

### Basic Usage (Memory Output)

```c
#include "image_to_jpeg.h"

uint8_t jpeg_buffer[32768];
size_t jpeg_size;
uint8_t* image_data = ...; // Your image data

bool success = image_to_jpeg(
    jpeg_buffer, sizeof(jpeg_buffer), &jpeg_size,
    width, height, PIXFORMAT_RGB888, 85, image_data
);

if (success) {
    // Use jpeg_buffer with jpeg_size bytes
}
```

### Callback-based Output

```c
bool jpeg_callback(void* user_data, const void* data, int len) {
    // Handle JPEG data chunk
    return true; // Return false on error
}

bool success = image_to_jpeg_cb(
    width, height, PIXFORMAT_RGB888, 85,
    image_data, jpeg_callback, user_data
);
```

## Quality Settings

Quality ranges from 1 (lowest) to 100 (highest). Recommended values:
- 50-70: Good balance of quality and size
- 80-95: High quality
- 95-100: Maximum quality (large file size)

## Memory Requirements

The encoder allocates memory dynamically based on image dimensions. Ensure sufficient heap memory is available for:
- Internal buffers
- MCU line buffers
- Quantization tables

## Integration

This library uses the LinX OS SDK logging system (`linx_log.h`) for error reporting and debugging information.