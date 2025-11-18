#
# Makefile for Raspberry Pi Firmware Update Tool
#

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lrt

# Target executable
TARGET = binFileUpdate

# Source files
SRCS = binFileUpdate.c RpiUart.c
OBJS = $(SRCS:.c=.o)

# Header files
HDRS = RpiUart.h

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build successful!"
	@echo "Usage: sudo ./$(TARGET) <firmware.bin> <port_number>"

# Compile source files
%.o: %.c $(HDRS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -f $(OBJS) $(TARGET) *.o.old *.c.old
	@echo "Clean complete!"

# Install (copy to /usr/local/bin)
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installation complete!"

# Uninstall
uninstall:
	@echo "Uninstalling $(TARGET)..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete!"

# Run with example
run: $(TARGET)
	@echo "Usage: sudo ./$(TARGET) <firmware.bin> <port_number>"

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build the firmware update tool (default)"
	@echo "  clean     - Remove build files"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build the tool"
	@echo "  make clean              # Clean build files"
	@echo "  make install            # Install system-wide"
	@echo "  sudo ./binFileUpdate firmware.bin 1   # Run the tool"

.PHONY: all clean install uninstall run help
