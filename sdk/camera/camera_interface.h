#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Camera interface structure for C99 compatibility
 */
typedef struct CameraInterface CameraInterface;

/**
 * Camera frame buffer structure
 */
typedef struct {
    uint8_t* data;      // Image data buffer
    size_t size;        // Size of the data in bytes
    int width;          // Image width in pixels
    int height;         // Image height in pixels
    int format;         // Image format (JPEG, RGB, etc.)
} CameraFrameBuffer;

/**
 * Camera configuration structure
 */
typedef struct {
    int width;          // Image width
    int height;         // Image height
    int quality;        // JPEG quality (1-100)
    int format;         // Image format
    bool h_mirror;      // Horizontal mirror
    bool v_flip;        // Vertical flip
} CameraConfig;

/**
 * Camera interface function pointers
 */
typedef struct {
    int (*init)(CameraInterface* self);
    int (*set_config)(CameraInterface* self, const CameraConfig* config);
    int (*capture)(CameraInterface* self, CameraFrameBuffer* frame);
    int (*set_h_mirror)(CameraInterface* self, bool enabled);
    int (*set_v_flip)(CameraInterface* self, bool enabled);
    int (*set_explain_url)(CameraInterface* self, const char* url, const char* token);
    int (*explain)(CameraInterface* self, const char* question, char* response, size_t response_size);
    int (*release_frame)(CameraInterface* self, CameraFrameBuffer* frame);
    int (*destroy)(CameraInterface* self);
} CameraInterfaceVTable;

/**
 * Base camera interface structure
 */
struct CameraInterface {
    const CameraInterfaceVTable* vtable;
    void* impl_data;  // Implementation-specific data
    
    // Configuration
    CameraConfig config;
    
    // Explain service configuration
    char* explain_url;
    char* explain_token;
    
    // State
    bool is_initialized;
    bool is_capturing;
};

/**
 * Initialize camera interface
 * @param self Camera interface instance
 * @return 0 on success, negative on error
 */
int camera_interface_init(CameraInterface* self);

/**
 * Set camera configuration
 * @param self Camera interface instance
 * @param config Camera configuration
 * @return 0 on success, negative on error
 */
int camera_interface_set_config(CameraInterface* self, const CameraConfig* config);

/**
 * Capture a frame from camera
 * @param self Camera interface instance
 * @param frame Frame buffer to store captured image
 * @return 0 on success, negative on error
 */
int camera_interface_capture(CameraInterface* self, CameraFrameBuffer* frame);

/**
 * Set horizontal mirror
 * @param self Camera interface instance
 * @param enabled Enable/disable horizontal mirror
 * @return 0 on success, negative on error
 */
int camera_interface_set_h_mirror(CameraInterface* self, bool enabled);

/**
 * Set vertical flip
 * @param self Camera interface instance
 * @param enabled Enable/disable vertical flip
 * @return 0 on success, negative on error
 */
int camera_interface_set_v_flip(CameraInterface* self, bool enabled);

/**
 * Set explain service URL and token
 * @param self Camera interface instance
 * @param url Explain service URL
 * @param token Authentication token
 * @return 0 on success, negative on error
 */
int camera_interface_set_explain_url(CameraInterface* self, const char* url, const char* token);

/**
 * Explain captured image with a question
 * @param self Camera interface instance
 * @param question Question to ask about the image
 * @param response Buffer to store the response
 * @param response_size Size of the response buffer
 * @return 0 on success, negative on error
 */
int camera_interface_explain(CameraInterface* self, const char* question, char* response, size_t response_size);

/**
 * Release frame buffer
 * @param self Camera interface instance
 * @param frame Frame buffer to release
 * @return 0 on success, negative on error
 */
int camera_interface_release_frame(CameraInterface* self, CameraFrameBuffer* frame);

/**
 * Destroy camera interface
 * @param self Camera interface instance
 * @return 0 on success, negative on error
 */
int camera_interface_destroy(CameraInterface* self);

#ifdef __cplusplus
}
#endif

#endif // CAMERA_INTERFACE_H