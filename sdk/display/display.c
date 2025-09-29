#include "display.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>

int display_interface_init(DisplayInterface* self) {
    if (!self || !self->vtable || !self->vtable->init) {
        LOG_ERROR("Invalid display interface or vtable");
        return -1;
    }
    return self->vtable->init(self);
}

void display_interface_set_status(DisplayInterface* self, const char* status) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return;
    }
    
    if (self->vtable && self->vtable->set_status) {
        self->vtable->set_status(self, status);
    } else {
        LOG_WARN("set_status not implemented");
    }
}

void display_interface_show_notification(DisplayInterface* self, const char* notification, int duration_ms) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return;
    }
    
    if (self->vtable && self->vtable->show_notification) {
        self->vtable->show_notification(self, notification, duration_ms);
    } else {
        LOG_WARN("show_notification not implemented");
    }
}

void display_interface_set_emotion(DisplayInterface* self, const char* emotion) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return;
    }
    
    if (self->vtable && self->vtable->set_emotion) {
        self->vtable->set_emotion(self, emotion);
    } else {
        LOG_WARN("set_emotion not implemented");
    }
}

void display_interface_set_chat_message(DisplayInterface* self, const char* role, const char* content) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return;
    }
    
    if (self->vtable && self->vtable->set_chat_message) {
        self->vtable->set_chat_message(self, role, content);
    } else {
        LOG_WARN("set_chat_message not implemented");
    }
}

void display_interface_set_theme(DisplayInterface* self, DisplayTheme* theme) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return;
    }
    
    self->current_theme = theme;
    
    if (self->vtable && self->vtable->set_theme) {
        self->vtable->set_theme(self, theme);
    } else {
        LOG_WARN("set_theme not implemented");
    }
}

DisplayTheme* display_interface_get_theme(DisplayInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return NULL;
    }
    
    if (self->vtable && self->vtable->get_theme) {
        return self->vtable->get_theme(self);
    } else {
        return self->current_theme;
    }
}

void display_interface_update_status_bar(DisplayInterface* self, bool update_all) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return;
    }
    
    if (self->vtable && self->vtable->update_status_bar) {
        self->vtable->update_status_bar(self, update_all);
    } else {
        LOG_WARN("update_status_bar not implemented");
    }
}

void display_interface_set_power_save_mode(DisplayInterface* self, bool on) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return;
    }
    
    self->power_save_mode = on;
    
    if (self->vtable && self->vtable->set_power_save_mode) {
        self->vtable->set_power_save_mode(self, on);
    } else {
        LOG_WARN("set_power_save_mode not implemented");
    }
}

bool display_interface_lock(DisplayInterface* self, int timeout_ms) {
    if (!self || !self->vtable || !self->vtable->lock) {
        LOG_ERROR("Invalid display interface or vtable");
        return false;
    }
    
    bool result = self->vtable->lock(self, timeout_ms);
    if (result) {
        self->is_locked = true;
    }
    return result;
}

void display_interface_unlock(DisplayInterface* self) {
    if (!self || !self->vtable || !self->vtable->unlock) {
        LOG_ERROR("Invalid display interface or vtable");
        return;
    }
    
    self->vtable->unlock(self);
    self->is_locked = false;
}

int display_interface_get_width(DisplayInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return -1;
    }
    return self->width;
}

int display_interface_get_height(DisplayInterface* self) {
    if (!self) {
        LOG_ERROR("Invalid display interface");
        return -1;
    }
    return self->height;
}

int display_interface_destroy(DisplayInterface* self) {
    if (!self || !self->vtable || !self->vtable->destroy) {
        LOG_ERROR("Invalid display interface or vtable");
        return -1;
    }
    
    return self->vtable->destroy(self);
}
