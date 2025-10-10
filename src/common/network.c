#include "network.h"
#include <string.h>
#include <stddef.h>

void network_interface_init(network_interface_t *net,
                           void *ctx,
                           network_type_t (*get_type)(void *net_ctx),
                           bool (*start)(void *net_ctx),
                           bool (*stop)(void *net_ctx),
                           network_state_t (*get_state)(void *net_ctx),
                           bool (*get_ip_address)(void *net_ctx, char *ip_str),
                           bool (*get_mac_address)(void *net_ctx, char *mac_str),
                           bool (*set_power_save)(void *net_ctx, bool enabled),
                           void (*destroy)(void *net_ctx)) {
    if (net == NULL) {
        return;
    }
    
    net->ctx = ctx;
    net->get_type = get_type;
    net->start = start;
    net->stop = stop;
    net->get_state = get_state;
    net->get_ip_address = get_ip_address;
    net->get_mac_address = get_mac_address;
    net->set_power_save = set_power_save;
    net->destroy = destroy;
}

void wifi_interface_init(wifi_interface_t *wifi,
                        void *ctx,
                        network_interface_t *base_funcs,
                        bool (*get_ssid)(void *wifi_ctx, char *ssid_str),
                        int8_t (*get_rssi)(void *wifi_ctx),
                        uint8_t (*get_channel)(void *wifi_ctx),
                        wifi_signal_level_t (*get_signal_level)(void *wifi_ctx),
                        bool (*reset_config)(void *wifi_ctx),
                        bool (*is_config_mode)(void *wifi_ctx)) {
    if (wifi == NULL || base_funcs == NULL) {
        return;
    }
    
    // Copy base network interface
    memcpy(&wifi->base, base_funcs, sizeof(network_interface_t));
    wifi->base.ctx = ctx;
    
    // Set WiFi-specific functions
    wifi->get_ssid = get_ssid;
    wifi->get_rssi = get_rssi;
    wifi->get_channel = get_channel;
    wifi->get_signal_level = get_signal_level;
    wifi->reset_config = reset_config;
    wifi->is_config_mode = is_config_mode;
}

wifi_signal_level_t wifi_rssi_to_signal_level(int8_t rssi) {
    if (rssi >= -50) {
        return WIFI_SIGNAL_EXCELLENT;
    } else if (rssi >= -60) {
        return WIFI_SIGNAL_GOOD;
    } else if (rssi >= -70) {
        return WIFI_SIGNAL_FAIR;
    } else if (rssi >= -80) {
        return WIFI_SIGNAL_WEAK;
    } else {
        return WIFI_SIGNAL_NONE;
    }
}

const char* network_get_state_icon(network_interface_t *net) {
    if (net == NULL) {
        return "network-offline";
    }
    
    network_state_t state = net->get_state ? net->get_state(net->ctx) : NETWORK_STATE_DISCONNECTED;
    
    switch (state) {
        case NETWORK_STATE_CONNECTED:
            return "network";
        case NETWORK_STATE_CONNECTING:
            return "network-connecting";
        case NETWORK_STATE_CONFIGURING:
            return "network-config";
        case NETWORK_STATE_ERROR:
            return "network-error";
        case NETWORK_STATE_DISCONNECTED:
        default:
            return "network-offline";
    }
}

const char* wifi_get_state_icon(wifi_interface_t *wifi) {
    if (wifi == NULL) {
        return "wifi-off";
    }
    
    // Check if in config mode
    if (wifi->is_config_mode && wifi->is_config_mode(wifi->base.ctx)) {
        return "wifi-config";
    }
    
    network_state_t state = wifi->base.get_state ? wifi->base.get_state(wifi->base.ctx) : NETWORK_STATE_DISCONNECTED;
    
    switch (state) {
        case NETWORK_STATE_CONNECTED: {
            // Get signal level for connected state
            wifi_signal_level_t level = wifi->get_signal_level ? wifi->get_signal_level(wifi->base.ctx) : WIFI_SIGNAL_NONE;
            switch (level) {
                case WIFI_SIGNAL_EXCELLENT:
                case WIFI_SIGNAL_GOOD:
                    return "wifi";
                case WIFI_SIGNAL_FAIR:
                    return "wifi-fair";
                case WIFI_SIGNAL_WEAK:
                    return "wifi-weak";
                case WIFI_SIGNAL_NONE:
                default:
                    return "wifi-off";
            }
        }
        case NETWORK_STATE_CONNECTING:
            return "wifi-connecting";
        case NETWORK_STATE_CONFIGURING:
            return "wifi-config";
        case NETWORK_STATE_ERROR:
            return "wifi-error";
        case NETWORK_STATE_DISCONNECTED:
        default:
            return "wifi-off";
    }
}