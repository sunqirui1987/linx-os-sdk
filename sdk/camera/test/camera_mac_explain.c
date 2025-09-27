/**
 * @file camera_mac_explain.c
 * @brief Simplified console camera test program focusing on explain_request functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "camera_interface.h"
#include "camera_mac.h"
#include "log/linx_log.h"

// Global variables
static CameraInterface *g_camera = NULL;
static volatile int running = 1;

// Function prototypes
static void cleanup_and_exit(int sig);
static int init_camera_system(void);
static void cleanup_camera_system(void);
static int capture_and_explain(void);

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
    if (g_camera) {
        printf("Cleaning up camera system...\n");
        camera_interface_destroy(g_camera);
        g_camera = NULL;
    }
}

/**
 * Capture image and send explain request
 */
static int capture_and_explain(void) {
    if (!g_camera) {
        printf("Camera not initialized\n");
        return -1;
    }
    
    printf("Capturing image...\n");
    
    // Capture image
    CameraFrameBuffer frame;
    memset(&frame, 0, sizeof(frame));
    
    int ret = camera_interface_capture(g_camera, &frame);
    if (ret != 0) {
        printf("Failed to capture image: %d\n", ret);
        return -1;
    }
    
    printf("Image captured successfully (size: %zu bytes)\n", frame.size);
    
    // Send explain request
    printf("Sending explain request...\n");
    char response[1024];
    ret = camera_interface_explain(g_camera, "Describe what you see in this image", response, sizeof(response));
    if (ret == 0) {
        printf("Explain request sent successfully!\n");
        printf("Response: %s\n", response);
    } else {
        printf("Failed to send explain request: %d\n", ret);
    }
    
    // Release frame data
    camera_interface_release_frame(g_camera, &frame);
    
    return ret;
}



/**
 * Main function
 */
int main(void) {
    printf("=== Camera Explain Test Program ===\n");
    printf("This program will capture an image and send it for explanation.\n");
    printf("Press Ctrl+C to exit.\n\n");
    
    // Setup signal handlers
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
    
    // Initialize camera system
    if (init_camera_system() != 0) {
        printf("Failed to initialize camera system\n");
        return -1;
    }
    
    printf("\nCamera system ready. Press Enter to capture and explain, or 'q' to quit.\n");
    
    // Main loop
    char input[256];
    while (running) {
        printf("\n> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            // Empty input (Enter pressed) - capture and explain
            capture_and_explain();
        } else if (strcmp(input, "q") == 0 || strcmp(input, "quit") == 0) {
            // Quit command
            break;
        } else if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0) {
            // Help command
            printf("Commands:\n");
            printf("  Enter    - Capture image and send explain request\n");
            printf("  q/quit   - Quit the program\n");
            printf("  h/help   - Show this help\n");
        } else {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
    
    // Cleanup
    cleanup_camera_system();
    printf("Program exited successfully.\n");
    
    return 0;
}