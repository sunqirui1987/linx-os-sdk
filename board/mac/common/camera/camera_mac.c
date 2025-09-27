#include "camera_mac.h"
#include "log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

// Mac-specific includes - using C-compatible headers only
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
// Note: AVFoundation and Foundation are Objective-C frameworks
// For C compatibility, we'll use stub implementations
#endif

#define TAG "MacCamera"
#define MAX_FRAME_SIZE (1920 * 1080 * 4)  // Max frame size for 1080p RGBA
#define DEFAULT_JPEG_QUALITY 0.8f
#define HTTP_TIMEOUT_MS 10000  // 10 seconds timeout

// HTTP request context structure
typedef struct {
    char* response_buffer;
    size_t response_size;
    size_t response_len;
    bool request_done;
    bool request_success;
    int status_code;
} HttpRequestContext;

// Forward declarations for vtable functions
static int mac_camera_init(CameraInterface* self);
static int mac_camera_set_config(CameraInterface* self, const CameraConfig* config);
static int mac_camera_capture(CameraInterface* self, CameraFrameBuffer* frame);
static int mac_camera_set_h_mirror(CameraInterface* self, bool enabled);
static int mac_camera_set_v_flip(CameraInterface* self, bool enabled);
static int mac_camera_set_explain_url(CameraInterface* self, const char* url, const char* token);
static int mac_camera_explain(CameraInterface* self, const char* question, char* response, size_t response_size);
static int mac_camera_release_frame(CameraInterface* self, CameraFrameBuffer* frame);
static int mac_camera_destroy(CameraInterface* self);

// Mac camera vtable
static const CameraInterfaceVTable mac_camera_vtable = {
    .init = mac_camera_init,
    .set_config = mac_camera_set_config,
    .capture = mac_camera_capture,
    .set_h_mirror = mac_camera_set_h_mirror,
    .set_v_flip = mac_camera_set_v_flip,
    .set_explain_url = mac_camera_set_explain_url,
    .explain = mac_camera_explain,
    .release_frame = mac_camera_release_frame,
    .destroy = mac_camera_destroy
};

// Helper function to get current timestamp in milliseconds
static long long get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

// HTTP event handler for mongoose
static void http_event_handler(struct mg_connection *c, int ev, void *ev_data) {
    HttpRequestContext *ctx = (HttpRequestContext *)c->fn_data;
    
    if (ev == MG_EV_CONNECT) {
        // Connection established
        LOG_DEBUG("HTTP connection established");
    } else if (ev == MG_EV_HTTP_MSG) {
        // HTTP response received
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        
        ctx->status_code = mg_http_status(hm);
        LOG_DEBUG("HTTP response received, status: %d", ctx->status_code);
        
        if (ctx->status_code == 200) {
            // Copy response body to buffer
            size_t copy_len = hm->body.len;
            if (copy_len >= ctx->response_size) {
                copy_len = ctx->response_size - 1;
            }
            
            memcpy(ctx->response_buffer, hm->body.buf, copy_len);
            ctx->response_buffer[copy_len] = '\0';
            ctx->response_len = copy_len;
            ctx->request_success = true;
        } else {
            LOG_ERROR("HTTP request failed with status: %d", ctx->status_code);
            ctx->request_success = false;
        }
        
        ctx->request_done = true;
        c->is_draining = 1;  // Close connection
    } else if (ev == MG_EV_ERROR) {
        // Connection error
        LOG_ERROR("HTTP connection error: %s", (char *)ev_data);
        ctx->request_done = true;
        ctx->request_success = false;
    } else if (ev == MG_EV_CLOSE) {
        // Connection closed
        LOG_DEBUG("HTTP connection closed");
        ctx->request_done = true;
    }
}

CameraInterface* mac_camera_create(void) {
    CameraInterface* interface = (CameraInterface*)malloc(sizeof(CameraInterface));
    if (!interface) {
        LOG_ERROR("Failed to allocate memory for camera interface");
        return NULL;
    }

    MacCameraData* data = (MacCameraData*)malloc(sizeof(MacCameraData));
    if (!data) {
        LOG_ERROR("Failed to allocate memory for Mac camera data");
        free(interface);
        return NULL;
    }

    // Initialize camera data
    memset(data, 0, sizeof(MacCameraData));
    data->initialized = false;
    data->capturing = false;
    data->frame_ready = false;
    data->capture_in_progress = false;
    data->h_mirror_enabled = false;
    data->v_flip_enabled = false;
    data->http_mgr_initialized = false;
    
    // Set default configuration
    data->config.width = 1280;
    data->config.height = 720;
    data->config.quality = 80;
    data->config.format = 1; // JPEG
    data->config.h_mirror = false;
    data->config.v_flip = false;

    // Initialize interface
    memset(interface, 0, sizeof(CameraInterface));
    interface->vtable = &mac_camera_vtable;
    interface->impl_data = data;
    interface->is_initialized = false;
    interface->is_capturing = false;
    interface->config = data->config;

    LOG_INFO("Mac camera interface created successfully");
    return interface;
}

static int mac_camera_init(CameraInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }

    MacCameraData* data = (MacCameraData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid Mac camera data");
        return -1;
    }

    if (data->initialized) {
        LOG_INFO("Mac camera already initialized");
        return 0;
    }

    // Initialize Mac camera hardware
    int result = mac_camera_init_hardware(data);
    if (result != 0) {
        LOG_ERROR("Failed to initialize Mac camera hardware");
        return result;
    }

    // Initialize mongoose HTTP manager
    mg_mgr_init(&data->http_mgr);
    data->http_mgr_initialized = true;

    data->initialized = true;
    self->is_initialized = true;
    self->config = data->config;

    LOG_INFO("Mac camera initialized successfully");
    return 0;
}

static int mac_camera_set_config(CameraInterface* self, const CameraConfig* config) {
    if (!self || !config) {
        LOG_ERROR("Invalid camera interface or config");
        return -1;
    }

    MacCameraData* data = (MacCameraData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid Mac camera data");
        return -1;
    }

    return mac_camera_set_config_internal(data, config);
}

static int mac_camera_capture(CameraInterface* self, CameraFrameBuffer* frame) {
    if (!self || !frame) {
        LOG_ERROR("Invalid camera interface or frame buffer");
        return -1;
    }

    MacCameraData* data = (MacCameraData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid Mac camera data");
        return -1;
    }

    if (!data->initialized) {
        LOG_ERROR("Mac camera not initialized");
        return -1;
    }

    return mac_camera_capture_internal(data, frame);
}

static int mac_camera_set_h_mirror(CameraInterface* self, bool enabled) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }

    MacCameraData* data = (MacCameraData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid Mac camera data");
        return -1;
    }

    // Update local config
    data->config.h_mirror = enabled;
    self->config.h_mirror = enabled;

    return mac_camera_set_h_mirror_internal(data, enabled);
}

static int mac_camera_set_v_flip(CameraInterface* self, bool enabled) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }

    MacCameraData* data = (MacCameraData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid Mac camera data");
        return -1;
    }

    // Update local config
    data->config.v_flip = enabled;
    self->config.v_flip = enabled;

    return mac_camera_set_v_flip_internal(data, enabled);
}

static int mac_camera_set_explain_url(CameraInterface* self, const char* url, const char* token) {
    if (!self || !url) {
        LOG_ERROR("Invalid camera interface or URL");
        return -1;
    }

    MacCameraData* data = (MacCameraData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid Mac camera data");
        return -1;
    }

    // Free existing URL and token
    if (data->explain_url) {
        free(data->explain_url);
        data->explain_url = NULL;
    }
    if (data->explain_token) {
        free(data->explain_token);
        data->explain_token = NULL;
    }

    // Set new URL
    data->explain_url = (char*)malloc(strlen(url) + 1);
    if (!data->explain_url) {
        LOG_ERROR("Failed to allocate memory for explain URL");
        return -1;
    }
    strcpy(data->explain_url, url);

    // Set new token if provided
    if (token) {
        data->explain_token = (char*)malloc(strlen(token) + 1);
        if (!data->explain_token) {
            LOG_ERROR("Failed to allocate memory for explain token");
            free(data->explain_url);
            data->explain_url = NULL;
            return -1;
        }
        strcpy(data->explain_token, token);
    }

    LOG_INFO("Mac camera explain URL set successfully");
    return 0;
}

static int mac_camera_explain(CameraInterface* self, const char* question, char* response, size_t response_size) {
    if (!self || !question || !response || response_size == 0) {
        LOG_ERROR("Invalid camera interface, question, response buffer, or size");
        return -1;
    }

    MacCameraData* data = (MacCameraData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid Mac camera data");
        return -1;
    }

    if (!data->explain_url) {
        LOG_ERROR("Explain URL not set");
        return -1;
    }

    return mac_camera_explain_internal(data, question, response, response_size);
}

static int mac_camera_release_frame(CameraInterface* self, CameraFrameBuffer* frame) {
    if (!self || !frame) {
        LOG_ERROR("Invalid camera interface or frame buffer");
        return -1;
    }

    if (frame->data) {
        free(frame->data);
        frame->data = NULL;
        frame->size = 0;
        frame->width = 0;
        frame->height = 0;
        frame->format = 0;
    }

    return 0;
}

static int mac_camera_destroy(CameraInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }

    MacCameraData* data = (MacCameraData*)self->impl_data;
    if (data) {
        // Cleanup hardware resources
        mac_camera_cleanup_hardware(data);

        // Cleanup mongoose HTTP manager
        if (data->http_mgr_initialized) {
            mg_mgr_free(&data->http_mgr);
            data->http_mgr_initialized = false;
        }

        // Free allocated memory
        if (data->explain_url) {
            free(data->explain_url);
        }
        if (data->explain_token) {
            free(data->explain_token);
        }
        if (data->current_frame_data) {
            free(data->current_frame_data);
        }

        free(data);
    }

    free(self);
    LOG_INFO("Mac camera destroyed successfully");
    return 0;
}

// Implementation of internal functions

int mac_camera_set_config_internal(MacCameraData* camera_data, const CameraConfig* config) {
    if (!camera_data || !config) {
        LOG_ERROR("Invalid camera data or config");
        return -1;
    }

    // Validate configuration
    if (config->width <= 0 || config->height <= 0) {
        LOG_ERROR("Invalid image dimensions: %dx%d", config->width, config->height);
        return -1;
    }

    if (config->quality < 1 || config->quality > 100) {
        LOG_ERROR("Invalid JPEG quality: %d", config->quality);
        return -1;
    }

    // Update configuration
    camera_data->config = *config;

    LOG_INFO("Mac camera configuration updated: %dx%d, quality=%d, format=%d",
             config->width, config->height, config->quality, config->format);
    return 0;
}

int mac_camera_capture_internal(MacCameraData* camera_data, CameraFrameBuffer* frame) {
    if (!camera_data || !frame) {
        LOG_ERROR("Invalid camera data or frame buffer");
        return -1;
    }

    if (camera_data->capture_in_progress) {
        LOG_WARN("Capture already in progress");
        return -1;
    }

    long long start_time = get_timestamp_ms();

#ifdef __APPLE__
    // For now, create a dummy frame for testing
    // In a real implementation, this would capture from AVFoundation
    int width = camera_data->config.width;
    int height = camera_data->config.height;
    size_t frame_size = width * height * 3; // RGB24

    uint8_t* dummy_data = (uint8_t*)malloc(frame_size);
    if (!dummy_data) {
        LOG_ERROR("Failed to allocate memory for dummy frame");
        return -1;
    }

    // Generate a simple test pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = (y * width + x) * 3;
            dummy_data[offset] = (uint8_t)((x * 255) / width);     // Red
            dummy_data[offset + 1] = (uint8_t)((y * 255) / height); // Green
            dummy_data[offset + 2] = 128;                           // Blue
        }
    }

    // Convert to JPEG if needed
    if (camera_data->config.format == 1) { // JPEG format
        uint8_t* jpeg_data = NULL;
        size_t jpeg_size = 0;
        
        int result = mac_camera_convert_to_jpeg(camera_data, dummy_data, frame_size, &jpeg_data, &jpeg_size);
        free(dummy_data);
        
        if (result != 0) {
            LOG_ERROR("Failed to convert frame to JPEG");
            return -1;
        }
        
        frame->data = jpeg_data;
        frame->size = jpeg_size;
        frame->format = 1; // JPEG
    } else {
        frame->data = dummy_data;
        frame->size = frame_size;
        frame->format = 0; // RGB
    }

    frame->width = width;
    frame->height = height;

#else
    // Non-Apple platform fallback
    LOG_ERROR("Mac camera not supported on this platform");
    return -1;
#endif

    long long end_time = get_timestamp_ms();
    LOG_INFO("Mac camera captured frame in %lld ms: %dx%d, size=%zu", 
             end_time - start_time, frame->width, frame->height, frame->size);

    return 0;
}

int mac_camera_set_h_mirror_internal(MacCameraData* camera_data, bool enabled) {
    if (!camera_data) {
        LOG_ERROR("Invalid camera data");
        return -1;
    }

    camera_data->h_mirror_enabled = enabled;
    LOG_INFO("Mac camera horizontal mirror set to: %s", enabled ? "enabled" : "disabled");
    return 0;
}

int mac_camera_set_v_flip_internal(MacCameraData* camera_data, bool enabled) {
    if (!camera_data) {
        LOG_ERROR("Invalid camera data");
        return -1;
    }

    camera_data->v_flip_enabled = enabled;
    LOG_INFO("Mac camera vertical flip set to: %s", enabled ? "enabled" : "disabled");
    return 0;
}

int mac_camera_explain_internal(MacCameraData* camera_data, const char* question, 
                                char* response, size_t response_size) {
    if (!camera_data || !question || !response || response_size == 0) {
        LOG_ERROR("Invalid parameters for explain");
        return -1;
    }

    if (!camera_data->current_frame_data || camera_data->current_frame_size == 0) {
        LOG_ERROR("No current frame available for explanation");
        return -1;
    }

    // Convert current frame to JPEG if needed
    uint8_t* jpeg_data = NULL;
    size_t jpeg_size = 0;
    bool need_free_jpeg = false;

    if (camera_data->current_frame_format == 1) { // Already JPEG
        jpeg_data = camera_data->current_frame_data;
        jpeg_size = camera_data->current_frame_size;
    } else {
        // Convert to JPEG
        int result = mac_camera_convert_to_jpeg(camera_data, 
                                               camera_data->current_frame_data, 
                                               camera_data->current_frame_size,
                                               &jpeg_data, &jpeg_size);
        if (result != 0) {
            LOG_ERROR("Failed to convert frame to JPEG for explanation");
            return -1;
        }
        need_free_jpeg = true;
    }

    // Send explain request
    int result = mac_camera_send_explain_request(camera_data, camera_data->explain_url, 
                                                camera_data->explain_token,
                                                question, jpeg_data, jpeg_size,
                                                response, response_size);

    if (need_free_jpeg && jpeg_data) {
        free(jpeg_data);
    }

    if (result != 0) {
        LOG_ERROR("Failed to send explain request");
        return -1;
    }

    LOG_INFO("Mac camera explain completed for question: %s", question);
    return 0;
}

int mac_camera_init_hardware(MacCameraData* camera_data) {
    if (!camera_data) {
        LOG_ERROR("Invalid camera data");
        return -1;
    }

#ifdef __APPLE__
    // Initialize AVFoundation capture session
    // For now, this is a stub implementation
    // In a real implementation, you would:
    // 1. Create AVCaptureSession
    // 2. Find and configure AVCaptureDevice
    // 3. Create AVCaptureDeviceInput
    // 4. Create AVCaptureVideoDataOutput
    // 5. Set up delegate for frame capture
    // 6. Start capture session

    LOG_WARN("Mac camera hardware initialization (stub)");
    return 0;
#else
    LOG_ERROR("Mac camera hardware not supported on this platform");
    return -1;
#endif
}

void mac_camera_cleanup_hardware(MacCameraData* camera_data) {
    if (!camera_data) {
        return;
    }

#ifdef __APPLE__
    // Cleanup AVFoundation resources
    // For now, this is a stub implementation
    LOG_INFO("Mac camera hardware cleanup (stub)");
#endif
}

int mac_camera_convert_to_jpeg(MacCameraData* camera_data, 
                              const uint8_t* raw_data, size_t raw_size,
                              uint8_t** jpeg_data, size_t* jpeg_size) {
    if (!camera_data || !raw_data || !jpeg_data || !jpeg_size) {
        LOG_ERROR("Invalid parameters for JPEG conversion");
        return -1;
    }

#ifdef __APPLE__
    // Use Core Graphics and Image I/O for JPEG conversion
    // For now, this is a simplified stub implementation
    
    // Allocate memory for JPEG data (estimate 1/10 of raw size)
    size_t estimated_jpeg_size = raw_size / 10;
    if (estimated_jpeg_size < 1024) {
        estimated_jpeg_size = 1024;
    }
    
    *jpeg_data = (uint8_t*)malloc(estimated_jpeg_size);
    if (!*jpeg_data) {
        LOG_ERROR("Failed to allocate memory for JPEG data");
        return -1;
    }
    
    // For testing, just copy a portion of the raw data
    *jpeg_size = estimated_jpeg_size;
    memset(*jpeg_data, 0xFF, *jpeg_size); // Fill with 0xFF to simulate JPEG header
    
    LOG_INFO("Converted frame to JPEG: %zu -> %zu bytes", raw_size, *jpeg_size);
    return 0;
#else
    LOG_ERROR("JPEG conversion not supported on this platform");
    return -1;
#endif
}

int mac_camera_send_explain_request(MacCameraData* camera_data, const char* url, const char* token,
                                   const char* question,
                                   const uint8_t* jpeg_data, size_t jpeg_size,
                                   char* response, size_t response_size) {
    if (!camera_data || !url || !question || !jpeg_data || !response || response_size == 0) {
        LOG_ERROR("Invalid parameters for explain request");
        return -1;
    }

    if (!camera_data->http_mgr_initialized) {
        LOG_ERROR("HTTP manager not initialized");
        return -1;
    }

    // Initialize HTTP request context
    HttpRequestContext ctx = {
        .response_buffer = response,
        .response_size = response_size,
        .response_len = 0,
        .request_done = false,
        .request_success = false,
        .status_code = 0
    };

    // Create HTTP connection
    struct mg_connection *c = mg_http_connect(&camera_data->http_mgr, url, http_event_handler, &ctx);
    if (!c) {
        LOG_ERROR("Failed to create HTTP connection to %s", url);
        return -1;
    }

    // Generate boundary for multipart form data
    char boundary[64];
    snprintf(boundary, sizeof(boundary), "----MacCameraBoundary%lld", get_timestamp_ms());

    // Calculate content length for multipart form data
    size_t question_part_len = strlen("--") + strlen(boundary) + 
                              strlen("\r\nContent-Disposition: form-data; name=\"question\"\r\n\r\n") +
                              strlen(question) + strlen("\r\n");
    
    size_t image_part_len = strlen("--") + strlen(boundary) + 
                           strlen("\r\nContent-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n") +
                           jpeg_size + strlen("\r\n");
    
    size_t end_boundary_len = strlen("--") + strlen(boundary) + strlen("--\r\n");
    size_t total_content_len = question_part_len + image_part_len + end_boundary_len;

    // Send HTTP headers
    mg_printf(c, "POST %s HTTP/1.1\r\n", mg_url_uri(url));
    mg_printf(c, "Host: %.*s\r\n", (int)mg_url_host(url).len, mg_url_host(url).buf);
    mg_printf(c, "Content-Type: multipart/form-data; boundary=%s\r\n", boundary);
    mg_printf(c, "Content-Length: %zu\r\n", total_content_len);
    
    if (token && strlen(token) > 0) {
        mg_printf(c, "Authorization: Bearer %s\r\n", token);
    }
    
    mg_printf(c, "\r\n");

    // Send question part
    mg_printf(c, "--%s\r\n", boundary);
    mg_printf(c, "Content-Disposition: form-data; name=\"question\"\r\n\r\n");
    mg_printf(c, "%s\r\n", question);

    // Send image part
    mg_printf(c, "--%s\r\n", boundary);
    mg_printf(c, "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n");
    mg_printf(c, "Content-Type: image/jpeg\r\n\r\n");
    mg_send(c, jpeg_data, jpeg_size);
    mg_printf(c, "\r\n");

    // Send end boundary
    mg_printf(c, "--%s--\r\n", boundary);

    LOG_INFO("HTTP request sent to %s, question=%s, jpeg_size=%zu", url, question, jpeg_size);

    // Wait for response with timeout
    long long start_time = get_timestamp_ms();
    while (!ctx.request_done && (get_timestamp_ms() - start_time) < HTTP_TIMEOUT_MS) {
        mg_mgr_poll(&camera_data->http_mgr, 100);  // Poll every 100ms
    }

    if (!ctx.request_done) {
        LOG_ERROR("HTTP request timeout after %d ms", HTTP_TIMEOUT_MS);
        c->is_draining = 1;  // Force close connection
        mg_mgr_poll(&camera_data->http_mgr, 100);  // Final poll to clean up
        return -1;
    }

    if (!ctx.request_success) {
        LOG_ERROR("HTTP request failed with status: %d", ctx.status_code);
        return -1;
    }

    LOG_INFO("HTTP request completed successfully, response length: %zu", ctx.response_len);
    return 0;
}