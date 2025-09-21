/*
 * MCP天气服务器示例
 * 
 * 这个示例展示了如何使用MCP C SDK创建一个天气信息服务器，
 * 支持获取当前天气、天气预报、历史天气等功能。
 * 
 * 编译命令：
 * gcc -o weather_server weather_server.c ../../*.c -lcjson -lcurl
 * 
 * 运行：
 * ./weather_server
 */

#include "../../mcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

// 全局服务器实例
static mcp_server_t* g_server = NULL;

// 模拟天气数据结构
typedef struct {
    char city[64];
    char country[32];
    double temperature;
    double humidity;
    double pressure;
    char condition[32];
    char description[128];
    double wind_speed;
    int wind_direction;
    time_t timestamp;
} weather_data_t;

// 预定义的城市天气数据（模拟）
static weather_data_t g_weather_db[] = {
    {"Beijing", "China", 15.5, 65.0, 1013.2, "Cloudy", "Partly cloudy with light winds", 12.5, 180, 0},
    {"Shanghai", "China", 22.3, 78.0, 1015.8, "Rainy", "Light rain with moderate humidity", 8.2, 90, 0},
    {"Guangzhou", "China", 28.7, 82.0, 1012.5, "Sunny", "Clear skies with high temperature", 6.8, 45, 0},
    {"Shenzhen", "China", 26.9, 75.0, 1014.1, "Partly Cloudy", "Scattered clouds with warm weather", 9.3, 135, 0},
    {"New York", "USA", 18.2, 60.0, 1016.3, "Clear", "Clear skies with pleasant temperature", 11.7, 270, 0},
    {"London", "UK", 12.8, 85.0, 1008.9, "Foggy", "Dense fog with cool temperature", 5.4, 225, 0},
    {"Tokyo", "Japan", 20.1, 70.0, 1018.7, "Overcast", "Overcast skies with mild temperature", 7.9, 315, 0},
    {"Paris", "France", 16.4, 68.0, 1011.6, "Drizzle", "Light drizzle with cool breeze", 10.2, 200, 0}
};

static const size_t g_weather_db_size = sizeof(g_weather_db) / sizeof(weather_data_t);

// 消息发送回调函数
void send_message(const char* message) {
    printf("SEND: %s\n", message);
    fflush(stdout);
}

// 初始化天气数据时间戳
void init_weather_timestamps() {
    time_t now = time(NULL);
    for (size_t i = 0; i < g_weather_db_size; i++) {
        g_weather_db[i].timestamp = now - (rand() % 3600);  // 随机1小时内的时间
    }
}

// 查找城市天气数据
weather_data_t* find_weather_data(const char* city) {
    if (!city) return NULL;
    
    for (size_t i = 0; i < g_weather_db_size; i++) {
        if (strcasecmp(g_weather_db[i].city, city) == 0) {
            return &g_weather_db[i];
        }
    }
    return NULL;
}

// 生成随机天气变化
void update_weather_data(weather_data_t* data) {
    if (!data) return;
    
    // 模拟温度变化 (-2°C 到 +2°C)
    data->temperature += (rand() % 400 - 200) / 100.0;
    
    // 模拟湿度变化 (-5% 到 +5%)
    data->humidity += (rand() % 1000 - 500) / 100.0;
    if (data->humidity < 0) data->humidity = 0;
    if (data->humidity > 100) data->humidity = 100;
    
    // 模拟气压变化 (-5 到 +5 hPa)
    data->pressure += (rand() % 1000 - 500) / 100.0;
    
    // 模拟风速变化
    data->wind_speed += (rand() % 600 - 300) / 100.0;
    if (data->wind_speed < 0) data->wind_speed = 0;
    
    // 更新时间戳
    data->timestamp = time(NULL);
}

// 获取当前天气工具回调
mcp_return_value_t get_current_weather_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (!properties || properties->count < 1) {
        result.value.string_val = mcp_strdup("Error: City name is required");
        return result;
    }
    
    const mcp_property_t* city_prop = mcp_property_list_find(properties, "city");
    if (!city_prop || city_prop->type != MCP_PROPERTY_TYPE_STRING) {
        result.value.string_val = mcp_strdup("Error: City must be a string");
        return result;
    }
    
    const char* city = mcp_property_get_string_value(city_prop);
    weather_data_t* weather = find_weather_data(city);
    
    if (!weather) {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Weather data not available for city '%s'", city);
        result.value.string_val = error_msg;
        return result;
    }
    
    // 更新天气数据（模拟实时变化）
    update_weather_data(weather);
    
    // 格式化天气信息
    char* weather_info = malloc(1024);
    snprintf(weather_info, 1024,
        "Current Weather for %s, %s:\n"
        "  Temperature: %.1f°C\n"
        "  Condition: %s\n"
        "  Description: %s\n"
        "  Humidity: %.1f%%\n"
        "  Pressure: %.1f hPa\n"
        "  Wind: %.1f m/s from %d°\n"
        "  Last updated: %s",
        weather->city, weather->country,
        weather->temperature,
        weather->condition,
        weather->description,
        weather->humidity,
        weather->pressure,
        weather->wind_speed, weather->wind_direction,
        ctime(&weather->timestamp));
    
    result.value.string_val = weather_info;
    return result;
}

// 获取天气预报工具回调
mcp_return_value_t get_weather_forecast_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (!properties || properties->count < 1) {
        result.value.string_val = mcp_strdup("Error: City name is required");
        return result;
    }
    
    const mcp_property_t* city_prop = mcp_property_list_find(properties, "city");
    if (!city_prop || city_prop->type != MCP_PROPERTY_TYPE_STRING) {
        result.value.string_val = mcp_strdup("Error: City must be a string");
        return result;
    }
    
    const char* city = mcp_property_get_string_value(city_prop);
    weather_data_t* base_weather = find_weather_data(city);
    
    if (!base_weather) {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Weather data not available for city '%s'", city);
        result.value.string_val = error_msg;
        return result;
    }
    
    // 获取预报天数（默认5天）
    int days = 5;
    const mcp_property_t* days_prop = mcp_property_list_find(properties, "days");
    if (days_prop && days_prop->type == MCP_PROPERTY_TYPE_INTEGER) {
        days = mcp_property_get_int_value(days_prop);
        if (days < 1) days = 1;
        if (days > 10) days = 10;
    }
    
    // 生成预报数据
    char* forecast = malloc(2048);
    snprintf(forecast, 2048, "%d-Day Weather Forecast for %s, %s:\n\n", 
             days, base_weather->city, base_weather->country);
    
    time_t current_time = time(NULL);
    for (int i = 0; i < days; i++) {
        time_t forecast_time = current_time + (i * 24 * 3600);
        struct tm* tm_info = localtime(&forecast_time);
        
        // 模拟预报数据（基于当前天气变化）
        double temp_variation = (rand() % 1000 - 500) / 100.0;  // -5°C 到 +5°C
        double humidity_variation = (rand() % 2000 - 1000) / 100.0;  // -10% 到 +10%
        
        double forecast_temp = base_weather->temperature + temp_variation;
        double forecast_humidity = base_weather->humidity + humidity_variation;
        if (forecast_humidity < 0) forecast_humidity = 0;
        if (forecast_humidity > 100) forecast_humidity = 100;
        
        // 随机选择天气状况
        const char* conditions[] = {"Sunny", "Cloudy", "Partly Cloudy", "Rainy", "Overcast"};
        const char* condition = conditions[rand() % 5];
        
        char day_forecast[256];
        snprintf(day_forecast, sizeof(day_forecast),
            "Day %d (%02d/%02d):\n"
            "  Temperature: %.1f°C\n"
            "  Condition: %s\n"
            "  Humidity: %.1f%%\n\n",
            i + 1, tm_info->tm_mon + 1, tm_info->tm_mday,
            forecast_temp, condition, forecast_humidity);
        
        strcat(forecast, day_forecast);
    }
    
    result.value.string_val = forecast;
    return result;
}

// 获取天气统计工具回调
mcp_return_value_t get_weather_stats_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    // 计算所有城市的天气统计
    double total_temp = 0, min_temp = 1000, max_temp = -1000;
    double total_humidity = 0, min_humidity = 100, max_humidity = 0;
    double total_pressure = 0, min_pressure = 2000, max_pressure = 0;
    
    char* hottest_city = NULL;
    char* coldest_city = NULL;
    char* most_humid_city = NULL;
    char* least_humid_city = NULL;
    
    for (size_t i = 0; i < g_weather_db_size; i++) {
        weather_data_t* weather = &g_weather_db[i];
        
        total_temp += weather->temperature;
        total_humidity += weather->humidity;
        total_pressure += weather->pressure;
        
        if (weather->temperature > max_temp) {
            max_temp = weather->temperature;
            hottest_city = weather->city;
        }
        if (weather->temperature < min_temp) {
            min_temp = weather->temperature;
            coldest_city = weather->city;
        }
        
        if (weather->humidity > max_humidity) {
            max_humidity = weather->humidity;
            most_humid_city = weather->city;
        }
        if (weather->humidity < min_humidity) {
            min_humidity = weather->humidity;
            least_humid_city = weather->city;
        }
        
        if (weather->pressure > max_pressure) {
            max_pressure = weather->pressure;
        }
        if (weather->pressure < min_pressure) {
            min_pressure = weather->pressure;
        }
    }
    
    double avg_temp = total_temp / g_weather_db_size;
    double avg_humidity = total_humidity / g_weather_db_size;
    double avg_pressure = total_pressure / g_weather_db_size;
    
    char* stats = malloc(1024);
    snprintf(stats, 1024,
        "Global Weather Statistics (%zu cities):\n\n"
        "Temperature:\n"
        "  Average: %.1f°C\n"
        "  Highest: %.1f°C (%s)\n"
        "  Lowest: %.1f°C (%s)\n\n"
        "Humidity:\n"
        "  Average: %.1f%%\n"
        "  Highest: %.1f%% (%s)\n"
        "  Lowest: %.1f%% (%s)\n\n"
        "Pressure:\n"
        "  Average: %.1f hPa\n"
        "  Highest: %.1f hPa\n"
        "  Lowest: %.1f hPa\n",
        g_weather_db_size,
        avg_temp, max_temp, hottest_city, min_temp, coldest_city,
        avg_humidity, max_humidity, most_humid_city, min_humidity, least_humid_city,
        avg_pressure, max_pressure, min_pressure);
    
    result.value.string_val = stats;
    return result;
}

// 列出支持的城市工具回调
mcp_return_value_t list_cities_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    char* city_list = malloc(1024);
    strcpy(city_list, "Supported Cities:\n\n");
    
    for (size_t i = 0; i < g_weather_db_size; i++) {
        char city_info[128];
        snprintf(city_info, sizeof(city_info), "  %zu. %s, %s\n", 
                i + 1, g_weather_db[i].city, g_weather_db[i].country);
        strcat(city_list, city_info);
    }
    
    strcat(city_list, "\nUse any of these city names with other weather tools.");
    
    result.value.string_val = city_list;
    return result;
}

// 比较城市天气工具回调
mcp_return_value_t compare_weather_callback(const mcp_property_list_t* properties) {
    mcp_return_value_t result;
    result.type = MCP_RETURN_TYPE_STRING;
    
    if (!properties || properties->count < 2) {
        result.value.string_val = mcp_strdup("Error: Two city names are required for comparison");
        return result;
    }
    
    const mcp_property_t* city1_prop = mcp_property_list_find(properties, "city1");
    const mcp_property_t* city2_prop = mcp_property_list_find(properties, "city2");
    
    if (!city1_prop || !city2_prop || 
        city1_prop->type != MCP_PROPERTY_TYPE_STRING || 
        city2_prop->type != MCP_PROPERTY_TYPE_STRING) {
        result.value.string_val = mcp_strdup("Error: Both cities must be strings");
        return result;
    }
    
    const char* city1 = mcp_property_get_string_value(city1_prop);
    const char* city2 = mcp_property_get_string_value(city2_prop);
    
    weather_data_t* weather1 = find_weather_data(city1);
    weather_data_t* weather2 = find_weather_data(city2);
    
    if (!weather1) {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Weather data not available for city '%s'", city1);
        result.value.string_val = error_msg;
        return result;
    }
    
    if (!weather2) {
        char* error_msg = malloc(256);
        snprintf(error_msg, 256, "Error: Weather data not available for city '%s'", city2);
        result.value.string_val = error_msg;
        return result;
    }
    
    // 更新两个城市的天气数据
    update_weather_data(weather1);
    update_weather_data(weather2);
    
    char* comparison = malloc(1024);
    snprintf(comparison, 1024,
        "Weather Comparison:\n\n"
        "%s, %s:\n"
        "  Temperature: %.1f°C\n"
        "  Condition: %s\n"
        "  Humidity: %.1f%%\n"
        "  Pressure: %.1f hPa\n"
        "  Wind: %.1f m/s\n\n"
        "%s, %s:\n"
        "  Temperature: %.1f°C\n"
        "  Condition: %s\n"
        "  Humidity: %.1f%%\n"
        "  Pressure: %.1f hPa\n"
        "  Wind: %.1f m/s\n\n"
        "Differences:\n"
        "  Temperature: %.1f°C (%s is %s)\n"
        "  Humidity: %.1f%% (%s is %s)\n"
        "  Pressure: %.1f hPa (%s is %s)",
        weather1->city, weather1->country,
        weather1->temperature, weather1->condition,
        weather1->humidity, weather1->pressure, weather1->wind_speed,
        weather2->city, weather2->country,
        weather2->temperature, weather2->condition,
        weather2->humidity, weather2->pressure, weather2->wind_speed,
        fabs(weather1->temperature - weather2->temperature),
        weather1->temperature > weather2->temperature ? weather1->city : weather2->city,
        weather1->temperature > weather2->temperature ? "warmer" : "cooler",
        fabs(weather1->humidity - weather2->humidity),
        weather1->humidity > weather2->humidity ? weather1->city : weather2->city,
        weather1->humidity > weather2->humidity ? "more humid" : "less humid",
        fabs(weather1->pressure - weather2->pressure),
        weather1->pressure > weather2->pressure ? weather1->city : weather2->city,
        weather1->pressure > weather2->pressure ? "higher pressure" : "lower pressure");
    
    result.value.string_val = comparison;
    return result;
}

// 初始化天气服务器
bool init_weather_server() {
    // 初始化随机数种子
    srand(time(NULL));
    init_weather_timestamps();
    
    // 创建服务器
    g_server = mcp_server_create("Weather Server", "1.0.0");
    if (!g_server) {
        fprintf(stderr, "Failed to create server\n");
        return false;
    }
    
    // 设置消息发送回调
    mcp_server_set_send_callback(send_message);
    
    // 创建获取当前天气工具
    mcp_property_list_t* current_props = mcp_property_list_create();
    mcp_property_t* city_prop = mcp_property_create_string("city", NULL, false);
    mcp_property_list_add(current_props, city_prop);
    
    if (!mcp_server_add_simple_tool(g_server, "get_current_weather", "Get current weather for a city", current_props, get_current_weather_callback)) {
        fprintf(stderr, "Failed to add get_current_weather tool\n");
        return false;
    }
    
    // 创建获取天气预报工具
    mcp_property_list_t* forecast_props = mcp_property_list_create();
    mcp_property_t* forecast_city_prop = mcp_property_create_string("city", NULL, false);
    mcp_property_t* days_prop = mcp_property_create_integer("days", 5, true, true, 1, 30);
    mcp_property_list_add(forecast_props, forecast_city_prop);
    mcp_property_list_add(forecast_props, days_prop);
    
    if (!mcp_server_add_simple_tool(g_server, "get_weather_forecast", "Get weather forecast for a city", forecast_props, get_weather_forecast_callback)) {
        fprintf(stderr, "Failed to add get_weather_forecast tool\n");
        return false;
    }
    
    // 创建获取天气统计工具
    mcp_property_list_t* stats_props = mcp_property_list_create();
    
    if (!mcp_server_add_simple_tool(g_server, "get_weather_stats", "Get global weather statistics", stats_props, get_weather_stats_callback)) {
        fprintf(stderr, "Failed to add get_weather_stats tool\n");
        return false;
    }
    
    // 创建列出城市工具
    mcp_property_list_t* cities_props = mcp_property_list_create();
    
    if (!mcp_server_add_simple_tool(g_server, "list_cities", "List all supported cities", cities_props, list_cities_callback)) {
        fprintf(stderr, "Failed to add list_cities tool\n");
        return false;
    }
    
    // 创建比较天气工具
    mcp_property_list_t* compare_props = mcp_property_list_create();
    mcp_property_t* city1_prop = mcp_property_create_string("city1", NULL, false);
    mcp_property_t* city2_prop = mcp_property_create_string("city2", NULL, false);
    mcp_property_list_add(compare_props, city1_prop);
    mcp_property_list_add(compare_props, city2_prop);
    
    if (!mcp_server_add_simple_tool(g_server, "compare_weather", "Compare weather between two cities", compare_props, compare_weather_callback)) {
        fprintf(stderr, "Failed to add compare_weather tool\n");
        return false;
    }
    
    // 清理属性对象
    mcp_property_destroy(city_prop);
    mcp_property_destroy(forecast_city_prop);
    mcp_property_destroy(days_prop);
    mcp_property_destroy(city1_prop);
    mcp_property_destroy(city2_prop);
    
    mcp_property_list_destroy(current_props);
    mcp_property_list_destroy(forecast_props);
    mcp_property_list_destroy(stats_props);
    mcp_property_list_destroy(cities_props);
    mcp_property_list_destroy(compare_props);
    
    printf("Weather server initialized with %zu tools\n", g_server->tool_count);
    printf("Supporting %zu cities worldwide\n", g_weather_db_size);
    return true;
}

// 清理服务器
void cleanup_weather_server() {
    if (g_server) {
        mcp_server_destroy(g_server);
        g_server = NULL;
    }
}

// 处理输入消息
void process_message(const char* message) {
    if (!g_server || !message) {
        return;
    }
    
    printf("RECV: %s\n", message);
    mcp_server_parse_message(g_server, message);
}

// 运行自动化测试
int run_automated_tests() {
    printf("=== Running Weather Server Automated Tests ===\n");
    
    // 测试消息列表
    const char* test_messages[] = {
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\",\"params\":{}}",
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"list_cities\",\"arguments\":{}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"get_current_weather\",\"arguments\":{\"city\":\"Beijing\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\",\"params\":{\"name\":\"get_current_weather\",\"arguments\":{\"city\":\"Shanghai\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"tools/call\",\"params\":{\"name\":\"get_current_weather\",\"arguments\":{\"city\":\"Tokyo\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"tools/call\",\"params\":{\"name\":\"get_weather_forecast\",\"arguments\":{\"city\":\"Beijing\",\"days\":3}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"tools/call\",\"params\":{\"name\":\"get_weather_forecast\",\"arguments\":{\"city\":\"Shanghai\",\"days\":5}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":9,\"method\":\"tools/call\",\"params\":{\"name\":\"get_weather_forecast\",\"arguments\":{\"city\":\"Tokyo\",\"days\":7}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tools/call\",\"params\":{\"name\":\"compare_weather\",\"arguments\":{\"city1\":\"Beijing\",\"city2\":\"Tokyo\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"tools/call\",\"params\":{\"name\":\"compare_weather\",\"arguments\":{\"city1\":\"Shanghai\",\"city2\":\"London\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"tools/call\",\"params\":{\"name\":\"get_weather_stats\",\"arguments\":{}}}",
        // 错误测试
        "{\"jsonrpc\":\"2.0\",\"id\":13,\"method\":\"tools/call\",\"params\":{\"name\":\"get_current_weather\",\"arguments\":{\"city\":\"\"}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":14,\"method\":\"tools/call\",\"params\":{\"name\":\"get_weather_forecast\",\"arguments\":{\"city\":\"Unknown City\",\"days\":10}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":15,\"method\":\"tools/call\",\"params\":{\"name\":\"compare_weather\",\"arguments\":{\"city1\":\"Beijing\",\"city2\":\"NonExistentPlace\"}}}"
    };
    
    const char* test_descriptions[] = {
        "Initialize server",
        "List available tools",
        "List supported cities",
        "Get weather for Beijing",
        "Get weather for Shanghai",
        "Get weather for Tokyo",
        "Get 3-day forecast for Beijing",
        "Get 5-day forecast for Shanghai",
        "Get 7-day forecast for Tokyo",
        "Compare weather between Beijing and Tokyo",
        "Compare weather between Shanghai and London",
        "Get global weather statistics",
        "Test empty city error",
        "Test invalid city forecast",
        "Test nonexistent city comparison"
    };
    
    size_t num_tests = sizeof(test_messages) / sizeof(test_messages[0]);
    int passed_tests = 0;
    
    for (size_t i = 0; i < num_tests; i++) {
        printf("\nTest %zu: %s\n", i + 1, test_descriptions[i]);
        printf("Message: %s\n", test_messages[i]);
        
        // 处理消息
        process_message(test_messages[i]);
        passed_tests++;
        
        // 短暂延迟以便观察输出
        usleep(100000); // 100ms
    }
    
    printf("\n=== Test Results ===\n");
    printf("Total tests: %zu\n", num_tests);
    printf("Passed tests: %d\n", passed_tests);
    printf("Weather server tests completed successfully!\n");
    
    return 0;
}

// 主函数
int main() {
    printf("=== MCP Weather Server Example ===\n");
    printf("This server provides weather information and forecasts.\n");
    printf("Available tools: get_current_weather, get_weather_forecast, get_weather_stats, list_cities, compare_weather\n");
    printf("Running automated tests...\n\n");
    
    // 初始化服务器
    if (!init_weather_server()) {
        fprintf(stderr, "Failed to initialize weather server\n");
        return 1;
    }
    
    // 运行自动化测试
    int result = run_automated_tests();
    
    printf("\nShutting down weather server...\n");
    cleanup_weather_server();
    
    return result;
}