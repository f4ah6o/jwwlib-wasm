#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "../base/property_base.hpp"
#include "../base/test_execution_config.hpp"
#include "../error/pbt_exception.hpp"

namespace jwwlib {
namespace pbt {
namespace parallel {

struct TestTask {
    std::string name;
    std::function<void()> test_function;
    std::chrono::milliseconds timeout;
};

struct TestResult {
    std::string test_name;
    bool success;
    std::chrono::milliseconds duration;
    std::string error_message;
    std::optional<std::string> counterexample;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    std::future<void> enqueue(std::function<void()> task);
    void shutdown();
    size_t active_workers() const { return active_workers_.load(); }
    size_t queue_size() const;

private:
    std::vector<std::thread> workers_;
    std::queue<std::packaged_task<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> active_workers_{0};

    void worker_thread();
};

class ParallelTestRunner {
public:
    struct Config {
        size_t num_threads = std::thread::hardware_concurrency();
        size_t max_concurrent_tests = 0;  // 0 = unlimited
        std::chrono::milliseconds default_timeout{60000};  // 60 seconds
        bool adaptive_scheduling = true;
        double cpu_threshold = 0.8;  // 80% CPU usage threshold
        double memory_threshold = 0.8;  // 80% memory usage threshold
    };

    explicit ParallelTestRunner(const Config& config = Config{});
    ~ParallelTestRunner();

    template <typename T>
    void add_property(const std::string& name,
                      std::shared_ptr<PropertyBase<T>> property,
                      const TestExecutionConfig& test_config = {}) {
        TestTask task{
            .name = name,
            .test_function = [property, test_config]() {
                property->check(test_config);
            },
            .timeout = std::chrono::milliseconds(test_config.timeout_ms)
        };
        
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        pending_tasks_.push(std::move(task));
    }

    std::vector<TestResult> run_all();
    void stop();
    
    // Resource monitoring
    double get_cpu_usage() const;
    double get_memory_usage() const;
    size_t get_active_threads() const;
    size_t get_pending_tests() const;

private:
    Config config_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::queue<TestTask> pending_tasks_;
    std::vector<TestResult> results_;
    mutable std::mutex tasks_mutex_;
    mutable std::mutex results_mutex_;
    std::atomic<bool> stop_requested_{false};
    
    // Resource monitoring
    std::thread monitor_thread_;
    std::atomic<double> current_cpu_usage_{0.0};
    std::atomic<double> current_memory_usage_{0.0};
    
    void adjust_thread_pool_size();
    void monitor_resources();
    TestResult run_test_with_timeout(const TestTask& task);
    static double calculate_cpu_usage();
    static double calculate_memory_usage();
};

// Utility class for distributing tests across multiple runners
class TestDistributor {
public:
    struct DistributionStrategy {
        enum Type {
            ROUND_ROBIN,
            LOAD_BALANCED,
            AFFINITY_BASED
        };
        
        Type type = ROUND_ROBIN;
        std::function<size_t(const TestTask&, const std::vector<ParallelTestRunner*>&)> 
            custom_selector;
    };

    TestDistributor(std::vector<std::unique_ptr<ParallelTestRunner>> runners,
                    const DistributionStrategy& strategy = {});

    template <typename T>
    void add_property(const std::string& name,
                      std::shared_ptr<PropertyBase<T>> property,
                      const TestExecutionConfig& test_config = {}) {
        size_t runner_index = select_runner(name);
        runners_[runner_index]->add_property(name, property, test_config);
    }

    std::vector<std::vector<TestResult>> run_all();

private:
    std::vector<std::unique_ptr<ParallelTestRunner>> runners_;
    DistributionStrategy strategy_;
    std::atomic<size_t> round_robin_counter_{0};

    size_t select_runner(const std::string& test_name);
};

}  // namespace parallel
}  // namespace pbt
}  // namespace jwwlib