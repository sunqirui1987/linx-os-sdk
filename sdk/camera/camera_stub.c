#include "camera_stub.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations for vtable functions
static int camera_stub_init(CameraInterface* self);
static int camera_stub_set_config(CameraInterface* self, const CameraConfig* config);
static int camera_stub_capture(CameraInterface* self, CameraFrameBuffer* frame);
static int camera_stub_set_h_mirror(CameraInterface* self, bool enabled);
static int camera_stub_set_v_flip(CameraInterface* self, bool enabled);
static int camera_stub_set_explain_url(CameraInterface* self, const char* url, const char* token);
static int camera_stub_explain(CameraInterface* self, const char* question, char* response, size_t response_size);
static int camera_stub_release_frame(CameraInterface* self, CameraFrameBuffer* frame);
static int camera_stub_destroy(CameraInterface* self);

// Stub vtable
static const CameraInterfaceVTable camera_stub_vtable = {
    .init = camera_stub_init,
    .set_config = camera_stub_set_config,
    .capture = camera_stub_capture,
    .set_h_mirror = camera_stub_set_h_mirror,
    .set_v_flip = camera_stub_set_v_flip,
    .set_explain_url = camera_stub_set_explain_url,
    .explain = camera_stub_explain,
    .release_frame = camera_stub_release_frame,
    .destroy = camera_stub_destroy
};

CameraInterface* camera_stub_create(void) {
    CameraInterface* interface = (CameraInterface*)malloc(sizeof(CameraInterface));
    if (!interface) {
        LOG_ERROR("Failed to allocate memory for camera interface");
        return NULL;
    }
    
    CameraStubData* data = (CameraStubData*)malloc(sizeof(CameraStubData));
    if (!data) {
        LOG_ERROR("Failed to allocate memory for camera stub data");
        free(interface);
        return NULL;
    }
    
    // Initialize interface
    memset(interface, 0, sizeof(CameraInterface));
    interface->vtable = &camera_stub_vtable;
    interface->impl_data = data;
    
    // Initialize stub data
    memset(data, 0, sizeof(CameraStubData));
    
    // Set default configuration
    data->config.width = 640;
    data->config.height = 480;
    data->config.quality = 80;
    data->config.format = 1; // JPEG
    data->config.h_mirror = false;
    data->config.v_flip = false;
    
    // Create dummy frame data (simple pattern)
    data->dummy_frame_size = 1024; // 1KB dummy JPEG data
    data->dummy_frame_data = (uint8_t*)malloc(data->dummy_frame_size);
    if (data->dummy_frame_data) {
        // Fill with a simple pattern
        for (size_t i = 0; i < data->dummy_frame_size; i++) {
            data->dummy_frame_data[i] = (uint8_t)(i % 256);
        }
    }
    
    return interface;
}

static int camera_stub_init(CameraInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }
    
    CameraStubData* data = (CameraStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid camera stub data");
        return -1;
    }
    
    data->initialized = true;
    self->is_initialized = true;
    self->config = data->config;
    
    LOG_INFO("Camera stub initialized successfully");
    return 0; // Success
}

static int camera_stub_set_config(CameraInterface* self, const CameraConfig* config) {
    if (!self || !config) {
        LOG_ERROR("Invalid camera interface or config");
        return -1;
    }
    
    CameraStubData* data = (CameraStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid camera stub data");
        return -1;
    }
    
    // Store configuration
    data->config = *config;
    self->config = *config;
    
    LOG_INFO("Camera stub config set: %dx%d, quality=%d", config->width, config->height, config->quality);
    return 0;
}

static int camera_stub_capture(CameraInterface* self, CameraFrameBuffer* frame) {
    if (!self || !frame) {
        LOG_ERROR("Invalid camera interface or frame buffer");
        return -1;
    }
    
    CameraStubData* data = (CameraStubData*)self->impl_data;
    if (!data || !data->initialized) {
        LOG_ERROR("Camera stub not initialized");
        return -1;
    }
    
    // Return dummy frame data
    frame->data = data->dummy_frame_data;
    frame->size = data->dummy_frame_size;
    frame->width = data->config.width;
    frame->height = data->config.height;
    frame->format = data->config.format;
    
    data->capturing = true;
    self->is_capturing = true;
    
    LOG_INFO("Camera stub captured frame: %dx%d, size=%zu", frame->width, frame->height, frame->size);
    return 0;
}

static int camera_stub_set_h_mirror(CameraInterface* self, bool enabled) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }
    
    CameraStubData* data = (CameraStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid camera stub data");
        return -1;
    }
    
    data->config.h_mirror = enabled;
    self->config.h_mirror = enabled;
    
    LOG_INFO("Camera stub horizontal mirror set to: %s", enabled ? "enabled" : "disabled");
    return 0;
}

static int camera_stub_set_v_flip(CameraInterface* self, bool enabled) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }
    
    CameraStubData* data = (CameraStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid camera stub data");
        return -1;
    }
    
    data->config.v_flip = enabled;
    self->config.v_flip = enabled;
    
    LOG_INFO("Camera stub vertical flip set to: %s", enabled ? "enabled" : "disabled");
    return 0;
}

static int camera_stub_set_explain_url(CameraInterface* self, const char* url, const char* token) {
    if (!self || !url || !token) {
        LOG_ERROR("Invalid camera interface, URL, or token");
        return -1;
    }
    
    CameraStubData* data = (CameraStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid camera stub data");
        return -1;
    }
    
    // Free existing strings
    if (data->explain_url) {
        free(data->explain_url);
        data->explain_url = NULL;
    }
    if (data->explain_token) {
        free(data->explain_token);
        data->explain_token = NULL;
    }
    
    // Allocate and copy new strings
    data->explain_url = malloc(strlen(url) + 1);
    if (!data->explain_url) {
        LOG_ERROR("Failed to allocate memory for explain URL");
        return -1;
    }
    strcpy(data->explain_url, url);
    
    data->explain_token = malloc(strlen(token) + 1);
    if (!data->explain_token) {
        LOG_ERROR("Failed to allocate memory for explain token");
        free(data->explain_url);
        data->explain_url = NULL;
        return -1;
    }
    strcpy(data->explain_token, token);
    
    LOG_INFO("Camera stub explain URL set: %s", url);
    return 0;
}

static int camera_stub_explain(CameraInterface* self, const char* question, char* response, size_t response_size) {
    if (!self || !question || !response || response_size == 0) {
        LOG_ERROR("Invalid camera interface, question, response buffer, or size");
        return -1;
    }
    
    CameraStubData* data = (CameraStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("Invalid camera stub data");
        return -1;
    }
    
    // Generate a dummy response
    const char* dummy_response = "This is a stub camera implementation. The explain feature is not available.";
    size_t response_len = strlen(dummy_response);
    
    if (response_size <= response_len) {
        LOG_ERROR("Response buffer too small");
        return -1;
    }
    
    strcpy(response, dummy_response);
    
    LOG_INFO("Camera stub explain called with question: %s", question);
    return 0;
}

static int camera_stub_release_frame(CameraInterface* self, CameraFrameBuffer* frame) {
    if (!self || !frame) {
        LOG_ERROR("Invalid camera interface or frame buffer");
        return -1;
    }
    
    // For stub implementation, we don't need to free the dummy data
    // Just clear the frame buffer pointers
    frame->data = NULL;
    frame->size = 0;
    frame->width = 0;
    frame->height = 0;
    frame->format = 0;
    
    self->is_capturing = false;
    
    LOG_INFO("Camera stub frame released");
    return 0;
}

static int camera_stub_destroy(CameraInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }
    
    CameraStubData* data = (CameraStubData*)self->impl_data;
    if (data) {
        // Free explain URL and token
        if (data->explain_url) {
            free(data->explain_url);
        }
        if (data->explain_token) {
            free(data->explain_token);
        }
        
        // Free dummy frame data
        if (data->dummy_frame_data) {
            free(data->dummy_frame_data);
        }
        
        free(data);
    }
    
    free(self);
    
    LOG_INFO("Camera stub destroyed");
    return 0;
}