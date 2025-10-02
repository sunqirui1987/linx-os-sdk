/**
 * @file linx_ota.h
 * @brief OTA (Over-The-Air) update functionality for LinX OS SDK
 */

#ifndef LINX_OTA_H
#define LINX_OTA_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTA status codes
 */
typedef enum {
    LINX_OTA_SUCCESS = 0,
    LINX_OTA_ERROR_INIT,
    LINX_OTA_ERROR_REQUEST,
    LINX_OTA_ERROR_DOWNLOAD,
    LINX_OTA_ERROR_VERIFY,
    LINX_OTA_ERROR_APPLY,
    LINX_OTA_NO_UPDATE,
    LINX_OTA_IN_PROGRESS,
} linx_ota_status_t;

/**
 * @brief OTA configuration structure
 */
typedef struct {
    const char *ota_server_url;     /**< OTA server URL */
    const char *device_id;          /**< Device ID (MAC address) */
    const char *client_id;          /**< Client ID (UUID) */
    const char *user_agent;         /**< User agent string */
    const char *current_version;    /**< Current firmware version */
    const char *elf_sha256;         /**< SHA256 of current firmware */
    const char *board_type;         /**< Board type */
    const char *board_name;         /**< Board name */
    const char *ssid;               /**< Connected WiFi SSID */
    int rssi;                       /**< WiFi signal strength */
    const char *mac_address;        /**< MAC address */
    const char *chip_model;         /**< Chip model */
    
    // Additional fields to match JavaScript request
    const char *app_name;           /**< Application name */
    const char *compile_time;       /**< Compile time */
    const char *idf_version;        /**< IDF version */
    const char *ota_label;          /**< OTA label */
    int wifi_channel;               /**< WiFi channel */
    const char *ip_address;         /**< IP address */
    uint32_t flash_size;            /**< Flash size */
    uint32_t minimum_free_heap_size; /**< Minimum free heap size */
    
    // Chip info
    uint32_t chip_model_id;         /**< Chip model ID */
    uint32_t chip_cores;            /**< Number of cores */
    uint32_t chip_revision;         /**< Chip revision */
    uint32_t chip_features;         /**< Chip features */
    
    void (*progress_cb)(int percentage); /**< Progress callback */
} linx_ota_config_t;

/**
 * @brief OTA update information
 */
typedef struct {
    char activation_code[32];       /**< Activation code */
    char activation_message[256];   /**< Activation message */
    char websocket_url[256];        /**< WebSocket URL */
    char firmware_version[32];      /**< New firmware version */
    char firmware_url[256];         /**< Firmware download URL */
    bool update_available;          /**< Whether update is available */
} linx_ota_info_t;

/**
 * @brief Initialize OTA module
 * 
 * @param config OTA configuration
 * @return linx_ota_status_t Status code
 */
linx_ota_status_t linx_ota_init(const linx_ota_config_t *config);

/**
 * @brief Check for OTA updates
 * 
 * @param info Pointer to store OTA information
 * @return linx_ota_status_t Status code
 */
linx_ota_status_t linx_ota_check_update(linx_ota_info_t *info);

/**
 * @brief Download OTA update
 * 
 * @param info OTA information with firmware URL
 * @param download_path Path to save downloaded firmware
 * @return linx_ota_status_t Status code
 */
linx_ota_status_t linx_ota_download(const linx_ota_info_t *info, const char *download_path);

/**
 * @brief Apply OTA update
 * 
 * @param download_path Path to downloaded firmware
 * @return linx_ota_status_t Status code
 */
linx_ota_status_t linx_ota_apply(const char *download_path);

/**
 * @brief Get OTA status string
 * 
 * @param status OTA status code
 * @return const char* Status string
 */
const char *linx_ota_status_str(linx_ota_status_t status);

/**
 * @brief Cleanup OTA module
 */
void linx_ota_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* LINX_OTA_H */