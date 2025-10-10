#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network interface types
 */
typedef enum {
    NETWORK_TYPE_NONE = 0,
    NETWORK_TYPE_WIFI,
    NETWORK_TYPE_ETHERNET,
    NETWORK_TYPE_CELLULAR
} network_type_t;

/**
 * @brief Network connection state
 */
typedef enum {
    NETWORK_STATE_DISCONNECTED = 0,
    NETWORK_STATE_CONNECTING,
    NETWORK_STATE_CONNECTED,
    NETWORK_STATE_CONFIGURING,
    NETWORK_STATE_ERROR
} network_state_t;

/**
 * @brief WiFi signal strength levels
 */
typedef enum {
    WIFI_SIGNAL_NONE = 0,
    WIFI_SIGNAL_WEAK,
    WIFI_SIGNAL_FAIR,
    WIFI_SIGNAL_GOOD,
    WIFI_SIGNAL_EXCELLENT
} wifi_signal_level_t;

/**
 * @brief Network interface structure
 */
typedef struct network_interface {
    /**
     * @brief Get network type
     * @param net_ctx Network context
     * @return Network type
     */
    network_type_t (*get_type)(void *net_ctx);
    
    /**
     * @brief Start network connection
     * @param net_ctx Network context
     * @return true if started successfully, false otherwise
     */
    bool (*start)(void *net_ctx);
    
    /**
     * @brief Stop network connection
     * @param net_ctx Network context
     * @return true if stopped successfully, false otherwise
     */
    bool (*stop)(void *net_ctx);
    
    /**
     * @brief Get connection state
     * @param net_ctx Network context
     * @return Current network state
     */
    network_state_t (*get_state)(void *net_ctx);
    
    /**
     * @brief Get IP address
     * @param net_ctx Network context
     * @param ip_str Buffer to store IP address string (minimum 16 bytes)
     * @return true if IP address is available, false otherwise
     */
    bool (*get_ip_address)(void *net_ctx, char *ip_str);
    
    /**
     * @brief Get MAC address
     * @param net_ctx Network context
     * @param mac_str Buffer to store MAC address string (minimum 18 bytes)
     * @return true if MAC address is available, false otherwise
     */
    bool (*get_mac_address)(void *net_ctx, char *mac_str);
    
    /**
     * @brief Set power save mode
     * @param net_ctx Network context
     * @param enabled true to enable power save, false to disable
     * @return true if set successfully, false otherwise
     */
    bool (*set_power_save)(void *net_ctx, bool enabled);
    
    /**
     * @brief Destroy network interface
     * @param net_ctx Network context
     */
    void (*destroy)(void *net_ctx);
    
    /**
     * @brief Network context data
     */
    void *ctx;
} network_interface_t;

/**
 * @brief WiFi-specific interface structure (extends network_interface)
 */
typedef struct wifi_interface {
    network_interface_t base;  // Base network interface
    
    /**
     * @brief Get current SSID
     * @param wifi_ctx WiFi context
     * @param ssid_str Buffer to store SSID string (minimum 33 bytes)
     * @return true if SSID is available, false otherwise
     */
    bool (*get_ssid)(void *wifi_ctx, char *ssid_str);
    
    /**
     * @brief Get signal strength (RSSI)
     * @param wifi_ctx WiFi context
     * @return RSSI value in dBm, or 0 if not available
     */
    int8_t (*get_rssi)(void *wifi_ctx);
    
    /**
     * @brief Get WiFi channel
     * @param wifi_ctx WiFi context
     * @return Channel number, or 0 if not available
     */
    uint8_t (*get_channel)(void *wifi_ctx);
    
    /**
     * @brief Get signal level
     * @param wifi_ctx WiFi context
     * @return Signal strength level
     */
    wifi_signal_level_t (*get_signal_level)(void *wifi_ctx);
    
    /**
     * @brief Reset WiFi configuration (enter config mode)
     * @param wifi_ctx WiFi context
     * @return true if reset successfully, false otherwise
     */
    bool (*reset_config)(void *wifi_ctx);
    
    /**
     * @brief Check if in configuration mode
     * @param wifi_ctx WiFi context
     * @return true if in config mode, false otherwise
     */
    bool (*is_config_mode)(void *wifi_ctx);
} wifi_interface_t;

/**
 * @brief Initialize a network interface
 * @param net Network interface structure
 * @param ctx Network context data
 * @param get_type Function pointer for get_type
 * @param start Function pointer for start
 * @param stop Function pointer for stop
 * @param get_state Function pointer for get_state
 * @param get_ip_address Function pointer for get_ip_address
 * @param get_mac_address Function pointer for get_mac_address
 * @param set_power_save Function pointer for set_power_save
 * @param destroy Function pointer for destroy
 */
void network_interface_init(network_interface_t *net,
                           void *ctx,
                           network_type_t (*get_type)(void *net_ctx),
                           bool (*start)(void *net_ctx),
                           bool (*stop)(void *net_ctx),
                           network_state_t (*get_state)(void *net_ctx),
                           bool (*get_ip_address)(void *net_ctx, char *ip_str),
                           bool (*get_mac_address)(void *net_ctx, char *mac_str),
                           bool (*set_power_save)(void *net_ctx, bool enabled),
                           void (*destroy)(void *net_ctx));

/**
 * @brief Initialize a WiFi interface
 * @param wifi WiFi interface structure
 * @param ctx WiFi context data
 * @param base_funcs Base network interface functions
 * @param get_ssid Function pointer for get_ssid
 * @param get_rssi Function pointer for get_rssi
 * @param get_channel Function pointer for get_channel
 * @param get_signal_level Function pointer for get_signal_level
 * @param reset_config Function pointer for reset_config
 * @param is_config_mode Function pointer for is_config_mode
 */
void wifi_interface_init(wifi_interface_t *wifi,
                        void *ctx,
                        network_interface_t *base_funcs,
                        bool (*get_ssid)(void *wifi_ctx, char *ssid_str),
                        int8_t (*get_rssi)(void *wifi_ctx),
                        uint8_t (*get_channel)(void *wifi_ctx),
                        wifi_signal_level_t (*get_signal_level)(void *wifi_ctx),
                        bool (*reset_config)(void *wifi_ctx),
                        bool (*is_config_mode)(void *wifi_ctx));

/**
 * @brief Get signal level from RSSI value
 * @param rssi RSSI value in dBm
 * @return Signal strength level
 */
wifi_signal_level_t wifi_rssi_to_signal_level(int8_t rssi);

/**
 * @brief Get network state icon name
 * @param net Network interface
 * @return Icon name string
 */
const char* network_get_state_icon(network_interface_t *net);

/**
 * @brief Get WiFi state icon name
 * @param wifi WiFi interface
 * @return Icon name string
 */
const char* wifi_get_state_icon(wifi_interface_t *wifi);

#ifdef __cplusplus
}
#endif

#endif // _NETWORK_H_