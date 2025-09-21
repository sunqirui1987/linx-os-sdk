#include "../audio_interface.h"
#include "../portaudio_mac.h"
#include "../../log/linx_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static volatile int running = 1;

void signal_handler(int sig) {
    running = 0;
    printf("\nStopping audio test...\n");
}

int test_audio_record_play() {
    printf("Testing audio record and play functionality...\n");
    
    // Create PortAudio Mac implementation
    AudioInterface* audio = portaudio_mac_create();
    if (!audio) {
        printf("Failed to create audio interface\n");
        return -1;
    }
    
    // Initialize audio
    audio_interface_init(audio);
    if (!audio->is_initialized) {
        printf("Failed to initialize audio interface\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    // Set audio configuration
    // 44.1kHz, 1024 frame size, 1 channel (mono), 4 periods, 8192 buffer size, 2048 period size
    audio_interface_set_config(audio, 44100, 1024, 1, 4, 8192, 2048);
    
    printf("Audio configuration:\n");
    printf("  Sample Rate: %u Hz\n", audio->sample_rate);
    printf("  Channels: %d\n", audio->channels);
    printf("  Frame Size: %d\n", audio->frame_size);
    printf("  Buffer Size: %d\n", audio->buffer_size);
    
    // Start recording
    printf("Starting recording...\n");
    audio_interface_record(audio);
    
    if (!audio->is_recording) {
        printf("Failed to start recording\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    // Start playback
    printf("Starting playback...\n");
    audio_interface_play(audio);
    
    if (!audio->is_playing) {
        printf("Failed to start playback\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    // Buffer for audio data
    size_t buffer_size = audio->frame_size * audio->channels;
    short* audio_buffer = (short*)malloc(buffer_size * sizeof(short));
    if (!audio_buffer) {
        printf("Failed to allocate audio buffer\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    printf("Recording and playing audio... Press Ctrl+C to stop\n");
    printf("You should hear your microphone input through your speakers/headphones\n");
    
    // Main loop: read from microphone and write to speakers
    int frames_processed = 0;
    while (running) {
        // Read audio data from microphone
        if (audio_interface_read(audio, audio_buffer, audio->frame_size)) {
            // Write audio data to speakers (echo back)
            if (audio_interface_write(audio, audio_buffer, audio->frame_size)) {
                frames_processed++;
                if (frames_processed % 100 == 0) {
                    printf("Processed %d frames\n", frames_processed);
                }
            } else {
                printf("Failed to write audio data\n");
            }
        } else {
            printf("Failed to read audio data\n");
            usleep(1000); // Sleep 1ms to avoid busy waiting
        }
    }
    
    printf("Processed total %d frames\n", frames_processed);
    
    // Cleanup
    free(audio_buffer);
    audio_interface_destroy(audio);
    
    printf("Audio test completed successfully\n");
    return 0;
}

int test_audio_basic() {
    printf("Testing basic audio interface functionality...\n");
    
    // Create PortAudio Mac implementation
    AudioInterface* audio = portaudio_mac_create();
    if (!audio) {
        printf("Failed to create audio interface\n");
        return -1;
    }
    
    printf("✓ Audio interface created\n");
    
    // Test initialization
    audio_interface_init(audio);
    if (audio->is_initialized) {
        printf("✓ Audio interface initialized\n");
    } else {
        printf("✗ Audio interface initialization failed\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    // Test configuration
    audio_interface_set_config(audio, 44100, 512, 1, 2, 4096, 1024);
    if (audio->sample_rate == 44100 && audio->channels == 1) {
        printf("✓ Audio configuration set correctly\n");
    } else {
        printf("✗ Audio configuration failed\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    // Cleanup
    audio_interface_destroy(audio);
    printf("✓ Audio interface destroyed\n");
    
    printf("Basic audio test completed successfully\n");
    return 0;
}

int main(int argc, char* argv[]) {
    printf("=== LINX Audio Test Suite ===\n\n");
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Run basic test first
    if (test_audio_basic() != 0) {
        printf("Basic audio test failed\n");
        return 1;
    }
    
    printf("\n");
    
    // Ask user if they want to run the interactive test
    if (argc > 1 && strcmp(argv[1], "--interactive") == 0) {
        printf("Running interactive audio test...\n");
        printf("Make sure you have a microphone and speakers/headphones connected\n");
        printf("Press Enter to continue or Ctrl+C to skip...");
        getchar();
        
        if (test_audio_record_play() != 0) {
            printf("Interactive audio test failed\n");
            return 1;
        }
    } else {
        printf("To run interactive test (record/play), use: %s --interactive\n", argv[0]);
    }
    
    printf("\n=== All tests completed ===\n");
    return 0;
}