#include "gtest_adapter.hpp"
#include "../properties/parser_properties.hpp"
#include "../generators/jww_generators.hpp"
#include <rapidcheck.h>

using namespace jwwlib::pbt;

// Test suite for parser properties using Google Test integration
class ParserPropertiesTest : public GTestPropertyAdapter {
protected:
    ParserRoundTripProperty round_trip_property_;
    ParserSafetyProperty safety_property_;
    ParserMemorySafetyProperty memory_property_;
};

PBT_TEST(ParserPropertiesTest, RoundTripForLines) {
    RunProperty("Parser round-trip for lines", [this]() {
        rc::check([this](const JWWLine& line) {
            return round_trip_property_.check(line);
        });
    });
}

PBT_TEST(ParserPropertiesTest, RoundTripForCircles) {
    RunProperty("Parser round-trip for circles", [this]() {
        rc::check([this](const JWWCircle& circle) {
            return round_trip_property_.check(circle);
        });
    });
}

PBT_TEST(ParserPropertiesTest, RoundTripForArcs) {
    RunProperty("Parser round-trip for arcs", [this]() {
        rc::check([this](const JWWArc& arc) {
            return round_trip_property_.check(arc);
        });
    });
}

PBT_TEST(ParserPropertiesTest, RoundTripForDocuments) {
    RunProperty("Parser round-trip for complete documents", [this]() {
        rc::check([this]() {
            auto doc = *jwwDocumentGen().as("JWW Document");
            
            // Serialize to bytes
            std::vector<uint8_t> serialized = serializeDocument(doc);
            RC_ASSERT(!serialized.empty());
            
            // Parse back
            auto parsed = parseDocument(serialized);
            RC_ASSERT(parsed.has_value());
            
            // Verify they are equivalent
            PropertyAssertions::AssertRoundTrip(
                doc, 
                [&serialized](const JWWDocument&) {
                    auto result = parseDocument(serialized);
                    return result.value();
                },
                [](const JWWDocument& a, const JWWDocument& b) {
                    return documentsEqual(a, b);
                }
            );
        });
    });
}

PBT_TEST(ParserPropertiesTest, SafetyWithRandomData) {
    RunProperty("Parser safety with random data", [this]() {
        rc::check([this]() {
            // Generate random binary data
            auto data = *rc::gen::container<std::vector<uint8_t>>(
                rc::gen::arbitrary<uint8_t>()
            ).as("Random binary data");
            
            // Parser should not crash on any input
            auto result = safety_property_.parseWithTimeout(data, 5000);
            
            // Result should be either valid parse or error, never crash
            RC_ASSERT(result.has_value() || result.error() != nullptr);
        });
    });
}

PBT_TEST(ParserPropertiesTest, MemorySafetyForValidDocuments) {
    RunProperty("Memory safety for valid documents", [this]() {
        rc::check([this]() {
            auto doc = *jwwDocumentGen().as("JWW Document");
            
            // Run under memory sanitizer
            bool is_safe = memory_property_.checkUnderSanitizer([&doc]() {
                auto serialized = serializeDocument(doc);
                auto parsed = parseDocument(serialized);
                // Force deallocation
                parsed.reset();
            });
            
            RC_ASSERT(is_safe);
        });
    });
}

PBT_TEST(ParserPropertiesTest, DeterministicParsing) {
    RunProperty("Deterministic parsing", [this]() {
        rc::check([this]() {
            auto doc = *jwwDocumentGen().as("JWW Document");
            auto serialized = serializeDocument(doc);
            
            PropertyAssertions::AssertDeterministic(
                serialized,
                [](const std::vector<uint8_t>& data) {
                    return parseDocument(data);
                }
            );
        });
    });
}

// Parameterized tests with different configurations
INSTANTIATE_TEST_SUITE_P(
    DifferentConfigs,
    ParserPropertiesTest,
    ::testing::Values(
        GTestPropertyAdapter::PropertyTestConfig{100, 100, 100, true, ""},
        GTestPropertyAdapter::PropertyTestConfig{1000, 50, 200, false, ""},
        GTestPropertyAdapter::PropertyTestConfig{50, 200, 50, true, "42"}
    )
);

// Test suite for error handling properties
class ParserErrorHandlingTest : public GTestPropertyAdapter {
protected:
    ParserErrorProperty error_property_;
};

PBT_TEST(ParserErrorHandlingTest, GracefulErrorHandling) {
    RunProperty("Graceful error handling", [this]() {
        rc::check([this]() {
            // Generate potentially invalid data
            auto data = *rc::gen::oneOf(
                // Truncated header
                rc::gen::container<std::vector<uint8_t>>(
                    rc::gen::arbitrary<uint8_t>()
                ).map([](auto v) { 
                    v.resize(std::min(v.size(), size_t(10))); 
                    return v; 
                }),
                // Invalid magic number
                rc::gen::container<std::vector<uint8_t>>(
                    rc::gen::arbitrary<uint8_t>()
                ).map([](auto v) {
                    if (v.size() >= 4) {
                        v[0] = 0xFF;
                        v[1] = 0xFF;
                        v[2] = 0xFF;
                        v[3] = 0xFF;
                    }
                    return v;
                }),
                // Valid header but corrupted body
                jwwDocumentGen().map([](auto doc) {
                    auto data = serializeDocument(doc);
                    // Corrupt some bytes in the middle
                    if (data.size() > 100) {
                        for (size_t i = 50; i < 70 && i < data.size(); i++) {
                            data[i] = ~data[i];
                        }
                    }
                    return data;
                })
            ).as("Potentially invalid data");
            
            auto result = error_property_.tryParse(data);
            
            // Should either parse successfully or return meaningful error
            if (!result.has_value()) {
                RC_ASSERT(result.error() != nullptr);
                RC_ASSERT(!result.error()->message.empty());
                RC_ASSERT(result.error()->code != ParseErrorCode::UNKNOWN);
            }
        });
    });
}

// Performance regression tests
class ParserPerformanceTest : public GTestPropertyAdapter {
protected:
    void SetUp() override {
        GTestPropertyAdapter::SetUp();
        // Set smaller limits for performance tests
        config_.max_success = 10;
        config_.max_size = 50;
    }
};

PBT_TEST(ParserPerformanceTest, LinearTimeComplexity) {
    RunProperty("Linear time complexity", [this]() {
        rc::check([this]() {
            // Generate documents of increasing size
            std::vector<double> sizes;
            std::vector<double> times;
            
            for (int entity_count : {10, 50, 100, 500, 1000}) {
                auto doc = *jwwDocumentGenWithSize(entity_count);
                auto serialized = serializeDocument(doc);
                
                auto start = std::chrono::high_resolution_clock::now();
                auto parsed = parseDocument(serialized);
                auto end = std::chrono::high_resolution_clock::now();
                
                double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
                
                sizes.push_back(serialized.size());
                times.push_back(time_ms);
            }
            
            // Check that time grows roughly linearly with size
            // Simple linear regression
            double n = sizes.size();
            double sum_x = 0, sum_y = 0, sum_xx = 0, sum_xy = 0;
            
            for (size_t i = 0; i < sizes.size(); i++) {
                sum_x += sizes[i];
                sum_y += times[i];
                sum_xx += sizes[i] * sizes[i];
                sum_xy += sizes[i] * times[i];
            }
            
            double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
            double intercept = (sum_y - slope * sum_x) / n;
            
            // Calculate R-squared
            double mean_y = sum_y / n;
            double ss_tot = 0, ss_res = 0;
            
            for (size_t i = 0; i < sizes.size(); i++) {
                double y_pred = slope * sizes[i] + intercept;
                ss_tot += (times[i] - mean_y) * (times[i] - mean_y);
                ss_res += (times[i] - y_pred) * (times[i] - y_pred);
            }
            
            double r_squared = 1 - (ss_res / ss_tot);
            
            // R-squared should be high (> 0.8) for linear relationship
            RC_ASSERT(r_squared > 0.8);
            
            RecordProperty("performance_r_squared", r_squared);
            RecordProperty("performance_slope_ms_per_byte", slope);
        });
    });
}