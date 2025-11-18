#!/bin/bash
#
# Build script for RPI Firmware Update Tool
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║        Building RPI to STM32 Firmware Update Tool             ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Clean previous build
echo -e "${YELLOW}[1/3] Cleaning previous build...${NC}"
make clean
echo ""

# Build the tool
echo -e "${YELLOW}[2/3] Building binFileUpdate...${NC}"
make
echo ""

# Show result
if [ -f "binFileUpdate" ]; then
    echo -e "${GREEN}[3/3] Build successful!${NC}"
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                    BUILD SUCCESSFUL                            ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Executable: ./binFileUpdate"
    echo "Usage:      sudo ./binFileUpdate <firmware.bin> <port_number>"
    echo ""
    echo "Examples:"
    echo "  sudo ./binFileUpdate firmware.bin 1    # Use /dev/ttyS0"
    echo "  sudo ./binFileUpdate app.bin 2         # Use /dev/ttyAMA0"
    echo "  sudo ./binFileUpdate boot.bin 3        # Use /dev/ttyUSB0"
    echo ""
else
    echo -e "${RED}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║                      BUILD FAILED                              ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════════╝${NC}"
    exit 1
fi
