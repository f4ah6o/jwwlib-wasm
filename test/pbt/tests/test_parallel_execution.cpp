#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <chrono>
#include <thread>
#include <vector>

#include "../framework/parallel/parallel_test_runner.hpp"
#include "../framework/parallel/resource_monitor.hpp"
#include "../framework/storage/counterexample_database.hpp"
#include "../framework/generators/lazy_generator.hpp"
#include "../generators/jww_generators.hpp"
#include "../generators/lazy_jww_generators.hpp"
#include "../properties/parser_properties.hpp"

using namespace jwwlib::pbt;
using namespace jwwlib::pbt::parallel;
using namespace jwwlib::pbt::storage;
using namespace jwwlib::pbt::generators;
using namespace jwwlib::pbt::properties;

class ParallelExecutionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize resource monitor
        resource_monitor_ = std::make_shared<ResourceMonitor>();
        resource_monitor_->start();
        
        // Initialize counterexample database
        CounterexampleDatabase::Config db_config;
        db_config.database_path = "test_parallel_execution.db";
        db_config.enable_compression = true;
        counterexample_db_ = std::make_shared<CounterexampleDatabase>(db_config);
    }
    
    void TearDown() override {
        resource_monitor_->stop();
        // Clean up test database
        std::filesystem::remove("test_parallel_execution.db");
    }
    
    std::shared_ptr<ResourceMonitor> resource_monitor_;
    std::shared_ptr<CounterexampleDatabase> counterexample_db_;
};

TEST_F(ParallelExecutionTest, BasicParallelExecution) {
    ParallelTestRunner::Config config;
    config.num_threads = 4;
    config.default_timeout = std::chrono::milliseconds(5000);
    
    ParallelTestRunner runner(config);
    
    // Add multiple properties
    for (int i = 0; i < 10; ++i) {
        auto property = std::make_shared<RoundTripProperty>();
        runner.add_property("round_trip_" + std::to_string(i), property);
    }
    
    // Run all tests
    auto results = runner.run_all();
    
    // Verify all tests passed
    ASSERT_EQ(results.size(), 10);
    for (const auto& result : results) {
        EXPECT_TRUE(result.success) << "Test " << result.test_name 
                                   << " failed: " << result.error_message;
    }
}

TEST_F(ParallelExecutionTest, ResourceConstrainedExecution) {
    ParallelTestRunner::Config config;
    config.num_threads = 8;
    config.adaptive_scheduling = true;
    config.cpu_threshold = 0.7;
    config.memory_threshold = 0.7;
    
    ParallelTestRunner runner(config);
    
    // Add memory-intensive properties
    for (int i = 0; i < 20; ++i) {
        auto property = std::make_shared<LargeDocumentProperty>();
        TestExecutionConfig test_config;
        test_config.max_test_count = 10;  // Reduce test count for speed
        
        runner.add_property("large_doc_" + std::to_string(i), property, test_config);
    }
    
    // Monitor resource usage during execution
    std::vector<ResourceMetrics> metrics_history;
    std::thread monitor_thread([this, &metrics_history]() {
        while (resource_monitor_->is_running()) {
            metrics_history.push_back(resource_monitor_->get_current_metrics());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Run tests
    auto results = runner.run_all();
    
    // Stop monitoring
    resource_monitor_->stop();
    monitor_thread.join();
    
    // Verify resource constraints were respected
    double max_cpu = 0.0;
    double max_memory = 0.0;
    for (const auto& metrics : metrics_history) {
        max_cpu = std::max(max_cpu, metrics.cpu_usage_percent);
        max_memory = std::max(max_memory, metrics.memory_usage_percent);
    }
    
    EXPECT_LE(max_cpu, config.cpu_threshold + 0.1);  // Allow 10% tolerance
    EXPECT_LE(max_memory, config.memory_threshold + 0.1);
    
    // Verify test results
    for (const auto& result : results) {
        EXPECT_TRUE(result.success) << "Test " << result.test_name 
                                   << " failed: " << result.error_message;
    }
}

TEST_F(ParallelExecutionTest, TestDistribution) {
    // Create multiple runners
    std::vector<std::unique_ptr<ParallelTestRunner>> runners;
    for (int i = 0; i < 3; ++i) {
        ParallelTestRunner::Config config;
        config.num_threads = 2;
        runners.push_back(std::make_unique<ParallelTestRunner>(config));
    }
    
    // Create distributor with load balancing
    TestDistributor::DistributionStrategy strategy;
    strategy.type = TestDistributor::DistributionStrategy::LOAD_BALANCED;
    
    TestDistributor distributor(std::move(runners), strategy);
    
    // Add properties with different workloads
    for (int i = 0; i < 30; ++i) {
        if (i % 3 == 0) {
            // Heavy properties
            auto property = std::make_shared<LargeDocumentProperty>();
            distributor.add_property("heavy_" + std::to_string(i), property);
        } else if (i % 3 == 1) {
            // Medium properties
            auto property = std::make_shared<RoundTripProperty>();
            distributor.add_property("medium_" + std::to_string(i), property);
        } else {
            // Light properties
            auto property = std::make_shared<ParserSafetyProperty>();
            distributor.add_property("light_" + std::to_string(i), property);
        }
    }
    
    // Run distributed tests
    auto all_results = distributor.run_all();
    
    // Verify distribution
    ASSERT_EQ(all_results.size(), 3);  // 3 runners
    
    size_t total_tests = 0;
    for (const auto& runner_results : all_results) {
        total_tests += runner_results.size();
        
        // Each runner should have roughly equal load
        EXPECT_GE(runner_results.size(), 8);
        EXPECT_LE(runner_results.size(), 12);
        
        // Verify all tests passed
        for (const auto& result : runner_results) {
            EXPECT_TRUE(result.success) << "Test " << result.test_name 
                                       << " failed: " << result.error_message;
        }
    }
    
    EXPECT_EQ(total_tests, 30);
}

TEST_F(ParallelExecutionTest, LazyGeneratorPerformance) {
    ParallelTestRunner::Config config;
    config.num_threads = 4;
    
    ParallelTestRunner runner(config);
    
    // Test with eager generators
    auto start_eager = std::chrono::steady_clock::now();
    for (int i = 0; i < 10; ++i) {
        auto property = std::make_shared<DocumentGeneratorProperty>(
            jww_document(1, 5, 100)  // Eager generation
        );
        
        TestExecutionConfig test_config;
        test_config.max_test_count = 50;
        
        runner.add_property("eager_" + std::to_string(i), property, test_config);
    }
    
    auto eager_results = runner.run_all();
    auto eager_duration = std::chrono::steady_clock::now() - start_eager;
    
    // Test with lazy generators
    ParallelTestRunner lazy_runner(config);
    
    auto start_lazy = std::chrono::steady_clock::now();
    for (int i = 0; i < 10; ++i) {
        auto property = std::make_shared<DocumentGeneratorProperty>(
            lazy_jww_document(1, 5, 100).force()  // Lazy generation
        );
        
        TestExecutionConfig test_config;
        test_config.max_test_count = 50;
        
        lazy_runner.add_property("lazy_" + std::to_string(i), property, test_config);
    }
    
    auto lazy_results = lazy_runner.run_all();
    auto lazy_duration = std::chrono::steady_clock::now() - start_lazy;
    
    // Verify both produced correct results
    ASSERT_EQ(eager_results.size(), lazy_results.size());
    
    for (const auto& result : eager_results) {
        EXPECT_TRUE(result.success);
    }
    
    for (const auto& result : lazy_results) {
        EXPECT_TRUE(result.success);
    }
    
    // Log performance comparison
    std::cout << "Eager generation time: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(eager_duration).count() 
              << "ms\n";
    std::cout << "Lazy generation time: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(lazy_duration).count() 
              << "ms\n";
    
    // Lazy should not be significantly slower (allow 20% overhead)
    auto eager_ms = std::chrono::duration_cast<std::chrono::milliseconds>(eager_duration).count();
    auto lazy_ms = std::chrono::duration_cast<std::chrono::milliseconds>(lazy_duration).count();
    EXPECT_LE(lazy_ms, eager_ms * 1.2);
}

TEST_F(ParallelExecutionTest, CounterexamplePersistence) {
    ParallelTestRunner::Config config;
    config.num_threads = 2;
    
    ParallelTestRunner runner(config);
    
    // Add a property that will fail
    auto failing_property = std::make_shared<AlwaysFailProperty>();
    
    TestExecutionConfig test_config;
    test_config.max_test_count = 10;
    
    runner.add_property("failing_test", failing_property, test_config);
    
    // Run tests
    auto results = runner.run_all();
    
    // Verify test failed
    ASSERT_EQ(results.size(), 1);
    EXPECT_FALSE(results[0].success);
    EXPECT_TRUE(results[0].counterexample.has_value());
    
    // Store counterexample
    counterexample_db_->store(
        results[0].test_name,
        "AlwaysFailProperty",
        results[0].counterexample.value(),
        results[0].error_message
    );
    
    // Verify persistence
    auto stored = counterexample_db_->get_latest("failing_test");
    ASSERT_TRUE(stored.has_value());
    
    auto decompressed = counterexample_db_->decompress(stored.value());
    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(decompressed.value(), results[0].counterexample.value());
}

TEST_F(ParallelExecutionTest, AdaptiveResourceManagement) {
    // Create adaptive resource manager
    AdaptiveResourceManager::Config arm_config;
    arm_config.target_cpu_usage = 0.6;
    arm_config.target_memory_usage = 0.5;
    arm_config.adjustment_interval = std::chrono::milliseconds(1000);
    arm_config.enable_logging = true;
    
    AdaptiveResourceManager arm(resource_monitor_, arm_config);
    
    // Create thread pool with adaptive management
    ParallelTestRunner::Config runner_config;
    runner_config.num_threads = 8;
    runner_config.adaptive_scheduling = false;  // Let ARM handle it
    
    auto runner = std::make_unique<ParallelTestRunner>(runner_config);
    
    // Register thread pool with ARM
    AdaptiveResourceManager::ThreadPoolInfo pool_info{
        .name = "test_pool",
        .get_current_size = [&runner]() { return runner->get_active_threads(); },
        .set_size = [](size_t) { /* Would adjust thread pool size */ },
        .min_threads = 2,
        .max_threads = 8
    };
    
    arm.register_thread_pool(pool_info);
    arm.start();
    
    // Add varying workload
    for (int batch = 0; batch < 3; ++batch) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Add batch of properties
        for (int i = 0; i < 10; ++i) {
            auto property = std::make_shared<RoundTripProperty>();
            runner->add_property(
                "batch" + std::to_string(batch) + "_test" + std::to_string(i),
                property
            );
        }
    }
    
    // Run tests
    auto results = runner->run_all();
    
    // Stop ARM
    arm.stop();
    
    // Get pool states
    auto pool_states = arm.get_pool_states();
    ASSERT_EQ(pool_states.size(), 1);
    
    // Verify all tests passed
    for (const auto& result : results) {
        EXPECT_TRUE(result.success);
    }
}

// Test property for always failing (used in persistence test)
class AlwaysFailProperty : public PropertyBase<JWWDocument> {
public:
    bool check(const JWWDocument& doc) override {
        last_counterexample_ = doc;
        throw PBTException(ErrorCategory::PROPERTY_FAILED, "Always fails for testing");
    }
    
    std::string name() const override { return "AlwaysFailProperty"; }
    std::string description() const override { return "Property that always fails"; }
};

// Test property that uses a specific generator
class DocumentGeneratorProperty : public PropertyBase<JWWDocument> {
public:
    explicit DocumentGeneratorProperty(rc::Gen<JWWDocument> gen) 
        : generator_(std::move(gen)) {}
    
    bool check(const JWWDocument& doc) override {
        // Simple check - document should have at least one layer
        return !doc.layers.empty();
    }
    
    std::string name() const override { return "DocumentGeneratorProperty"; }
    std::string description() const override { return "Tests document generation"; }
    
    rc::Gen<JWWDocument> arbitrary() const override { return generator_; }
    
private:
    rc::Gen<JWWDocument> generator_;
};