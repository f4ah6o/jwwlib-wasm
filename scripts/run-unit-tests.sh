#!/bin/bash
# Script to build and run C++ unit tests

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building C++ unit tests...${NC}"

# Create build directory for native tests
mkdir -p build-native
cd build-native

# Configure with CMake (native build, not Emscripten)
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF ..

# Build tests
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

echo -e "${GREEN}Running unit tests...${NC}"

# Run tests with CTest
ctest --output-on-failure --verbose

# Optional: Run with AddressSanitizer if available
if command -v llvm-symbolizer &> /dev/null; then
    echo -e "${YELLOW}Running memory leak tests with AddressSanitizer...${NC}"
    export ASAN_OPTIONS=detect_leaks=1:symbolize=1
    export ASAN_SYMBOLIZER_PATH=$(which llvm-symbolizer)
    ./tests/unit/test_memory_leaks || true
fi

# Optional: Run with Valgrind if available
if command -v valgrind &> /dev/null; then
    echo -e "${YELLOW}Running memory leak tests with Valgrind...${NC}"
    valgrind --leak-check=full --show-leak-kinds=all ./tests/unit/test_memory_leaks || true
fi

echo -e "${GREEN}Unit tests completed!${NC}"

# Return to original directory
cd ..