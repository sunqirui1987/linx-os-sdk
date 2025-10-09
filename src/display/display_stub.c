#include "display_stub.h"
#include "../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations of stub implementation functions
static int display_stub_init(DisplayInterface* self);
static void display_stub_set_status(DisplayInterface* self, const char* status);
static void display_stub_show_notification(DisplayInterface* self, const char* notification, int duration_ms);
static void display_stub_set_emotion(DisplayInterface* self, const char* emotion);
static void display_stub_set_chat_message(DisplayInterface* self, const char* role, const char* content);
static void display_stub_set_theme(DisplayInterface* self, DisplayTheme* theme);
static DisplayTheme* display_stub_get_theme(DisplayInterface* self);
static void display_stub_update_status_bar(DisplayInterface* self, bool update_all);
static void display_stub_set_power_save_mode(DisplayInterface* self, bool on);
static bool display_stub_lock(DisplayInterface* self, int timeout_ms);
static void display_stub_unlock(DisplayInterface* self);
static int display_stub_destroy(DisplayInterface* self);

// Stub implementation vtable
static const DisplayInterfaceVTable display_stub_vtable = {
    .init = display_stub_init,
    .set_status = display_stub_set_status,
    .show_notification = display_stub_show_notification,
    .set_emotion = display_stub_set_emotion,
    .set_chat_message = display_stub_set_chat_message,
    .set_theme = display_stub_set_theme,
    .get_theme = display_stub_get_theme,
    .update_status_bar = display_stub_update_status_bar,
    .set_power_save_mode = display_stub_set_power_save_mode,
    .lock = display_stub_lock,
    .unlock = display_stub_unlock,
    .destroy = display_stub_destroy
};

DisplayInterface* display_stub_create(void) {
    return display_stub_create_with_size(320, 240);  // Default size
}

DisplayInterface* display_stub_create_with_size(int width, int height) {
    DisplayInterface* interface = (DisplayInterface*)malloc(sizeof(DisplayInterface));
    if (!interface) {
        LOG_ERROR("Failed to allocate memory for display interface");
        return NULL;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)malloc(sizeof(DisplayStubData));
    if (!stub_data) {
        LOG_ERROR("Failed to allocate memory for display stub data");
        free(interface);
        return NULL;
    }
    
    // Initialize stub data
    memset(stub_data, 0, sizeof(DisplayStubData));
    stub_data->initialized = false;
    stub_data->power_save_mode = false;
    stub_data->is_locked = false;
    stub_data->current_theme = NULL;
    
    // Initialize interface
    memset(interface, 0, sizeof(DisplayInterface));
    interface->vtable = &display_stub_vtable;
    interface->impl_data = stub_data;
    interface->width = width;
    interface->height = height;
    interface->current_theme = NULL;
    interface->is_initialized = false;
    interface->power_save_mode = false;
    interface->is_locked = false;
    interface->lock_data = NULL;
    
    LOG_INFO("Created display stub interface with size %dx%d", width, height);
    return interface;
}

// Stub implementation functions
static int display_stub_init(DisplayInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid display interface");
        return -1;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    stub_data->initialized = true;
    self->is_initialized = true;
    
    LOG_INFO("Display stub initialized");
    return 0;
}

static void display_stub_set_status(DisplayInterface* self, const char* status) {
    if (!self || !self->impl_data || !status) {
        LOG_ERROR("Invalid parameters");
        return;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    strncpy(stub_data->last_status, status, sizeof(stub_data->last_status) - 1);
    stub_data->last_status[sizeof(stub_data->last_status) - 1] = '\0';
    
    LOG_INFO("Display stub set status: %s", status);
}

static void display_stub_show_notification(DisplayInterface* self, const char* notification, int duration_ms) {
    if (!self || !self->impl_data || !notification) {
        LOG_ERROR("Invalid parameters");
        return;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    strncpy(stub_data->last_notification, notification, sizeof(stub_data->last_notification) - 1);
    stub_data->last_notification[sizeof(stub_data->last_notification) - 1] = '\0';
    
    LOG_INFO("Display stub show notification: %s (duration: %d ms)", notification, duration_ms);
}

static void display_stub_set_emotion(DisplayInterface* self, const char* emotion) {
    if (!self || !self->impl_data || !emotion) {
        LOG_ERROR("Invalid parameters");
        return;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    strncpy(stub_data->last_emotion, emotion, sizeof(stub_data->last_emotion) - 1);
    stub_data->last_emotion[sizeof(stub_data->last_emotion) - 1] = '\0';
    
    LOG_INFO("Display stub set emotion: %s", emotion);
}

static void display_stub_set_chat_message(DisplayInterface* self, const char* role, const char* content) {
    if (!self || !self->impl_data || !role || !content) {
        LOG_ERROR("Invalid parameters");
        return;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    strncpy(stub_data->last_role, role, sizeof(stub_data->last_role) - 1);
    stub_data->last_role[sizeof(stub_data->last_role) - 1] = '\0';
    strncpy(stub_data->last_content, content, sizeof(stub_data->last_content) - 1);
    stub_data->last_content[sizeof(stub_data->last_content) - 1] = '\0';
    
    LOG_INFO("Display stub set chat message - Role: %s, Content: %s", role, content);
}

static void display_stub_set_theme(DisplayInterface* self, DisplayTheme* theme) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid parameters");
        return;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    stub_data->current_theme = theme;
    self->current_theme = theme;
    
    if (theme) {
        LOG_INFO("Display stub set theme: %s", theme->name);
    } else {
        LOG_INFO("Display stub set theme: NULL");
    }
}

static DisplayTheme* display_stub_get_theme(DisplayInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid parameters");
        return NULL;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    return stub_data->current_theme;
}

static void display_stub_update_status_bar(DisplayInterface* self, bool update_all) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid parameters");
        return;
    }
    
    LOG_INFO("Display stub update status bar (update_all: %s)", update_all ? "true" : "false");
}

static void display_stub_set_power_save_mode(DisplayInterface* self, bool on) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid parameters");
        return;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    stub_data->power_save_mode = on;
    self->power_save_mode = on;
    
    LOG_INFO("Display stub set power save mode: %s", on ? "on" : "off");
}

static bool display_stub_lock(DisplayInterface* self, int timeout_ms) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid parameters");
        return false;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    stub_data->is_locked = true;
    self->is_locked = true;
    
    LOG_INFO("Display stub locked (timeout: %d ms)", timeout_ms);
    return true;
}

static void display_stub_unlock(DisplayInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid parameters");
        return;
    }
    
    DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
    stub_data->is_locked = false;
    self->is_locked = false;
    
    LOG_INFO("Display stub unlocked");
}

static int display_stub_destroy(DisplayInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return -1;
    }
    
    if (self->impl_data) {
        DisplayStubData* stub_data = (DisplayStubData*)self->impl_data;
        
        // Clean up theme if it was created by this instance
        if (stub_data->current_theme) {
            // Note: Don't free theme here as it might be shared
            LOG_INFO("Display stub had theme: %s", stub_data->current_theme->name);
        }
        
        free(self->impl_data);
    }
    
    free(self);
    LOG_INFO("Display stub destroyed");
    return 0;
}