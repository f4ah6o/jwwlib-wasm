#pragma once

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <string>
#include <functional>
#include <memory>
#include <sstream>

namespace jwwlib {
namespace pbt {

/**
 * @brief Adapter to run RapidCheck property tests within Google Test framework
 */
class GTestPropertyAdapter : public ::testing::Test {
protected:
    struct PropertyTestConfig {
        int max_success = 100;
        int max_size = 100;
        int max_shrink_steps = 100;
        bool verbose_shrinking = true;
        std::string seed;  // Optional seed for reproducibility
    };

    PropertyTestConfig config_;

    void SetUp() override {
        // Load configuration from environment or defaults
        if (const char* env_max_success = std::getenv("RC_MAX_SUCCESS")) {
            config_.max_success = std::stoi(env_max_success);
        }
        if (const char* env_max_size = std::getenv("RC_MAX_SIZE")) {
            config_.max_size = std::stoi(env_max_size);
        }
        if (const char* env_seed = std::getenv("RC_SEED")) {
            config_.seed = env_seed;
        }
    }

    template<typename Property>
    void RunProperty(const std::string& property_name, Property&& property) {
        rc::detail::Configuration rc_config;
        rc_config.maxSuccess = config_.max_success;
        rc_config.maxSize = config_.max_size;
        
        if (!config_.seed.empty()) {
            rc_config.seed = rc::detail::parseSeed(config_.seed);
        }

        auto result = rc::detail::checkTestable(std::forward<Property>(property), rc_config);

        if (!result) {
            // Format failure message with counterexample
            std::stringstream ss;
            ss << "Property '" << property_name << "' failed!\n";
            
            if (result.counterexample) {
                ss << "Counterexample:\n";
                for (const auto& value : result.counterexample->shrinkable) {
                    ss << "  " << rc::detail::toString(value) << "\n";
                }
            }
            
            ss << "Reproduce with seed: " << rc::detail::seedToString(result.seed) << "\n";
            
            GTEST_FAIL() << ss.str();
        } else {
            RecordProperty("property_cases_tested", result.numSuccess);
            RecordProperty("property_seed", rc::detail::seedToString(result.seed));
        }
    }
};

/**
 * @brief Macro to define a property test that integrates with Google Test
 */
#define PBT_TEST(TestSuiteName, TestName)                                      \
    class TestSuiteName##_##TestName##_Property : public GTestPropertyAdapter { \
    protected:                                                                 \
        void RunPropertyTest();                                               \
    };                                                                        \
    TEST_F(TestSuiteName##_##TestName##_Property, Property) {                \
        RunPropertyTest();                                                    \
    }                                                                         \
    void TestSuiteName##_##TestName##_Property::RunPropertyTest()

/**
 * @brief Macro to define a parameterized property test
 */
#define PBT_TEST_P(TestSuiteName, TestName)                                    \
    class TestSuiteName##_##TestName##_Property :                             \
        public GTestPropertyAdapter,                                          \
        public ::testing::WithParamInterface<PropertyTestConfig> {            \
    protected:                                                                \
        void SetUp() override {                                              \
            GTestPropertyAdapter::SetUp();                                   \
            config_ = GetParam();                                            \
        }                                                                    \
        void RunPropertyTest();                                              \
    };                                                                       \
    TEST_P(TestSuiteName##_##TestName##_Property, Property) {                \
        RunPropertyTest();                                                   \
    }                                                                        \
    void TestSuiteName##_##TestName##_Property::RunPropertyTest()

/**
 * @brief Utility class for property test assertions
 */
class PropertyAssertions {
public:
    template<typename T>
    static void AssertRoundTrip(const T& original, 
                               std::function<T(const T&)> roundtrip,
                               std::function<bool(const T&, const T&)> equals = std::equal_to<T>()) {
        T result = roundtrip(original);
        if (!equals(original, result)) {
            throw rc::CaseResult::failure() 
                << "Round-trip property failed:\n"
                << "  Original: " << rc::detail::toString(original) << "\n"
                << "  Result: " << rc::detail::toString(result);
        }
    }

    template<typename T>
    static void AssertInvariant(const T& value, 
                               std::function<bool(const T&)> invariant,
                               const std::string& description) {
        if (!invariant(value)) {
            throw rc::CaseResult::failure()
                << "Invariant '" << description << "' failed for:\n"
                << "  Value: " << rc::detail::toString(value);
        }
    }

    template<typename Input, typename Output>
    static void AssertDeterministic(const Input& input,
                                   std::function<Output(const Input&)> function) {
        Output result1 = function(input);
        Output result2 = function(input);
        
        if (!(result1 == result2)) {
            throw rc::CaseResult::failure()
                << "Function is not deterministic:\n"
                << "  Input: " << rc::detail::toString(input) << "\n"
                << "  Result1: " << rc::detail::toString(result1) << "\n"
                << "  Result2: " << rc::detail::toString(result2);
        }
    }
};

/**
 * @brief Test listener for collecting PBT statistics
 */
class PBTTestListener : public ::testing::TestEventListener {
private:
    struct TestStats {
        int total_properties = 0;
        int failed_properties = 0;
        int total_cases = 0;
        std::vector<std::string> counterexamples;
    };

    TestStats stats_;
    ::testing::TestEventListener* default_listener_;

public:
    explicit PBTTestListener(::testing::TestEventListener* default_listener)
        : default_listener_(default_listener) {}

    ~PBTTestListener() override {
        delete default_listener_;
    }

    void OnTestStart(const ::testing::TestInfo& test_info) override {
        default_listener_->OnTestStart(test_info);
    }

    void OnTestPartResult(const ::testing::TestPartResult& result) override {
        default_listener_->OnTestPartResult(result);
        
        if (result.failed() && result.message()) {
            std::string message(result.message());
            if (message.find("Property") != std::string::npos &&
                message.find("failed") != std::string::npos) {
                stats_.failed_properties++;
                
                // Extract counterexample if present
                size_t counterexample_pos = message.find("Counterexample:");
                if (counterexample_pos != std::string::npos) {
                    stats_.counterexamples.push_back(message.substr(counterexample_pos));
                }
            }
        }
    }

    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        default_listener_->OnTestEnd(test_info);
        
        // Extract property test statistics from test properties
        const auto* result = test_info.result();
        for (int i = 0; i < result->test_property_count(); i++) {
            const auto& prop = result->GetTestProperty(i);
            if (std::string(prop.key()) == "property_cases_tested") {
                stats_.total_cases += std::stoi(prop.value());
                stats_.total_properties++;
            }
        }
    }

    void OnTestProgramEnd(const ::testing::UnitTest& unit_test) override {
        default_listener_->OnTestProgramEnd(unit_test);
        
        if (stats_.total_properties > 0) {
            std::cout << "\n=== Property-Based Testing Summary ===\n";
            std::cout << "Total properties tested: " << stats_.total_properties << "\n";
            std::cout << "Failed properties: " << stats_.failed_properties << "\n";
            std::cout << "Total test cases generated: " << stats_.total_cases << "\n";
            
            if (!stats_.counterexamples.empty()) {
                std::cout << "\nCounterexamples found:\n";
                for (const auto& ce : stats_.counterexamples) {
                    std::cout << ce << "\n";
                }
            }
            std::cout << "=====================================\n";
        }
    }

    // Delegate all other events
    void OnTestProgramStart(const ::testing::UnitTest& unit_test) override {
        default_listener_->OnTestProgramStart(unit_test);
    }
    void OnTestIterationStart(const ::testing::UnitTest& unit_test, int iteration) override {
        default_listener_->OnTestIterationStart(unit_test, iteration);
    }
    void OnEnvironmentsSetUpStart(const ::testing::UnitTest& unit_test) override {
        default_listener_->OnEnvironmentsSetUpStart(unit_test);
    }
    void OnEnvironmentsSetUpEnd(const ::testing::UnitTest& unit_test) override {
        default_listener_->OnEnvironmentsSetUpEnd(unit_test);
    }
    void OnTestSuiteStart(const ::testing::TestSuite& test_suite) override {
        default_listener_->OnTestSuiteStart(test_suite);
    }
    void OnTestSuiteEnd(const ::testing::TestSuite& test_suite) override {
        default_listener_->OnTestSuiteEnd(test_suite);
    }
    void OnEnvironmentsTearDownStart(const ::testing::UnitTest& unit_test) override {
        default_listener_->OnEnvironmentsTearDownStart(unit_test);
    }
    void OnEnvironmentsTearDownEnd(const ::testing::UnitTest& unit_test) override {
        default_listener_->OnEnvironmentsTearDownEnd(unit_test);
    }
    void OnTestIterationEnd(const ::testing::UnitTest& unit_test, int iteration) override {
        default_listener_->OnTestIterationEnd(unit_test, iteration);
    }
};

/**
 * @brief Install PBT test listener
 */
inline void InstallPBTTestListener() {
    ::testing::TestEventListeners& listeners = 
        ::testing::UnitTest::GetInstance()->listeners();
    
    auto* default_listener = listeners.Release(listeners.default_result_printer());
    listeners.Append(new PBTTestListener(default_listener));
}

} // namespace pbt
} // namespace jwwlib