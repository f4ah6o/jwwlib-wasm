#!/bin/bash

# Build script for jwwlib-wasm (Debug mode with PBT support)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building jwwlib-wasm in Debug mode...${NC}"

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo -e "${RED}Error: Emscripten not found. Please install and activate emsdk.${NC}"
    exit 1
fi

# Create build directory
BUILD_DIR="build-debug"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with Emscripten
echo -e "${YELLOW}Configuring with Emscripten (Debug mode)...${NC}"
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF

# Build
echo -e "${YELLOW}Building WebAssembly module...${NC}"
emmake make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 1)

# Copy outputs to wasm directory
echo -e "${YELLOW}Copying outputs...${NC}"
cd ..
mkdir -p wasm
cp -f dist/jwwlib.js wasm/
cp -f dist/jwwlib.wasm wasm/

echo -e "${GREEN}WebAssembly build (Debug) completed successfully!${NC}"
echo -e "${GREEN}Output files:${NC}"
echo -e "  - wasm/jwwlib.js"
echo -e "  - wasm/jwwlib.wasm"