#!/bin/bash

# Build script for jwwlib-wasm (Release mode with PBT support)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building jwwlib-wasm in Release mode...${NC}"

# Create build directory
BUILD_DIR="build-release"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON

# Build
echo -e "${YELLOW}Building...${NC}"
make -j$(nproc)

# Run tests if requested
if [ "$1" == "--test" ]; then
    echo -e "${YELLOW}Running tests...${NC}"
    ctest --output-on-failure
    
    # Run PBT tests with optimizations
    echo -e "${YELLOW}Running Property-Based Tests (Release)...${NC}"
    ./tests/pbt/pbt_tests
fi

echo -e "${GREEN}Build completed successfully!${NC}"

# For WebAssembly build
if [ "$1" == "--wasm" ]; then
    cd ..
    echo -e "${YELLOW}Building WebAssembly version (Release)...${NC}"
    
    BUILD_DIR_WASM="build-wasm-release"
    mkdir -p $BUILD_DIR_WASM
    cd $BUILD_DIR_WASM
    
    # Use Emscripten
    emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=OFF \
        -DBUILD_EXAMPLES=OFF
    
    emmake make -j$(nproc)
    
    echo -e "${GREEN}WebAssembly build completed!${NC}"
fi