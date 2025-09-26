#ifndef CAMERA_STUB_H
#define CAMERA_STUB_H

#include "camera_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stub camera implementation data structure
 * This is a placeholder implementation for platforms without camera support
 */
typedef struct {
    bool initialized;
    bool capturing;
    CameraConfig config;
    char* explain_url;
    char* explain_token;
    uint8_t* dummy_frame_data;  // Dummy frame data for testing
    size_t dummy_frame_size;
} CameraStubData;

/**
 * Create stub camera implementation
 * @return CameraInterface instance or NULL on failure
 */
CameraInterface* camera_stub_create(void);

#ifdef __cplusplus
}
#endif

#endif // CAMERA_STUB_H