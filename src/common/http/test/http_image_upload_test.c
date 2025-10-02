#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "../http_client.h"

// 配置信息
#define VISION_URL "http://xrobo-io.qiniuapi.com/mcp/vision/explain"
#define VISION_TOKEN "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJkYXRhIjoicmRicU1Vc19JR3h0el8waE1TbGgtaE1tXy1vejd5aXROTVF1d2U1X0VEbWpqa2JhbmVVT24wQjJSalJKQXEzZ29CTjhsUnk5VHRQUTVlZlR2Y2NPVllVSXRnOG1NR0RFMmZDelZSSmJNRXJqU09mSFE3TzkifQ.3bFqulYi2r7ASpXGrUTVnVC5EFCMaltZVUhomgm29Ro"
#define IMAGE_FILE "2.jpg"
#define BOUNDARY "----ESP32_CAMERA_BOUNDARY"
#define CHUNK_SIZE 4096  // 分块大小
#define HTTP_TIMEOUT_MS 10000  // HTTP超时时间（毫秒）

/**
 * @brief 读取文件内容到内存
 * @param filename 文件名
 * @param size 返回文件大小
 * @return 文件内容指针，需要调用者释放
 */
static char* read_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 分配内存并读取文件
    char* buffer = malloc(*size);
    if (!buffer) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(buffer, 1, *size, file);
    fclose(file);
    
    if (read_size != *size) {
        printf("Error: Failed to read complete file\n");
        free(buffer);
        return NULL;
    }
    
    return buffer;
}

/**
 * @brief 分块发送图片数据
 * @param client HTTP客户端
 * @param image_data 图片数据
 * @param image_size 图片大小
 * @return 成功返回发送的总字节数，失败返回-1
 */
static size_t send_image_chunks(http_client_t* client, const char* image_data, size_t image_size) {
    size_t total_sent = 0;
    size_t remaining = image_size;
    const char* ptr = image_data;
    
    while (remaining > 0) {
        size_t chunk_size = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        
        if (http_client_write(client, ptr, chunk_size) != HTTP_CLIENT_OK) {
            printf("Error: Failed to send image chunk\n");
            return (size_t)-1;
        }
        
        ptr += chunk_size;
        remaining -= chunk_size;
        total_sent += chunk_size;
        
        // 显示进度
        if (total_sent % (CHUNK_SIZE * 4) == 0 || remaining == 0) {
            printf("Sent %zu/%zu bytes (%.1f%%)\n", 
                   total_sent, image_size, 
                   (float)total_sent / image_size * 100.0);
        }
    }
    
    return total_sent;
}

/**
 * @brief 上传图片到视觉API（改进版本）
 * @param question 要问的问题
 * @return 成功返回0，失败返回负数
 */
static int upload_image(const char* question) {
    http_client_t* client = NULL;
    char* image_data = NULL;
    size_t image_size = 0;
    int result = -1;
    char* device_id = "98:a3:16:f9:d9:34";
    char* client_id = "98:a3:16:f9:d9:34";
    
    // 创建HTTP客户端
    client = http_client_create();
    if (!client) {
        printf("Error: Failed to create HTTP client\n");
        goto cleanup;
    }
    
    // 设置超时时间
    http_client_set_timeout(client, HTTP_TIMEOUT_MS);
    printf("HTTP client timeout set to %d ms (%.1f seconds)\n", HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS / 1000.0);
    
    // 读取图片文件
    image_data = read_file(IMAGE_FILE, &image_size);
    if (!image_data) {
        printf("Error: Failed to read image file\n");
        goto cleanup;
    }
    
    printf("Image file size: %zu bytes\n", image_size);
    
    // 获取动态设备信息
    
    
    printf("Device ID: %s\n", device_id);
    printf("Client ID: %s\n", client_id);
    
    // 计算完整请求体大小
    char question_part[1024];
    snprintf(question_part, sizeof(question_part),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"question\"\r\n"
        "\r\n"
        "%s\r\n",
        BOUNDARY, question);
    
    char file_header[512];
    snprintf(file_header, sizeof(file_header),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"image\"; filename=\"camera.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n"
        "\r\n",
        BOUNDARY);
    
    char multipart_end[128];
    snprintf(multipart_end, sizeof(multipart_end), "\r\n--%s--\r\n", BOUNDARY);
    
    // 计算总的Content-Length
    size_t total_length = strlen(question_part) + strlen(file_header) + image_size + strlen(multipart_end);
    
    // 设置HTTP头部
    char content_type[256];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", BOUNDARY);
    
    char content_length[64];
    snprintf(content_length, sizeof(content_length), "%zu", total_length);
    
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", VISION_TOKEN);
    
    // 设置所有必需的头部
    if (http_client_set_header(client, "Host", "xrobo-io.qiniuapi.com") != HTTP_CLIENT_OK ||
        http_client_set_header(client, "Content-Type", content_type) != HTTP_CLIENT_OK ||
        http_client_set_header(client, "Content-Length", content_length) != HTTP_CLIENT_OK ||
        http_client_set_header(client, "Authorization", auth_header) != HTTP_CLIENT_OK ||
        http_client_set_header(client, "Device-Id", device_id) != HTTP_CLIENT_OK ||
        http_client_set_header(client, "Client-Id", client_id) != HTTP_CLIENT_OK) {
        printf("Error: Failed to set HTTP headers\n");
        goto cleanup;
    }
    
    printf("Content-Length: %zu bytes\n", total_length);
    
    // 打开HTTP连接
    if (http_client_open(client, "POST", VISION_URL) != HTTP_CLIENT_OK) {
        printf("Error: Failed to open HTTP connection\n");
        goto cleanup;
    }
    
    printf("Connected to %s\n", VISION_URL);
    
    // 发送 multipart/form-data 请求体
    printf("Sending question part...\n");
    if (http_client_write(client, question_part, strlen(question_part)) != HTTP_CLIENT_OK) {
        printf("Error: Failed to send question part\n");
        goto cleanup;
    }
    
    printf("Sending file header...\n");
    if (http_client_write(client, file_header, strlen(file_header)) != HTTP_CLIENT_OK) {
        printf("Error: Failed to send file header\n");
        goto cleanup;
    }
    
    printf("Sending image data (%zu bytes)...\n", image_size);
    // 分块发送图片数据
    size_t total_sent = send_image_chunks(client, image_data, image_size);
    if (total_sent != image_size) {
        printf("Error: Failed to send complete image data\n");
        goto cleanup;
    }
    
    printf("Sending multipart end...\n");
    if (http_client_write(client, multipart_end, strlen(multipart_end)) != HTTP_CLIENT_OK) {
        printf("Error: Failed to send multipart end\n");
        goto cleanup;
    }
    
    // 结束请求
    printf("Finishing request...\n");
    if (http_client_write(client, NULL, 0) != HTTP_CLIENT_OK) {
        printf("Error: Failed to finish request\n");
        goto cleanup;
    }
    
    printf("Request sent successfully\n");
    
    // 获取响应状态码
    int status_code = http_client_get_status_code(client);
    printf("Response status code: %d\n", status_code);
    
    // if (status_code != 200) {
    //     printf("Error: Server returned status code %d\n", status_code);
    //     goto cleanup;
    // }
    
    // 读取响应内容
    char* response = http_client_read_all(client);
    if (response) {
        printf("Response:\n%s\n", response);
        free(response);
        result = 0; // 成功
    } else {
        printf("Error: Failed to read response\n");
    }
    
cleanup:
    if (image_data) {
        free(image_data);
    }
    if (client) {
        http_client_close(client);
        http_client_destroy(client);
    }
    
    return result;
}

int main(int argc, char* argv[]) {
       // 初始化日志系统
    log_config_t log_config = LOG_DEFAULT_CONFIG;
    log_config.level = LOG_LEVEL_DEBUG;  // 默认INFO级别
    log_config.enable_timestamp = true;
    log_config.enable_color = true;
    if (log_init(&log_config) != 0) {
        LOG_ERROR("日志系统初始化失败");
        return 0;
    }
    

    const char* question = "请描述这张图片的内容";
    
    // 如果命令行提供了问题，使用命令行参数
    if (argc > 1) {
        question = argv[1];
    }
    
    printf("HTTP Image Upload Test\n");
    printf("======================\n");
    printf("Image file: %s\n", IMAGE_FILE);
    printf("Vision URL: %s\n", VISION_URL);
    printf("Question: %s\n", question);
    printf("\n");
    
    // 检查图片文件是否存在
    struct stat st;
    if (stat(IMAGE_FILE, &st) != 0) {
        printf("Error: Image file %s not found\n", IMAGE_FILE);
        return 1;
    }
    
    // 执行上传
    int result = upload_image(question);
    
    if (result == 0) {
        printf("\nImage upload completed successfully!\n");
    } else {
        printf("\nImage upload failed!\n");
    }
    
    return result;
}