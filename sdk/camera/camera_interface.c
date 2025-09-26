#include "camera_interface.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>

int camera_interface_init(CameraInterface* self) {
    if (!self || !self->vtable || !self->vtable->init) {
        LOG_ERROR("Invalid camera interface or vtable");
        return -1;
    }
    return self->vtable->init(self);
}

int camera_interface_set_config(CameraInterface* self, const CameraConfig* config) {
    if (!self || !config) {
        LOG_ERROR("Invalid camera interface or config");
        return -1;
    }
    
    // Store configuration in the interface
    self->config = *config;
    
    if (self->vtable && self->vtable->set_config) {
        return self->vtable->set_config(self, config);
    }
    
    return 0;
}

int camera_interface_capture(CameraInterface* self, CameraFrameBuffer* frame) {
    if (!self || !frame || !self->vtable || !self->vtable->capture) {
        LOG_ERROR("Invalid camera interface, frame buffer, or vtable");
        return -1;
    }
    return self->vtable->capture(self, frame);
}

int camera_interface_set_h_mirror(CameraInterface* self, bool enabled) {
    if (!self || !self->vtable || !self->vtable->set_h_mirror) {
        LOG_ERROR("Invalid camera interface or vtable");
        return -1;
    }
    
    // Update local config
    self->config.h_mirror = enabled;
    
    return self->vtable->set_h_mirror(self, enabled);
}

int camera_interface_set_v_flip(CameraInterface* self, bool enabled) {
    if (!self || !self->vtable || !self->vtable->set_v_flip) {
        LOG_ERROR("Invalid camera interface or vtable");
        return -1;
    }
    
    // Update local config
    self->config.v_flip = enabled;
    
    return self->vtable->set_v_flip(self, enabled);
}

int camera_interface_set_explain_url(CameraInterface* self, const char* url, const char* token) {
    if (!self || !url || !token) {
        LOG_ERROR("Invalid camera interface, URL, or token");
        return -1;
    }
    
    // Free existing strings
    if (self->explain_url) {
        free(self->explain_url);
        self->explain_url = NULL;
    }
    if (self->explain_token) {
        free(self->explain_token);
        self->explain_token = NULL;
    }
    
    // Allocate and copy new strings
    self->explain_url = malloc(strlen(url) + 1);
    if (!self->explain_url) {
        LOG_ERROR("Failed to allocate memory for explain URL");
        return -1;
    }
    strcpy(self->explain_url, url);
    
    self->explain_token = malloc(strlen(token) + 1);
    if (!self->explain_token) {
        LOG_ERROR("Failed to allocate memory for explain token");
        free(self->explain_url);
        self->explain_url = NULL;
        return -1;
    }
    strcpy(self->explain_token, token);
    
    if (self->vtable && self->vtable->set_explain_url) {
        return self->vtable->set_explain_url(self, url, token);
    }
    
    return 0;
}

int camera_interface_explain(CameraInterface* self, const char* question, char* response, size_t response_size) {
    if (!self || !question || !response || response_size == 0) {
        LOG_ERROR("Invalid camera interface, question, response buffer, or size");
        return -1;
    }
    
    if (!self->vtable || !self->vtable->explain) {
        LOG_ERROR("Invalid camera interface vtable or explain function");
        return -1;
    }
    
    return self->vtable->explain(self, question, response, response_size);
}

int camera_interface_release_frame(CameraInterface* self, CameraFrameBuffer* frame) {
    if (!self || !frame) {
        LOG_ERROR("Invalid camera interface or frame buffer");
        return -1;
    }
    
    if (self->vtable && self->vtable->release_frame) {
        return self->vtable->release_frame(self, frame);
    }
    
    // Default implementation: just clear the frame buffer
    if (frame->data) {
        frame->data = NULL;
        frame->size = 0;
        frame->width = 0;
        frame->height = 0;
        frame->format = 0;
    }
    
    return 0;
}

int camera_interface_destroy(CameraInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid camera interface");
        return -1;
    }
    
    // Free explain URL and token
    if (self->explain_url) {
        free(self->explain_url);
        self->explain_url = NULL;
    }
    if (self->explain_token) {
        free(self->explain_token);
        self->explain_token = NULL;
    }
    
    if (self->vtable && self->vtable->destroy) {
        return self->vtable->destroy(self);
    }
    
    return 0;
}