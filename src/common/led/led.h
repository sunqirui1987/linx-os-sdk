#ifndef _LED_H_
#define _LED_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED interface structure
 * 
 * This structure defines the interface for LED operations.
 * Implementations should provide function pointers for the required operations.
 */
typedef struct led_interface {
    /**
     * @brief Set the LED state based on the device state
     * @param led_ctx Pointer to the LED context/instance
     */
    void (*on_state_changed)(void *led_ctx);
    
    /**
     * @brief Destroy/cleanup the LED instance
     * @param led_ctx Pointer to the LED context/instance
     */
    void (*destroy)(void *led_ctx);
    
    /**
     * @brief LED context/instance data
     */
    void *ctx;
} led_interface_t;

/**
 * @brief Initialize a LED interface
 * @param led Pointer to the LED interface structure
 * @param ctx Pointer to the LED context/instance data
 * @param on_state_changed Function pointer for state change callback
 * @param destroy Function pointer for cleanup callback (can be NULL)
 */
void led_interface_init(led_interface_t *led, 
                       void *ctx,
                       void (*on_state_changed)(void *led_ctx),
                       void (*destroy)(void *led_ctx));

/**
 * @brief Call the state changed callback
 * @param led Pointer to the LED interface
 */
void led_on_state_changed(led_interface_t *led);

/**
 * @brief Destroy the LED interface
 * @param led Pointer to the LED interface
 */
void led_destroy(led_interface_t *led);

/**
 * @brief No-operation LED implementation
 * 
 * This is equivalent to the NoLed class in the original C++ code.
 * It provides a default implementation that does nothing.
 */
extern led_interface_t no_led_instance;

/**
 * @brief Get a no-operation LED instance
 * @return Pointer to a no-operation LED interface
 */
led_interface_t* led_get_no_led(void);

#ifdef __cplusplus
}
#endif

#endif // _LED_H_