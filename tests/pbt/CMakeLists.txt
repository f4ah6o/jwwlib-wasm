# Property-Based Testing (PBT) for jwwlib-wasm

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../test/pbt/framework
)

# PBT framework headers
set(PBT_HEADERS
    include/property_base.h
    include/property_builder.h
    include/pbt_exception.h
    include/test_execution_config.h
    include/counterexample_minimizer.h
)

# PBT framework sources
set(PBT_SOURCES
    src/property_base.cpp
    src/property_builder.cpp
    src/pbt_exception.cpp
    src/test_execution_config.cpp
    src/counterexample_minimizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../test/pbt/framework/reporting/html_report_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../test/pbt/framework/reporting/data_visualizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../test/pbt/framework/reporting/test_metrics.cpp
)

# JWW entity generators
set(GENERATOR_HEADERS
    generators/jww_entity_generators.h
    generators/line_generator.h
    generators/circle_generator.h
    generators/arc_generator.h
    generators/text_generator.h
    generators/layer_generator.h
    generators/document_generator.h
)

# Property headers
set(PROPERTY_HEADERS
    properties/parser_properties.h
    properties/error_handling_properties.h
    properties/wasm_binding_properties.h
)

# Test files
set(TEST_SOURCES
    tests/test_parser_properties.cpp
    tests/test_error_handling_properties.cpp
    tests/test_wasm_binding_properties.cpp
)

# Create PBT test executable
add_executable(pbt_tests
    ${PBT_SOURCES}
    ${TEST_SOURCES}
)

# Link with required libraries
target_link_libraries(pbt_tests
    jwwlib_static
    rapidcheck
    GTest::gtest
    GTest::gtest_main
    ZLIB::ZLIB
)

# Set C++ standard
set_target_properties(pbt_tests PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

# Register tests with CTest
add_test(NAME pbt_all_tests COMMAND pbt_tests)

# Add timeout for PBT tests (10 minutes)
set_tests_properties(pbt_all_tests PROPERTIES TIMEOUT 600)

# Create individual test targets for development
foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(pbt_${test_name} ${test_file} ${PBT_SOURCES})
    target_link_libraries(pbt_${test_name}
        jwwlib_static
        rapidcheck
        GTest::gtest
        GTest::gtest_main
        ZLIB::ZLIB
    )
    add_test(NAME pbt_${test_name} COMMAND pbt_${test_name})
    set_tests_properties(pbt_${test_name} PROPERTIES TIMEOUT 300)
endforeach()

# Add custom target for running PBT tests with memory checking
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_custom_target(pbt_memcheck
            COMMAND ${VALGRIND_EXECUTABLE} --leak-check=full --error-exitcode=1 $<TARGET_FILE:pbt_tests>
            DEPENDS pbt_tests
            COMMENT "Running PBT tests with Valgrind memory check"
        )
    endif()
endif()

# Add custom target for running PBT tests with AddressSanitizer
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_custom_target(pbt_asan
        COMMAND ${CMAKE_COMMAND} -E env ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $<TARGET_FILE:pbt_tests>
        DEPENDS pbt_tests
        COMMENT "Running PBT tests with AddressSanitizer"
    )
endif()

# Generate compilation database for development tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add Google Test integration subdirectory
add_subdirectory(integration)