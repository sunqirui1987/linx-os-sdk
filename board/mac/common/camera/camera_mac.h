#ifndef MAC_CAMERA_H
#define MAC_CAMERA_H

#include "camera/camera_interface.h"
#include "mongoose.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mac camera implementation data structure
 * This implementation uses AVFoundation framework for camera access
 */
typedef struct {
    bool initialized;
    bool capturing;
    CameraConfig config;
    char* explain_url;
    char* explain_token;
    
    // Mac-specific camera data
    void* capture_session;      // AVCaptureSession*
    void* video_device;         // AVCaptureDevice*
    void* video_input;          // AVCaptureDeviceInput*
    void* video_output;         // AVCaptureVideoDataOutput*
    void* delegate;             // Custom delegate for frame capture
    
    // Frame buffer management
    uint8_t* current_frame_data;
    size_t current_frame_size;
    int current_frame_width;
    int current_frame_height;
    int current_frame_format;
    
    // Threading and synchronization
    void* capture_queue;        // dispatch_queue_t
    bool frame_ready;
    bool capture_in_progress;
    
    // Mirror and flip settings
    bool h_mirror_enabled;
    bool v_flip_enabled;
    
    // HTTP client using mongoose
    struct mg_mgr http_mgr;
    bool http_mgr_initialized;
} MacCameraData;

/**
 * JPEG chunk structure for streaming encoding
 */
typedef struct {
    uint8_t* data;
    size_t len;
} MacJpegChunk;

/**
 * Create Mac camera implementation
 * @return CameraInterface instance or NULL on failure
 */
CameraInterface* mac_camera_create(void);

/**
 * Set camera configuration for Mac implementation
 * @param camera_data Mac camera data structure
 * @param config Camera configuration
 * @return 0 on success, negative on error
 */
int mac_camera_set_config_internal(MacCameraData* camera_data, const CameraConfig* config);

/**
 * Capture frame from Mac camera
 * @param camera_data Mac camera data structure
 * @param frame Frame buffer to store captured image
 * @return 0 on success, negative on error
 */
int mac_camera_capture_internal(MacCameraData* camera_data, CameraFrameBuffer* frame);

/**
 * Set horizontal mirror for Mac camera
 * @param camera_data Mac camera data structure
 * @param enabled Enable/disable horizontal mirror
 * @return 0 on success, negative on error
 */
int mac_camera_set_h_mirror_internal(MacCameraData* camera_data, bool enabled);

/**
 * Set vertical flip for Mac camera
 * @param camera_data Mac camera data structure
 * @param enabled Enable/disable vertical flip
 * @return 0 on success, negative on error
 */
int mac_camera_set_v_flip_internal(MacCameraData* camera_data, bool enabled);

/**
 * Explain captured image using AI service
 * @param camera_data Mac camera data structure
 * @param question Question to ask about the image
 * @param response Buffer to store response
 * @param response_size Size of response buffer
 * @return 0 on success, negative on error
 */
int mac_camera_explain_internal(MacCameraData* camera_data, const char* question, 
                                char* response, size_t response_size);

/**
 * Initialize Mac camera hardware
 * @param camera_data Mac camera data structure
 * @return 0 on success, negative on error
 */
int mac_camera_init_hardware(MacCameraData* camera_data);

/**
 * Cleanup Mac camera resources
 * @param camera_data Mac camera data structure
 */
void mac_camera_cleanup_hardware(MacCameraData* camera_data);

/**
 * Convert captured frame to JPEG format
 * @param camera_data Mac camera data structure
 * @param raw_data Raw frame data
 * @param raw_size Size of raw data
 * @param jpeg_data Output JPEG data (caller must free)
 * @param jpeg_size Output JPEG size
 * @return 0 on success, negative on error
 */
int mac_camera_convert_to_jpeg(MacCameraData* camera_data, 
                              const uint8_t* raw_data, size_t raw_size,
                              uint8_t** jpeg_data, size_t* jpeg_size);

/**
 * Send HTTP request to explain service using captured image
 * @param camera_data Mac camera data structure
 * @param url Service URL
 * @param token Authentication token
 * @param question Question to ask about the image
 * @param jpeg_data JPEG image data
 * @param jpeg_size Size of JPEG data
 * @param response Buffer to store response
 * @param response_size Size of response buffer
 * @return 0 on success, negative on error
 */
int mac_camera_send_explain_request(MacCameraData* camera_data, const char* url, const char* token,
                                   const char* question,
                                   const uint8_t* jpeg_data, size_t jpeg_size,
                                   char* response, size_t response_size);

#ifdef __cplusplus
}
#endif

#endif // MAC_CAMERA_H