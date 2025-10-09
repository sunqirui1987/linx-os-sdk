# ESP32 Board Implementation

This directory contains the ESP32 board implementation for the LinX OS SDK, based on the C interface defined in `src/board/`.

## Overview

The ESP32 board implementation provides a concrete implementation of the board interface for ESP32-based devices. It includes support for:

- WiFi connectivity and management
- Display interface integration
- Camera interface integration
- Audio codec support (placeholder)
- LED control (placeholder)
- Backlight control (placeholder)
- Power management
- System information reporting

## Architecture

The implementation follows the board interface pattern defined in `src/board/board.h`:

```
ESP32Board (esp32_board.h/c)
    ↓ inherits from
Board (src/board/board.h/c)
    ↓ uses
BoardVTable (function pointers for virtual methods)
```

## Files

- `esp32_board.h` - Header file defining the ESP32Board structure and functions
- `esp32_board.c` - Implementation of the ESP32Board functionality
- `test_esp32_board.c` - Test suite for the ESP32Board implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - This documentation

## Key Features

### WiFi Management
- WiFi connection and configuration
- WiFi configuration mode (AP mode for setup)
- Signal strength monitoring
- Network status reporting

### Hardware Interfaces
- Display interface using the display module from `src/display/`
- Camera interface using the camera module from `src/camera/`
- Placeholder implementations for audio, LED, and backlight

### JSON Reporting
- Board information JSON with WiFi details
- Device status JSON with network, screen, and chip information
- System information JSON (uses default implementation)

### Power Management
- Power save mode control
- Temperature monitoring
- Battery level reporting (returns false for dev boards)

## Building

To build the ESP32 board implementation:

```bash
cd board/esp32
mkdir build && cd build
cmake ..
make
```

## Testing

To run the test suite:

```bash
cd board/esp32/build
./esp32_board_test
```

The test suite covers:
- Board creation and destruction
- WiFi functionality
- Hardware interface availability
- Power management features
- JSON output generation
- Network operations

## Integration

To use this ESP32 board implementation in your project:

1. Include the header: `#include "esp32_board.h"`
2. Create an instance: `ESP32Board* board = esp32_board_create();`
3. Use the board interface: `board_get_display((Board*)board);`
4. Clean up: `esp32_board_destroy(board);`

Or use the singleton pattern:
```c
Board* board = board_get_instance(); // Uses create_board() function
```

## TODO

The following features are currently placeholder implementations and need to be completed:

- [ ] Actual WiFi hardware initialization and management
- [ ] Real audio codec integration
- [ ] LED control implementation
- [ ] Backlight control implementation
- [ ] ESP32-specific temperature sensor reading
- [ ] Power save mode hardware implementation
- [ ] Memory management improvements for JSON strings
- [ ] Error handling enhancements

## Dependencies

This implementation depends on:
- `src/board/` - Base board interface
- `src/common/` - Common utilities (settings, system_info, logging, cJSON)
- `src/display/` - Display interface
- `src/camera/` - Camera interface

## Compatibility

This implementation is designed to work with:
- ESP32 development boards
- ESP32-S3 boards
- Other ESP32 variants (with minor modifications)

The code is written in C99 and should be compatible with the ESP-IDF framework.