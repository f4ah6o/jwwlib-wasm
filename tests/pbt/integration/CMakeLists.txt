# CMakeLists.txt for Google Test integration

# Create a unified test executable that includes both unit tests and PBT
add_executable(jwwlib_all_tests
    gtest_main.cpp
    test_parser_properties_gtest.cpp
    test_wasm_properties_gtest.cpp
    
    # Include existing unit tests
    ${CMAKE_SOURCE_DIR}/tests/unit/test_new_entities.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/test_memory_leaks.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/test_batch_processing.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/test_wasm_interface.cpp
)

target_include_directories(jwwlib_all_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/stubs
    ${CMAKE_SOURCE_DIR}/src/wasm
    ${CMAKE_SOURCE_DIR}/tests/pbt
)

target_link_libraries(jwwlib_all_tests
    GTest::gtest
    rapidcheck
    jwwlib_static
    pbt_framework
)

# Enable coverage if requested
if(ENABLE_COVERAGE)
    target_compile_options(jwwlib_all_tests PRIVATE --coverage)
    target_link_options(jwwlib_all_tests PRIVATE --coverage)
endif()

# Add as CTest
add_test(NAME AllTests COMMAND jwwlib_all_tests)

# Add test with XML output for CI
add_test(NAME AllTestsXML 
    COMMAND jwwlib_all_tests --gtest_output=xml:test_results.xml
)

# Add test with specific PBT configuration
add_test(NAME AllTestsPBTQuick
    COMMAND jwwlib_all_tests 
        --gtest_filter="*Property*"
        --pbt-max-success=10
        --pbt-max-size=50
)

# Create a separate executable for PBT-only tests
add_executable(jwwlib_pbt_tests
    gtest_main.cpp
    test_parser_properties_gtest.cpp
    test_wasm_properties_gtest.cpp
)

target_include_directories(jwwlib_pbt_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/stubs
    ${CMAKE_SOURCE_DIR}/src/wasm
    ${CMAKE_SOURCE_DIR}/tests/pbt
)

target_link_libraries(jwwlib_pbt_tests
    GTest::gtest
    rapidcheck
    jwwlib_static
    pbt_framework
)

add_test(NAME PBTOnly COMMAND jwwlib_pbt_tests)

# Helper script for running tests with coverage
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run_tests_with_coverage.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run_tests_with_coverage.sh
    @ONLY
)

# Copy test utilities
file(COPY test_utils.hpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})