# CMakeLists.txt for jwwlib-wasm tests

# Add unit test subdirectory
add_subdirectory(unit)

# Add property-based testing subdirectory
add_subdirectory(pbt)

# Note: Integration tests are JavaScript/TypeScript based and run with npm/pnpm