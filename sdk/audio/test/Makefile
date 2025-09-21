# Makefile for audio test

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -O0
INCLUDES = -I.. -I../.. -I/opt/homebrew/opt/portaudio/include
LIBS = -L/opt/homebrew/opt/portaudio/lib -lportaudio -lpthread

# macOS specific frameworks
FRAMEWORKS = -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices

# Build directory
BUILD_DIR = build

# Source files
AUDIO_SOURCES = ../audio_interface.c ../portaudio_mac.c ../../log/linx_log.c
TEST_SOURCES = audio_test.c

# Object files (in build directory)
AUDIO_OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(AUDIO_SOURCES:.c=.o)))
TEST_OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(TEST_SOURCES:.c=.o)))

# Target
TARGET = $(BUILD_DIR)/audio_test

.PHONY: all clean test test-interactive install-deps

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(AUDIO_OBJECTS) $(TEST_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(FRAMEWORKS)

# Compile audio interface sources
$(BUILD_DIR)/audio_interface.o: ../audio_interface.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/portaudio_mac.o: ../portaudio_mac.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/linx_log.o: ../../log/linx_log.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile test sources
$(BUILD_DIR)/audio_test.o: audio_test.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

test: $(TARGET)
	$(TARGET)

test-interactive: $(TARGET)
	$(TARGET) --interactive

clean:
	rm -rf $(BUILD_DIR)

install-deps:
	@echo "Installing PortAudio via Homebrew..."
	@if command -v brew >/dev/null 2>&1; then \
		brew install portaudio; \
	else \
		echo "Homebrew not found. Please install Homebrew first:"; \
		echo "/bin/bash -c \"\$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""; \
	fi

help:
	@echo "Available targets:"
	@echo "  all            - Build the audio test"
	@echo "  test           - Run basic audio test"
	@echo "  test-interactive - Run interactive audio test (record/play)"
	@echo "  clean          - Clean build files"
	@echo "  install-deps   - Install PortAudio via Homebrew"
	@echo "  help           - Show this help message"