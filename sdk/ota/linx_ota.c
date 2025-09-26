/**
 * @file linx_ota.c
 * @brief Implementation of OTA functionality using mongoose
 */

#include "linx_ota.h"
#include "../third/mongoose/mongoose.h"
#include "../cjson/cJSON.h"
#include "../log/linx_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OTA_TAG "LINX_OTA"

// Define LINX log macros
#define LINX_LOGE(tag, fmt, ...) LOG_ERROR("[%s] " fmt, tag, ##__VA_ARGS__)
#define LINX_LOGW(tag, fmt, ...) LOG_WARN("[%s] " fmt, tag, ##__VA_ARGS__)
#define LINX_LOGI(tag, fmt, ...) LOG_INFO("[%s] " fmt, tag, ##__VA_ARGS__)

// OTA context structure
typedef struct {
    linx_ota_config_t config;
    struct mg_mgr mgr;
    bool initialized;
    bool request_in_progress;
    bool download_in_progress;
    linx_ota_info_t info;
    char *download_buffer;
    size_t download_size;
    size_t download_received;
    void (*progress_cb)(int percentage);
} ota_ctx_t;

static ota_ctx_t s_ota_ctx = {0};

// Status strings
static const char *s_ota_status_str[] = {
    "Success",
    "Initialization error",
    "Request error",
    "Download error",
    "Verification error",
    "Apply error",
    "No update available",
    "OTA in progress"
};

// Forward declarations
static void ota_http_handler(struct mg_connection *c, int ev, void *ev_data);
static void ota_download_handler(struct mg_connection *c, int ev, void *ev_data);

const char *linx_ota_status_str(linx_ota_status_t status) {
    if (status < 0 || status >= sizeof(s_ota_status_str) / sizeof(s_ota_status_str[0])) {
        return "Unknown status";
    }
    return s_ota_status_str[status];
}

linx_ota_status_t linx_ota_init(const linx_ota_config_t *config) {
    if (config == NULL) {
        LINX_LOGE(OTA_TAG, "Invalid OTA configuration");
        return LINX_OTA_ERROR_INIT;
    }

    if (s_ota_ctx.initialized) {
        linx_ota_cleanup();
    }

    // Initialize mongoose
    mg_mgr_init(&s_ota_ctx.mgr);

    // Copy configuration
    memcpy(&s_ota_ctx.config, config, sizeof(linx_ota_config_t));
    s_ota_ctx.progress_cb = config->progress_cb;
    s_ota_ctx.initialized = true;
    s_ota_ctx.request_in_progress = false;
    s_ota_ctx.download_in_progress = false;

    LINX_LOGI(OTA_TAG, "OTA module initialized");
    return LINX_OTA_SUCCESS;
}

void linx_ota_cleanup(void) {
    if (s_ota_ctx.initialized) {
        mg_mgr_free(&s_ota_ctx.mgr);
        if (s_ota_ctx.download_buffer) {
            free(s_ota_ctx.download_buffer);
            s_ota_ctx.download_buffer = NULL;
        }
        memset(&s_ota_ctx, 0, sizeof(ota_ctx_t));
        LINX_LOGI(OTA_TAG, "OTA module cleaned up");
    }
}

linx_ota_status_t linx_ota_check_update(linx_ota_info_t *info) {
    if (!s_ota_ctx.initialized) {
        LINX_LOGE(OTA_TAG, "OTA module not initialized");
        return LINX_OTA_ERROR_INIT;
    }

    if (s_ota_ctx.request_in_progress || s_ota_ctx.download_in_progress) {
        LINX_LOGW(OTA_TAG, "OTA operation already in progress");
        return LINX_OTA_IN_PROGRESS;
    }

    // Clear previous info
    memset(&s_ota_ctx.info, 0, sizeof(linx_ota_info_t));
    s_ota_ctx.request_in_progress = true;

    // Prepare JSON request body using string concatenation - matching JavaScript request structure
    const char *app_name = s_ota_ctx.config.app_name ? s_ota_ctx.config.app_name : "xiaoniu-web-test";
    const char *compile_time = s_ota_ctx.config.compile_time ? s_ota_ctx.config.compile_time : "2025-04-16 10:00:00";
    const char *idf_version = s_ota_ctx.config.idf_version ? s_ota_ctx.config.idf_version : "4.4.3";
    const char *ota_label = s_ota_ctx.config.ota_label ? s_ota_ctx.config.ota_label : "xiaoniu-web-test";
    const char *ip_address = s_ota_ctx.config.ip_address ? s_ota_ctx.config.ip_address : "192.168.1.1";
    
    // Calculate buffer size needed
    size_t json_size = 2048; // Base size for JSON structure
    json_size += strlen(app_name) * 2; // Used in multiple places
    json_size += s_ota_ctx.config.current_version ? strlen(s_ota_ctx.config.current_version) : 0;
    json_size += strlen(compile_time);
    json_size += strlen(idf_version);
    json_size += s_ota_ctx.config.elf_sha256 ? strlen(s_ota_ctx.config.elf_sha256) : 0;
    json_size += strlen(ota_label);
    json_size += s_ota_ctx.config.board_type ? strlen(s_ota_ctx.config.board_type) : 0;
    json_size += s_ota_ctx.config.ssid ? strlen(s_ota_ctx.config.ssid) : 0;
    json_size += strlen(ip_address);
    json_size += s_ota_ctx.config.mac_address ? strlen(s_ota_ctx.config.mac_address) * 2 : 0; // Used twice
    json_size += s_ota_ctx.config.chip_model ? strlen(s_ota_ctx.config.chip_model) : 0;
    
    char *json_str = malloc(json_size);
    if (!json_str) {
        LINX_LOGE(OTA_TAG, "Failed to allocate memory for JSON request");
        s_ota_ctx.request_in_progress = false;
        return LINX_OTA_ERROR_REQUEST;
    }
    
    // Build JSON string using snprintf
    int json_len = snprintf(json_str, json_size,
        "{"
        "\"version\":0,"
        "\"uuid\":\"\","
        "\"application\":{"
            "\"name\":\"%s\","
            "\"version\":\"%s\","
            "\"compile_time\":\"%s\","
            "\"idf_version\":\"%s\","
            "\"elf_sha256\":\"%s\""
        "},"
        "\"ota\":{"
            "\"label\":\"%s\""
        "},"
        "\"board\":{"
            "\"type\":\"%s\","
            "\"ssid\":\"%s\","
            "\"rssi\":%d,"
            "\"channel\":%d,"
            "\"ip\":\"%s\","
            "\"mac\":\"%s\""
        "},"
        "\"flash_size\":%u,"
        "\"minimum_free_heap_size\":%u,"
        "\"mac_address\":\"%s\","
        "\"chip_model_name\":\"%s\","
        "\"chip_info\":{"
            "\"model\":%u,"
            "\"cores\":%u,"
            "\"revision\":%u,"
            "\"features\":%u"
        "},"
        "\"partition_table\":[{"
            "\"label\":\"\","
            "\"type\":0,"
            "\"subtype\":0,"
            "\"address\":0,"
            "\"size\":0"
        "}]"
        "}",
        app_name,
        s_ota_ctx.config.current_version ? s_ota_ctx.config.current_version : "",
        compile_time,
        idf_version,
        s_ota_ctx.config.elf_sha256 ? s_ota_ctx.config.elf_sha256 : "",
        ota_label,
        s_ota_ctx.config.board_type ? s_ota_ctx.config.board_type : "",
        s_ota_ctx.config.ssid ? s_ota_ctx.config.ssid : "",
        s_ota_ctx.config.rssi,
        s_ota_ctx.config.wifi_channel,
        ip_address,
        s_ota_ctx.config.mac_address ? s_ota_ctx.config.mac_address : "",
        s_ota_ctx.config.flash_size,
        s_ota_ctx.config.minimum_free_heap_size,
        s_ota_ctx.config.mac_address ? s_ota_ctx.config.mac_address : "",
        s_ota_ctx.config.chip_model ? s_ota_ctx.config.chip_model : "",
        s_ota_ctx.config.chip_model_id,
        s_ota_ctx.config.chip_cores,
        s_ota_ctx.config.chip_revision,
        s_ota_ctx.config.chip_features
    );

    if (json_len < 0 || json_len >= json_size) {
        LINX_LOGE(OTA_TAG, "Failed to create JSON request - buffer too small");
        free(json_str);
        s_ota_ctx.request_in_progress = false;
        return LINX_OTA_ERROR_REQUEST;
    }
    
    // Debug: Log the JSON request
    LINX_LOGI(OTA_TAG, "Sending JSON request (%d bytes): %s", json_len, json_str);

    // Create HTTP connection
    struct mg_connection *c = mg_http_connect(&s_ota_ctx.mgr, s_ota_ctx.config.ota_server_url, 
                                             ota_http_handler, NULL);
    if (c == NULL) {
        LINX_LOGE(OTA_TAG, "Failed to create HTTP connection");
        free(json_str);
        s_ota_ctx.request_in_progress = false;
        return LINX_OTA_ERROR_REQUEST;
    }
    
    // Store the info pointer in the connection's user data
    c->fn_data = info;

    // Extract hostname safely
    struct mg_str host = mg_url_host(s_ota_ctx.config.ota_server_url);
    char host_str[256] = {0};
    if (host.len > 0 && host.len < sizeof(host_str)) {
        strncpy(host_str, host.buf, host.len);
        host_str[host.len] = '\0';
    } else {
        // Fallback to a default host if extraction fails
        strncpy(host_str, "xrobo.qiniuapi.com", sizeof(host_str) - 1);
    }

    // Prepare HTTP headers - simplified to avoid 400 errors
    mg_printf(c,
              "POST /v1/ota/ HTTP/1.1\r\n"
              "Host: %s\r\n"
              "Content-Type: application/json\r\n"
              "Content-Length: %d\r\n"
              "Accept: application/json\r\n"
              "User-Agent: %s\r\n"
              "Client-Id: %s\r\n"
              "Device-Id: %s\r\n"
              "Connection: close\r\n"
              "\r\n"
              "%s",
              host_str,
              json_len,
              s_ota_ctx.config.user_agent ? s_ota_ctx.config.user_agent : "LinxOS-OTA/1.0",
              s_ota_ctx.config.client_id ? s_ota_ctx.config.client_id : "",
              s_ota_ctx.config.device_id ? s_ota_ctx.config.device_id : "",
              json_str);

    free(json_str);

    // Poll for events
    while (s_ota_ctx.request_in_progress) {
        mg_mgr_poll(&s_ota_ctx.mgr, 1000);
    }

    // Copy info if provided
    if (info) {
        memcpy(info, &s_ota_ctx.info, sizeof(linx_ota_info_t));
    }

    return s_ota_ctx.info.update_available ? LINX_OTA_SUCCESS : LINX_OTA_NO_UPDATE;
}

static void ota_http_handler(struct mg_connection *c, int ev, void *ev_data) {
    linx_ota_info_t *info = (linx_ota_info_t *)c->fn_data;
    
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        int status_code = mg_http_status(hm);
        
        if (status_code == 200) {
            // Parse JSON response
            cJSON *root = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
            if (root) {
                // Extract activation info
                cJSON *activation = cJSON_GetObjectItem(root, "activation");
                if (activation) {
                    cJSON *code = cJSON_GetObjectItem(activation, "code");
                    cJSON *message = cJSON_GetObjectItem(activation, "message");
                    
                    if (cJSON_IsString(code) && code->valuestring) {
                        strncpy(s_ota_ctx.info.activation_code, code->valuestring, 
                                sizeof(s_ota_ctx.info.activation_code) - 1);
                    }
                    
                    if (cJSON_IsString(message) && message->valuestring) {
                        strncpy(s_ota_ctx.info.activation_message, message->valuestring, 
                                sizeof(s_ota_ctx.info.activation_message) - 1);
                    }
                }
                
                // Extract websocket info
                cJSON *websocket = cJSON_GetObjectItem(root, "websocket");
                if (websocket) {
                    cJSON *url = cJSON_GetObjectItem(websocket, "url");
                    if (cJSON_IsString(url) && url->valuestring) {
                        strncpy(s_ota_ctx.info.websocket_url, url->valuestring, 
                                sizeof(s_ota_ctx.info.websocket_url) - 1);
                    }
                }
                
                // Extract firmware info
                cJSON *firmware = cJSON_GetObjectItem(root, "firmware");
                if (firmware) {
                    cJSON *version = cJSON_GetObjectItem(firmware, "version");
                    cJSON *url = cJSON_GetObjectItem(firmware, "url");
                    
                    if (cJSON_IsString(version) && version->valuestring) {
                        strncpy(s_ota_ctx.info.firmware_version, version->valuestring, 
                                sizeof(s_ota_ctx.info.firmware_version) - 1);
                    }
                    
                    if (cJSON_IsString(url) && url->valuestring) {
                        strncpy(s_ota_ctx.info.firmware_url, url->valuestring, 
                                sizeof(s_ota_ctx.info.firmware_url) - 1);
                        s_ota_ctx.info.update_available = true;
                    }
                }
                
                cJSON_Delete(root);
                
                LINX_LOGI(OTA_TAG, "OTA check completed, update available: %d", 
                         s_ota_ctx.info.update_available);
                
                if (info) {
                    memcpy(info, &s_ota_ctx.info, sizeof(linx_ota_info_t));
                }
            } else {
                LINX_LOGE(OTA_TAG, "Failed to parse OTA response");
            }
        } else {
            // Log the response body for debugging
            char response_body[1024] = {0};
            size_t body_len = hm->body.len < sizeof(response_body) - 1 ? hm->body.len : sizeof(response_body) - 1;
            if (body_len > 0) {
                strncpy(response_body, hm->body.buf, body_len);
                response_body[body_len] = '\0';
            }
            LINX_LOGE(OTA_TAG, "OTA check failed with status code: %d, response: %s", status_code, response_body);
        }
        
        s_ota_ctx.request_in_progress = false;
        c->is_closing = 1;
    } else if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
        if (s_ota_ctx.request_in_progress) {
            LINX_LOGE(OTA_TAG, "OTA check connection error or closed");
            s_ota_ctx.request_in_progress = false;
        }
    }
}

linx_ota_status_t linx_ota_download(const linx_ota_info_t *info, const char *download_path) {
    if (!s_ota_ctx.initialized) {
        LINX_LOGE(OTA_TAG, "OTA module not initialized");
        return LINX_OTA_ERROR_INIT;
    }

    if (s_ota_ctx.request_in_progress || s_ota_ctx.download_in_progress) {
        LINX_LOGW(OTA_TAG, "OTA operation already in progress");
        return LINX_OTA_IN_PROGRESS;
    }

    if (!info || !info->update_available || !info->firmware_url[0]) {
        LINX_LOGE(OTA_TAG, "No firmware URL available for download");
        return LINX_OTA_ERROR_DOWNLOAD;
    }

    // Initialize download context
    s_ota_ctx.download_in_progress = true;
    s_ota_ctx.download_buffer = NULL;
    s_ota_ctx.download_size = 0;
    s_ota_ctx.download_received = 0;

    // Create HTTP connection for download
    struct mg_connection *c = mg_http_connect(&s_ota_ctx.mgr, info->firmware_url, 
                                             ota_download_handler, NULL);
    if (c == NULL) {
        LINX_LOGE(OTA_TAG, "Failed to create download connection");
        s_ota_ctx.download_in_progress = false;
        return LINX_OTA_ERROR_DOWNLOAD;
    }
    
    // Store the download path in a dynamically allocated string to ensure it remains valid
    if (download_path) {
        c->fn_data = strdup(download_path);
    }

    // Extract URI and hostname safely
    const char *uri = mg_url_uri(info->firmware_url);
    struct mg_str host = mg_url_host(info->firmware_url);
    
    char uri_str[512] = {0};
    char host_str[256] = {0};
    
    if (uri && strlen(uri) < sizeof(uri_str)) {
        strncpy(uri_str, uri, strlen(uri));
        uri_str[strlen(uri)] = '\0';
    } else {
        strncpy(uri_str, "/", sizeof(uri_str) - 1);
    }
    
    if (host.len > 0 && host.len < sizeof(host_str)) {
        strncpy(host_str, host.buf, host.len);
        host_str[host.len] = '\0';
    } else {
        strncpy(host_str, "localhost", sizeof(host_str) - 1);
    }

    // Send HTTP GET request
    mg_printf(c, "GET %s HTTP/1.1\r\n"
                "Host: %s\r\n"
                "User-Agent: %s\r\n"
                "Connection: close\r\n"
                "\r\n",
                uri_str,
                host_str,
                s_ota_ctx.config.user_agent);

    // Poll for events
    while (s_ota_ctx.download_in_progress) {
        mg_mgr_poll(&s_ota_ctx.mgr, 1000);
    }

    // Check if download was successful
    if (s_ota_ctx.download_buffer && s_ota_ctx.download_size > 0) {
        // Save to file
        FILE *fp = fopen(download_path, "wb");
        if (fp) {
            size_t written = fwrite(s_ota_ctx.download_buffer, 1, s_ota_ctx.download_size, fp);
            fclose(fp);
            
            if (written == s_ota_ctx.download_size) {
                LINX_LOGI(OTA_TAG, "Firmware downloaded successfully to %s (%zu bytes)", 
                         download_path, s_ota_ctx.download_size);
                free(s_ota_ctx.download_buffer);
                s_ota_ctx.download_buffer = NULL;
                return LINX_OTA_SUCCESS;
            } else {
                LINX_LOGE(OTA_TAG, "Failed to write firmware to file");
            }
        } else {
            LINX_LOGE(OTA_TAG, "Failed to open download file for writing");
        }
        
        free(s_ota_ctx.download_buffer);
        s_ota_ctx.download_buffer = NULL;
        return LINX_OTA_ERROR_DOWNLOAD;
    }

    return LINX_OTA_ERROR_DOWNLOAD;
}

static void ota_download_handler(struct mg_connection *c, int ev, void *ev_data) {
    const char *download_path = c->fn_data ? (const char *)c->fn_data : NULL;
    
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        int status_code = mg_http_status(hm);
        
        if (status_code == 200) {
            // Get content length
            struct mg_str *cl_header = mg_http_get_header(hm, "Content-Length");
            if (cl_header) {
                s_ota_ctx.download_size = strtoul(cl_header->buf, NULL, 10);
                LINX_LOGI(OTA_TAG, "Firmware size: %zu bytes", s_ota_ctx.download_size);
                
                // Allocate buffer for the firmware
                s_ota_ctx.download_buffer = malloc(s_ota_ctx.download_size);
                if (!s_ota_ctx.download_buffer) {
                    LINX_LOGE(OTA_TAG, "Failed to allocate memory for firmware download");
                    s_ota_ctx.download_in_progress = false;
                    c->is_closing = 1;
                    return;
                }
                
                // Copy body data
                memcpy(s_ota_ctx.download_buffer, hm->body.buf, hm->body.len);
                s_ota_ctx.download_received = hm->body.len;
                
                // Report progress
                if (s_ota_ctx.progress_cb) {
                    int percentage = (int)((s_ota_ctx.download_received * 100) / s_ota_ctx.download_size);
                    s_ota_ctx.progress_cb(percentage);
                }
                
                // Check if we got all data in one chunk
                if (s_ota_ctx.download_received >= s_ota_ctx.download_size) {
                    LINX_LOGI(OTA_TAG, "Firmware download completed in one chunk");
                    s_ota_ctx.download_in_progress = false;
                    c->is_closing = 1;
                }
            } else {
                LINX_LOGE(OTA_TAG, "Content-Length header not found");
                s_ota_ctx.download_in_progress = false;
                c->is_closing = 1;
            }
        } else {
            LINX_LOGE(OTA_TAG, "Firmware download failed with status code: %d", status_code);
            s_ota_ctx.download_in_progress = false;
            c->is_closing = 1;
        }
    } else if (ev == MG_EV_READ) {
        long *bytes_read = (long *) ev_data;
        
        // Append data to buffer
        if (s_ota_ctx.download_buffer && 
            s_ota_ctx.download_received + *bytes_read <= s_ota_ctx.download_size) {
            // For MG_EV_READ, we need to read from the connection's receive buffer
            // The data is already in the connection's receive buffer
            size_t bytes_to_copy = *bytes_read;
            memcpy(s_ota_ctx.download_buffer + s_ota_ctx.download_received, 
                   c->recv.buf, bytes_to_copy);
            s_ota_ctx.download_received += bytes_to_copy;
            
            // Mark the data as consumed
            mg_iobuf_del(&c->recv, 0, bytes_to_copy);
            
            // Report progress
            if (s_ota_ctx.progress_cb) {
                int percentage = (int)((s_ota_ctx.download_received * 100) / s_ota_ctx.download_size);
                s_ota_ctx.progress_cb(percentage);
            }
            
            LINX_LOGI(OTA_TAG, "Downloaded %zu of %zu bytes (%d%%)", 
                     s_ota_ctx.download_received, s_ota_ctx.download_size,
                     (int)((s_ota_ctx.download_received * 100) / s_ota_ctx.download_size));
            
            // Check if download is complete
            if (s_ota_ctx.download_received >= s_ota_ctx.download_size) {
                LINX_LOGI(OTA_TAG, "Firmware download completed");
                s_ota_ctx.download_in_progress = false;
                c->is_closing = 1;
            }
        }
    } else if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
        if (s_ota_ctx.download_in_progress) {
            if (s_ota_ctx.download_buffer && 
                s_ota_ctx.download_received == s_ota_ctx.download_size) {
                LINX_LOGI(OTA_TAG, "Firmware download completed on connection close");
            } else {
                LINX_LOGE(OTA_TAG, "Firmware download failed or connection closed prematurely");
                if (s_ota_ctx.download_buffer) {
                    free(s_ota_ctx.download_buffer);
                    s_ota_ctx.download_buffer = NULL;
                }
            }
            s_ota_ctx.download_in_progress = false;
        }
        
        // Free the allocated download path
        if (c->fn_data) {
            free(c->fn_data);
            c->fn_data = NULL;
        }
    }
}

linx_ota_status_t linx_ota_apply(const char *download_path) {
    if (!s_ota_ctx.initialized) {
        LINX_LOGE(OTA_TAG, "OTA module not initialized");
        return LINX_OTA_ERROR_INIT;
    }

    if (!download_path) {
        LINX_LOGE(OTA_TAG, "Invalid download path");
        return LINX_OTA_ERROR_APPLY;
    }

    // Check if file exists
    FILE *fp = fopen(download_path, "rb");
    if (!fp) {
        LINX_LOGE(OTA_TAG, "Firmware file not found: %s", download_path);
        return LINX_OTA_ERROR_APPLY;
    }
    fclose(fp);

    LINX_LOGI(OTA_TAG, "Applying firmware update from %s", download_path);

    // Platform-specific implementation for applying the update
    // This is a placeholder - actual implementation depends on the target platform
    #if defined(__APPLE__) || defined(__linux__)
    // For desktop platforms, we just simulate the update
    LINX_LOGI(OTA_TAG, "Simulating firmware update on desktop platform");
    return LINX_OTA_SUCCESS;
    #else
    // For embedded platforms, implement the actual update mechanism
    // This would typically involve:
    // 1. Verifying the firmware image
    // 2. Writing to a staging area or directly to flash
    // 3. Setting up for reboot to apply the update
    LINX_LOGE(OTA_TAG, "OTA apply not implemented for this platform");
    return LINX_OTA_ERROR_APPLY;
    #endif
}