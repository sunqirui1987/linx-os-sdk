#include "led.h"
#include <stddef.h>

/**
 * @brief No-operation state change callback
 * @param led_ctx LED context (unused)
 */
static void no_led_on_state_changed(void *led_ctx) {
    // Do nothing - equivalent to NoLed::OnStateChanged() override {}
    (void)led_ctx; // Suppress unused parameter warning
}

/**
 * @brief No-operation destroy callback
 * @param led_ctx LED context (unused)
 */
static void no_led_destroy(void *led_ctx) {
    // Do nothing for no-op LED
    (void)led_ctx; // Suppress unused parameter warning
}

/**
 * @brief Global no-operation LED instance
 */
led_interface_t no_led_instance = {
    .on_state_changed = no_led_on_state_changed,
    .destroy = no_led_destroy,
    .ctx = NULL
};

void led_interface_init(led_interface_t *led, 
                       void *ctx,
                       void (*on_state_changed)(void *led_ctx),
                       void (*destroy)(void *led_ctx)) {
    if (led == NULL) {
        return;
    }
    
    led->ctx = ctx;
    led->on_state_changed = on_state_changed;
    led->destroy = destroy;
}

void led_on_state_changed(led_interface_t *led) {
    if (led != NULL && led->on_state_changed != NULL) {
        led->on_state_changed(led->ctx);
    }
}

void led_destroy(led_interface_t *led) {
    if (led != NULL && led->destroy != NULL) {
        led->destroy(led->ctx);
        led->ctx = NULL;
        led->on_state_changed = NULL;
        led->destroy = NULL;
    }
}

led_interface_t* led_get_no_led(void) {
    return &no_led_instance;
}