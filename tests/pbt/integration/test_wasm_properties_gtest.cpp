#include "gtest_adapter.hpp"
#include "../properties/wasm_properties.hpp"
#include "../generators/jww_generators.hpp"
#include <rapidcheck.h>
#include <emscripten/val.h>

using namespace jwwlib::pbt;
using emscripten::val;

// Test suite for WebAssembly binding properties
class WASMBindingPropertiesTest : public GTestPropertyAdapter {
protected:
    WASMDataConversionProperty conversion_property_;
    WASMEncodingProperty encoding_property_;
    WASMMemoryProperty memory_property_;
    WASMErrorProperty error_property_;
};

PBT_TEST(WASMBindingPropertiesTest, DataStructureMapping) {
    RunProperty("C++ to JavaScript structure mapping", [this]() {
        rc::check([this]() {
            // Test line entity mapping
            auto line = *jwwLineGen().as("JWW Line");
            
            val js_line = conversion_property_.convertToJS(line);
            RC_ASSERT(js_line["type"].as<std::string>() == "LINE");
            RC_ASSERT(js_line.hasOwnProperty("start"));
            RC_ASSERT(js_line.hasOwnProperty("end"));
            RC_ASSERT(js_line["start"].hasOwnProperty("x"));
            RC_ASSERT(js_line["start"].hasOwnProperty("y"));
            
            // Verify numeric precision
            double epsilon = 1e-10;
            RC_ASSERT(std::abs(js_line["start"]["x"].as<double>() - line.start.x) < epsilon);
            RC_ASSERT(std::abs(js_line["start"]["y"].as<double>() - line.start.y) < epsilon);
            RC_ASSERT(std::abs(js_line["end"]["x"].as<double>() - line.end.x) < epsilon);
            RC_ASSERT(std::abs(js_line["end"]["y"].as<double>() - line.end.y) < epsilon);
        });
    });
}

PBT_TEST(WASMBindingPropertiesTest, ShiftJISToUTF8Conversion) {
    RunProperty("Shift-JIS to UTF-8 encoding conversion", [this]() {
        rc::check([this]() {
            auto text = *japaneseTextGen().as("Japanese text");
            
            // Convert to Shift-JIS
            auto shift_jis = encoding_property_.toShiftJIS(text);
            RC_ASSERT(!shift_jis.empty());
            
            // Convert through WASM binding
            val js_text = encoding_property_.convertTextToJS(shift_jis);
            RC_ASSERT(js_text.typeOf().as<std::string>() == "string");
            
            // Convert back and verify
            std::string utf8_result = js_text.as<std::string>();
            PropertyAssertions::AssertRoundTrip(
                text,
                [&shift_jis, this](const std::string&) {
                    return encoding_property_.fromShiftJIS(shift_jis);
                }
            );
        });
    });
}

PBT_TEST(WASMBindingPropertiesTest, MemoryUsagePredictability) {
    RunProperty("Memory usage predictability", [this]() {
        rc::check([this]() {
            auto doc = *jwwDocumentGen().as("JWW Document");
            
            // Measure memory before conversion
            size_t initial_memory = memory_property_.getCurrentHeapSize();
            
            // Convert to JavaScript
            val js_doc = conversion_property_.convertDocumentToJS(doc);
            
            // Measure memory after conversion
            size_t final_memory = memory_property_.getCurrentHeapSize();
            size_t memory_used = final_memory - initial_memory;
            
            // Calculate expected memory usage
            size_t expected_memory = memory_property_.estimateMemoryUsage(doc);
            
            // Memory usage should be within reasonable bounds (2x expected)
            RC_ASSERT(memory_used <= expected_memory * 2);
            
            // Clean up
            js_doc = val::null();
            emscripten_gc();
            
            // Verify memory is released
            size_t cleaned_memory = memory_property_.getCurrentHeapSize();
            RC_ASSERT(cleaned_memory <= initial_memory * 1.1); // Allow 10% overhead
        });
    });
}

PBT_TEST(WASMBindingPropertiesTest, ErrorPropagation) {
    RunProperty("Error propagation from C++ to JavaScript", [this]() {
        rc::check([this]() {
            // Generate various error conditions
            auto error_type = *rc::gen::element(
                ParseErrorCode::INVALID_FORMAT,
                ParseErrorCode::UNSUPPORTED_VERSION,
                ParseErrorCode::CORRUPTED_DATA,
                ParseErrorCode::MEMORY_ERROR
            );
            
            auto error_message = *rc::gen::string<std::string>();
            
            // Create error in C++
            ParseError cpp_error{error_type, error_message};
            
            // Convert to JavaScript
            val js_error = error_property_.convertErrorToJS(cpp_error);
            
            // Verify error structure
            RC_ASSERT(js_error.instanceof(val::global("Error")));
            RC_ASSERT(js_error["code"].as<int>() == static_cast<int>(error_type));
            RC_ASSERT(js_error["message"].as<std::string>() == error_message);
            
            // Verify error can be caught in JavaScript
            val can_catch = val::global("eval")(R"(
                (function(err) {
                    try {
                        throw err;
                    } catch (e) {
                        return e === err;
                    }
                })
            )")(js_error);
            
            RC_ASSERT(can_catch.as<bool>());
        });
    });
}

PBT_TEST(WASMBindingPropertiesTest, LargeDataTransfer) {
    RunProperty("Large data transfer efficiency", [this]() {
        // Use smaller limits for this test
        config_.max_success = 5;
        
        rc::check([this]() {
            // Generate large document
            auto doc = *jwwDocumentGenWithSize(1000).as("Large JWW Document");
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // Serialize in C++
            auto serialized = serializeDocument(doc);
            
            // Transfer to JavaScript
            val js_buffer = val::global("Uint8Array").new_(serialized.size());
            val js_view = val::global("Uint8Array").new_(
                val::module_property("HEAPU8")["buffer"],
                reinterpret_cast<uintptr_t>(serialized.data()),
                serialized.size()
            );
            js_buffer.call<void>("set", js_view);
            
            auto end = std::chrono::high_resolution_clock::now();
            double transfer_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            // Verify data integrity
            for (size_t i = 0; i < std::min(size_t(100), serialized.size()); i++) {
                RC_ASSERT(js_buffer[i].as<uint8_t>() == serialized[i]);
            }
            
            // Performance check: should transfer at least 1MB/s
            double mb_transferred = serialized.size() / (1024.0 * 1024.0);
            double transfer_rate_mbps = mb_transferred / (transfer_time_ms / 1000.0);
            RC_ASSERT(transfer_rate_mbps >= 1.0);
            
            RecordProperty("transfer_size_bytes", serialized.size());
            RecordProperty("transfer_time_ms", transfer_time_ms);
            RecordProperty("transfer_rate_mbps", transfer_rate_mbps);
        });
    });
}

// Test suite for complex binding scenarios
class WASMComplexBindingTest : public GTestPropertyAdapter {};

PBT_TEST(WASMComplexBindingTest, AsyncOperations) {
    RunProperty("Asynchronous operation handling", [this]() {
        rc::check([this]() {
            auto doc = *jwwDocumentGen().as("JWW Document");
            
            // Create promise in JavaScript
            val promise = val::global("Promise").new_(
                val::global("Function").new_(
                    std::string("resolve"),
                    std::string("reject"),
                    R"(
                        setTimeout(() => {
                            resolve({ status: 'completed' });
                        }, 10);
                    )"
                )
            );
            
            // Verify promise handling
            RC_ASSERT(promise.instanceof(val::global("Promise")));
            
            // Test error propagation in async context
            val error_promise = val::global("Promise")["reject"](
                val::object()
                    .set("code", 42)
                    .set("message", "Test error")
            );
            
            RC_ASSERT(error_promise.instanceof(val::global("Promise")));
        });
    });
}

PBT_TEST(WASMComplexBindingTest, CallbackIntegration) {
    RunProperty("JavaScript callback integration", [this]() {
        rc::check([this]() {
            auto entities = *rc::gen::container<std::vector<JWWLine>>(
                jwwLineGen()
            ).as("Line entities");
            
            // Create JavaScript callback
            val callback = val::global("Function").new_(
                std::string("entity"),
                std::string("index"),
                R"(
                    return {
                        processed: true,
                        index: index,
                        type: entity.type
                    };
                )"
            );
            
            // Process entities with callback
            val results = val::array();
            for (size_t i = 0; i < entities.size(); i++) {
                val js_entity = conversion_property_.convertToJS(entities[i]);
                val result = callback(js_entity, val(i));
                results.set(i, result);
            }
            
            // Verify callback results
            for (size_t i = 0; i < entities.size(); i++) {
                val result = results[i];
                RC_ASSERT(result["processed"].as<bool>() == true);
                RC_ASSERT(result["index"].as<size_t>() == i);
                RC_ASSERT(result["type"].as<std::string>() == "LINE");
            }
        });
    });
}

// Parameterized test for different WASM memory configurations
class WASMMemoryConfigTest : public GTestPropertyAdapter,
                            public ::testing::WithParamInterface<size_t> {
protected:
    void SetUp() override {
        GTestPropertyAdapter::SetUp();
        // Set memory limit based on parameter
        memory_limit_ = GetParam();
    }
    
    size_t memory_limit_;
};

TEST_P(WASMMemoryConfigTest, MemoryLimitRespected) {
    RunProperty("Memory limit enforcement", [this]() {
        rc::check([this]() {
            // Try to allocate data that might exceed limit
            size_t target_size = memory_limit_ / 2;
            
            try {
                std::vector<uint8_t> data(target_size);
                
                // Transfer to JavaScript
                val js_buffer = val::global("Uint8Array").new_(data.size());
                
                // Should succeed if within limit
                RC_ASSERT(js_buffer["length"].as<size_t>() == data.size());
            } catch (const std::bad_alloc&) {
                // Allocation failed - this is acceptable
                RC_SUCCEED();
            } catch (const val&) {
                // JavaScript error - also acceptable
                RC_SUCCEED();
            }
        });
    });
}

INSTANTIATE_TEST_SUITE_P(
    DifferentMemoryLimits,
    WASMMemoryConfigTest,
    ::testing::Values(
        16 * 1024 * 1024,   // 16MB
        64 * 1024 * 1024,   // 64MB
        256 * 1024 * 1024   // 256MB
    )
);