# CMakeLists.txt for unit tests

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/stubs
    ${CMAKE_SOURCE_DIR}/src/wasm
)

# Test executables
add_executable(test_new_entities test_new_entities.cpp)
add_executable(test_memory_leaks test_memory_leaks.cpp)
add_executable(test_batch_processing test_batch_processing.cpp)

# Link with Google Test and jwwlib
target_link_libraries(test_new_entities 
    GTest::gtest 
    GTest::gtest_main
    jwwlib_static
)

target_link_libraries(test_memory_leaks 
    GTest::gtest 
    GTest::gtest_main
    jwwlib_static
)

target_link_libraries(test_batch_processing 
    GTest::gtest 
    GTest::gtest_main
    jwwlib_static
)

# Add tests to CTest
add_test(NAME NewEntitiesTest COMMAND test_new_entities)
add_test(NAME MemoryLeakTest COMMAND test_memory_leaks)
add_test(NAME BatchProcessingTest COMMAND test_batch_processing)

# Enable memory leak detection for tests (if using AddressSanitizer)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
    
    set_target_properties(test_memory_leaks PROPERTIES
        COMPILE_FLAGS ${SANITIZER_FLAGS}
        LINK_FLAGS ${SANITIZER_FLAGS}
    )
endif()

# Copy test fixtures if needed
file(GLOB TEST_FIXTURES "${CMAKE_SOURCE_DIR}/tests/fixtures/*")
file(COPY ${TEST_FIXTURES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/fixtures)