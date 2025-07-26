cmake_minimum_required(VERSION 3.10)
project(jwwlib_wasm VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stubs
)

# Core source files (from LibreCAD)
set(CORE_SOURCES
    src/core/dl_jww.cpp
    src/core/jwwdoc.cpp
    src/core/dl_writer_ascii.cpp
)

# WASM specific sources
set(WASM_SOURCES
    src/wasm/wasm_bindings.cpp
    src/wasm/wasm_encoding.cpp
)

# Define macros
# add_definitions(-DSKIP_MOJI)  # Commented out - already defined in source

# Native build (for testing)
if(NOT EMSCRIPTEN)
    # Create static library for native testing
    add_library(jwwlib_static STATIC ${CORE_SOURCES})
    
    # Build tests if enabled
    if(BUILD_TESTS)
        enable_testing()
        
        # Find or fetch Google Test
        find_package(GTest QUIET)
        if(NOT GTest_FOUND)
            include(FetchContent)
            FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG v1.14.0
            )
            FetchContent_MakeAvailable(googletest)
        endif()
        
        # Fetch RapidCheck for Property-Based Testing
        include(FetchContent)
        FetchContent_Declare(
            rapidcheck
            GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(rapidcheck)
        
        # Find zlib for compression support in PBT framework
        find_package(ZLIB REQUIRED)
        
        add_subdirectory(tests)
    endif()
    
    # Build examples if enabled
    if(BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()
endif()

# WebAssembly build with Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building for WebAssembly with Emscripten")
    
    # Create WASM module
    add_executable(jwwlib_wasm ${CORE_SOURCES} ${WASM_SOURCES})
    
    # Emscripten compilation flags
    target_compile_options(jwwlib_wasm PRIVATE
        -fno-exceptions
    )
    
    # Emscripten link flags
    target_link_options(jwwlib_wasm PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORT_NAME='createJWWModule'"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s NO_EXIT_RUNTIME=1"
        "SHELL:-s FILESYSTEM=1"
        "SHELL:-s EXPORTED_FUNCTIONS=['_malloc','_free']"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','HEAPU8','HEAP8','UTF8ToString','stringToUTF8']"
        "SHELL:-s ENVIRONMENT='web,worker'"
        "SHELL:-s SINGLE_FILE=0"
        --bind  # Enable Embind for C++ bindings
    )
    
    # Debug/Release specific settings
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(jwwlib_wasm PRIVATE -g2)
        target_link_options(jwwlib_wasm PRIVATE 
            "SHELL:-s ASSERTIONS=1"
            "SHELL:-s SAFE_HEAP=0"
            "SHELL:-s STACK_OVERFLOW_CHECK=1"
        )
    else()
        target_compile_options(jwwlib_wasm PRIVATE -O3)
        target_link_options(jwwlib_wasm PRIVATE 
            -O3
            "SHELL:-s ASSERTIONS=0"
            "SHELL:--closure 1"
        )
    endif()
    
    # Set output properties
    set_target_properties(jwwlib_wasm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist
        OUTPUT_NAME "jwwlib"
    )
    
    # Create output directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist)
    
    # Post-build: Copy generated files
    add_custom_command(TARGET jwwlib_wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/dist/jwwlib.wasm
            ${CMAKE_CURRENT_SOURCE_DIR}/wasm/jwwlib.wasm
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/dist/jwwlib.js
            ${CMAKE_CURRENT_SOURCE_DIR}/wasm/jwwlib.js
    )
endif()

# Installation rules
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/dist/jwwlib.js
    ${CMAKE_CURRENT_SOURCE_DIR}/dist/jwwlib.wasm
    DESTINATION lib
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/jwwlib-wasmConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)