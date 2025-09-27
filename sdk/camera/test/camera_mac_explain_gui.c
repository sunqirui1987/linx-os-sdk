/**
 * @file camera_mac_explain_gui.c
 * @brief Camera test program with LVGL GUI interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

// LVGL includes
#include "lvgl.h"

// Camera includes
#include "camera_interface.h"
#include "camera_mac.h"
#include "log/linx_log.h"

// Global variables
static CameraInterface *g_camera = NULL;
static volatile int running = 1;
static pthread_mutex_t camera_mutex = PTHREAD_MUTEX_INITIALIZER;

// GUI objects
static lv_obj_t *main_screen;
static lv_obj_t *preview_img;
static lv_obj_t *capture_btn;
static lv_obj_t *explain_btn;
static lv_obj_t *status_label;
static lv_obj_t *response_textarea;
static lv_obj_t *config_panel;

// Camera frame buffer
static CameraFrameBuffer current_frame = {0};
static bool frame_available = false;

// Function prototypes
static void cleanup_and_exit(int sig);
static int init_camera_system(void);
static void cleanup_camera_system(void);
static int capture_image(void);
static int send_explain_request(const char* question);
static void init_lvgl(void);
static void create_gui(void);
static void capture_btn_event_cb(lv_event_t * e);
static void explain_btn_event_cb(lv_event_t * e);
static void update_status(const char* status);
static void update_response(const char* response);
static void update_preview_image(void);

/**
 * Signal handler for graceful shutdown
 */
static void cleanup_and_exit(int sig) {
    printf("\nReceived signal %d, cleaning up...\n", sig);
    running = 0;
    cleanup_camera_system();
    exit(0);
}

/**
 * Initialize camera system
 */
static int init_camera_system(void) {
    // Initialize logging
    log_init(NULL);
    log_set_level(LOG_LEVEL_DEBUG);
    
    printf("Initializing camera system...\n");
    
    // Create camera instance
    g_camera = mac_camera_create();
    if (!g_camera) {
        printf("Failed to create camera instance\n");
        return -1;
    }
    
    // Initialize camera
    int ret = camera_interface_init(g_camera);
    if (ret != 0) {
        printf("Failed to initialize camera: %d\n", ret);
        camera_interface_destroy(g_camera);
        g_camera = NULL;
        return -1;
    }
    
    // Configure camera
    CameraConfig config = {
        .width = 640,
        .height = 480,
        .quality = 80,
        .format = 1, // JPEG format
        .h_mirror = false,
        .v_flip = false
    };
    
    ret = camera_interface_set_config(g_camera, &config);
    if (ret != 0) {
        printf("Failed to configure camera: %d\n", ret);
        camera_interface_destroy(g_camera);
        g_camera = NULL;
        return -1;
    }
    
    // Configure explain URL
    ret = camera_interface_set_explain_url(g_camera, 
        "http://xrobo-io.qiniuapi.com/mcp/vision/explain", 
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJkYXRhIjoiLVFrTFhVbDhOZV9LeHRNcThhQUZwYTBTLUVRNTBSS01IclV1UVdVYXVKWEdHQXBhYjU3YzkzWGVJcU1tQ0IzZHFWT2F5LTkyWVAtaFpHaXpNQXN6MHZtVWJQWDhqdWRaa1NQVXNDMGFGYlk0cGhsa1FfRExmdz09In0.NaQ3jqXh2QovQpvtmM71QSBTZ-LZ0mzl3lOIo5FiJOs");
    if (ret != 0) {
        printf("Failed to configure explain URL: %d\n", ret);
        camera_interface_destroy(g_camera);
        g_camera = NULL;
        return -1;
    }
    
    printf("Camera system initialized successfully\n");
    return 0;
}

/**
 * Cleanup camera system
 */
static void cleanup_camera_system(void) {
    pthread_mutex_lock(&camera_mutex);
    if (g_camera) {
        printf("Cleaning up camera system...\n");
        if (frame_available && current_frame.data) {
            camera_interface_release_frame(g_camera, &current_frame);
            frame_available = false;
        }
        camera_interface_destroy(g_camera);
        g_camera = NULL;
    }
    pthread_mutex_unlock(&camera_mutex);
}

/**
 * Capture image from camera
 */
static int capture_image(void) {
    if (!g_camera) {
        printf("Camera not initialized\n");
        return -1;
    }
    
    pthread_mutex_lock(&camera_mutex);
    
    // Release previous frame if available
    if (frame_available && current_frame.data) {
        camera_interface_release_frame(g_camera, &current_frame);
        frame_available = false;
    }
    
    // Capture new frame
    memset(&current_frame, 0, sizeof(current_frame));
    int ret = camera_interface_capture(g_camera, &current_frame);
    if (ret == 0) {
        frame_available = true;
        printf("Image captured successfully (size: %zu bytes)\n", current_frame.size);
    } else {
        printf("Failed to capture image: %d\n", ret);
    }
    
    pthread_mutex_unlock(&camera_mutex);
    return ret;
}

/**
 * Send explain request
 */
static int send_explain_request(const char* question) {
    if (!g_camera) {
        printf("Camera not initialized\n");
        return -1;
    }
    
    char response[2048];
    int ret = camera_interface_explain(g_camera, question, response, sizeof(response));
    if (ret == 0) {
        printf("Explain request sent successfully!\n");
        printf("Response: %s\n", response);
        update_response(response);
    } else {
        printf("Failed to send explain request: %d\n", ret);
        update_response("Failed to get explanation");
    }
    
    return ret;
}

/**
 * Initialize LVGL
 */
static void init_lvgl(void) {
    lv_init();
    
    // Create SDL window and display
    lv_display_t *disp = lv_sdl_window_create(800, 600);
    if (!disp) {
        printf("Failed to create LVGL SDL window\n");
        exit(-1);
    }
    
    lv_sdl_window_set_title(disp, "Camera GUI - LVGL");
    
    // Create input device (mouse/keyboard)
    lv_indev_t *mouse = lv_sdl_mouse_create();
    lv_indev_t *keyboard = lv_sdl_keyboard_create();
    
    printf("LVGL initialized with SDL window\n");
}

/**
 * Create GUI interface
 */
static void create_gui(void) {
    // Create main screen
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x2E3440), 0);
    lv_screen_load(main_screen);
    
    // Title label
    lv_obj_t * title = lv_label_create(main_screen);
    lv_label_set_text(title, "Camera Explain Test");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Preview image area
    preview_img = lv_img_create(main_screen);
    lv_obj_set_size(preview_img, 320, 240);
    lv_obj_align(preview_img, LV_ALIGN_TOP_LEFT, 20, 70);
    lv_obj_set_style_bg_color(preview_img, lv_color_hex(0x4C566A), 0);
    lv_obj_set_style_bg_opa(preview_img, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(preview_img, 2, 0);
    lv_obj_set_style_border_color(preview_img, lv_color_hex(0x5E81AC), 0);
    
    // Placeholder text for preview
    lv_obj_t * preview_label = lv_label_create(preview_img);
    lv_label_set_text(preview_label, "Camera Preview\n(Click Capture)");
    lv_obj_set_style_text_color(preview_label, lv_color_white(), 0);
    lv_obj_center(preview_label);
    
    // Control panel
    config_panel = lv_obj_create(main_screen);
    lv_obj_set_size(config_panel, 420, 240);
    lv_obj_align(config_panel, LV_ALIGN_TOP_RIGHT, -20, 70);
    lv_obj_set_style_bg_color(config_panel, lv_color_hex(0x3B4252), 0);
    lv_obj_set_style_border_width(config_panel, 1, 0);
    lv_obj_set_style_border_color(config_panel, lv_color_hex(0x5E81AC), 0);
    
    // Capture button
    capture_btn = lv_btn_create(config_panel);
    lv_obj_set_size(capture_btn, 180, 50);
    lv_obj_align(capture_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_bg_color(capture_btn, lv_color_hex(0x5E81AC), 0);
    lv_obj_add_event_cb(capture_btn, capture_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * capture_label = lv_label_create(capture_btn);
    lv_label_set_text(capture_label, "Capture Image");
    lv_obj_set_style_text_color(capture_label, lv_color_white(), 0);
    lv_obj_center(capture_label);
    
    // Explain button
    explain_btn = lv_btn_create(config_panel);
    lv_obj_set_size(explain_btn, 180, 50);
    lv_obj_align(explain_btn, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_obj_set_style_bg_color(explain_btn, lv_color_hex(0xBF616A), 0);
    lv_obj_add_event_cb(explain_btn, explain_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * explain_label = lv_label_create(explain_btn);
    lv_label_set_text(explain_label, "Explain Image");
    lv_obj_set_style_text_color(explain_label, lv_color_white(), 0);
    lv_obj_center(explain_label);
    
    // Status label
    status_label = lv_label_create(config_panel);
    lv_label_set_text(status_label, "Status: Ready");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xA3BE8C), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 20, 90);
    
    // Response text area
    response_textarea = lv_textarea_create(main_screen);
    lv_obj_set_size(response_textarea, 760, 200);
    lv_obj_align(response_textarea, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_textarea_set_placeholder_text(response_textarea, "AI explanation will appear here...");
    lv_obj_set_style_bg_color(response_textarea, lv_color_hex(0x3B4252), 0);
    lv_obj_set_style_text_color(response_textarea, lv_color_white(), 0);
    lv_obj_set_style_border_color(response_textarea, lv_color_hex(0x5E81AC), 0);
    lv_textarea_set_text(response_textarea, "");
}

/**
 * Capture button event callback
 */
static void capture_btn_event_cb(lv_event_t * e) {
    update_status("Status: Capturing...");
    
    if (capture_image() == 0) {
        update_status("Status: Image captured successfully");
        update_preview_image();
    } else {
        update_status("Status: Failed to capture image");
    }
}

/**
 * Explain button event callback
 */
static void explain_btn_event_cb(lv_event_t * e) {
    if (!frame_available) {
        update_status("Status: Please capture an image first");
        return;
    }
    
    update_status("Status: Sending explain request...");
    update_response("Processing...");
    
    if (send_explain_request("Describe what you see in this image") == 0) {
        update_status("Status: Explanation received");
    } else {
        update_status("Status: Failed to get explanation");
    }
}

/**
 * Update status label
 */
static void update_status(const char* status) {
    if (status_label) {
        lv_label_set_text(status_label, status);
    }
}

/**
 * Update response text area
 */
static void update_response(const char* response) {
    if (response_textarea) {
        lv_textarea_set_text(response_textarea, response);
    }
}

/**
 * Update preview image
 */
static void update_preview_image(void) {
    if (!preview_img || !frame_available || !current_frame.data) {
        return;
    }
    
    pthread_mutex_lock(&camera_mutex);
    
    // Remove the placeholder label if it exists
    lv_obj_t * preview_label = lv_obj_get_child(preview_img, 0);
    if (preview_label) {
        lv_obj_del(preview_label);
    }
    
    // Create image descriptor for JPEG data
    static lv_image_dsc_t img_dsc;
    img_dsc.header.w = 640;  // Camera width
    img_dsc.header.h = 480;  // Camera height
    img_dsc.header.cf = LV_COLOR_FORMAT_RAW;  // Raw JPEG data
    img_dsc.data_size = current_frame.size;
    img_dsc.data = current_frame.data;
    
    // Set the image source
    lv_img_set_src(preview_img, &img_dsc);
    
    // Scale the image to fit the preview area (320x240)
    lv_img_set_zoom(preview_img, 128);  // 50% zoom (256 = 100%)
    
    pthread_mutex_unlock(&camera_mutex);
}



/**
 * Main function
 */
int main(void) {
    printf("=== Camera Explain GUI Test Program ===\n");
    printf("This program provides a GUI interface for camera capture and explanation.\n");
    printf("Press Ctrl+C to exit.\n\n");
    
    // Setup signal handlers
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
    
    // Initialize camera system
    if (init_camera_system() != 0) {
        printf("Failed to initialize camera system\n");
        return -1;
    }
    
    // Initialize LVGL
    init_lvgl();
    
    // Create GUI
    create_gui();
    
    printf("GUI started successfully. Use the interface to capture and explain images.\n");
    
    // Main loop - handle LVGL events on main thread
    while (running) {
        lv_timer_handler();
        usleep(5000); // 5ms delay
    }
    
    // Cleanup
    cleanup_camera_system();
    printf("Program exited successfully.\n");
    
    return 0;
}