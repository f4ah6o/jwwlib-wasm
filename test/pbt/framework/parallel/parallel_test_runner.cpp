#include "parallel_test_runner.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#ifdef __linux__
#include <sys/resource.h>
#include <sys/sysinfo.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

namespace jwwlib {
namespace pbt {
namespace parallel {

// ThreadPool implementation
ThreadPool::ThreadPool(size_t num_threads) {
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { worker_thread(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

std::future<void> ThreadPool::enqueue(std::function<void()> task) {
    auto packaged_task = std::packaged_task<void()>(std::move(task));
    auto future = packaged_task.get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw PBTException(ErrorCategory::RUNTIME_ERROR, 
                             "Cannot enqueue task to stopped thread pool");
        }
        tasks_.push(std::move(packaged_task));
    }
    
    condition_.notify_one();
    return future;
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    
    condition_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

size_t ThreadPool::queue_size() const {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

void ThreadPool::worker_thread() {
    while (true) {
        std::packaged_task<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            
            if (stop_ && tasks_.empty()) {
                return;
            }
            
            task = std::move(tasks_.front());
            tasks_.pop();
            active_workers_++;
        }
        
        try {
            task();
        } catch (...) {
            // Task exceptions are handled by the future
        }
        
        active_workers_--;
    }
}

// ParallelTestRunner implementation
ParallelTestRunner::ParallelTestRunner(const Config& config)
    : config_(config) {
    size_t num_threads = config.num_threads;
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 4;  // Fallback
        }
    }
    
    thread_pool_ = std::make_unique<ThreadPool>(num_threads);
    
    if (config.adaptive_scheduling) {
        monitor_thread_ = std::thread([this] { monitor_resources(); });
    }
}

ParallelTestRunner::~ParallelTestRunner() {
    stop();
}

std::vector<TestResult> ParallelTestRunner::run_all() {
    std::vector<std::future<TestResult>> futures;
    
    // Submit all tests
    while (true) {
        TestTask task;
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            if (pending_tasks_.empty()) {
                break;
            }
            task = std::move(pending_tasks_.front());
            pending_tasks_.pop();
        }
        
        // Check resource constraints
        if (config_.adaptive_scheduling) {
            while (thread_pool_->active_workers() >= config_.max_concurrent_tests &&
                   config_.max_concurrent_tests > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            // Wait if resources are constrained
            while (current_cpu_usage_ > config_.cpu_threshold ||
                   current_memory_usage_ > config_.memory_threshold) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        auto future = thread_pool_->enqueue([this, task]() -> TestResult {
            return run_test_with_timeout(task);
        });
        
        futures.push_back(std::move(future));
    }
    
    // Collect results
    std::vector<TestResult> all_results;
    for (auto& future : futures) {
        try {
            all_results.push_back(future.get());
        } catch (const std::exception& e) {
            // This shouldn't happen as exceptions are caught in run_test_with_timeout
            TestResult error_result{
                .test_name = "Unknown",
                .success = false,
                .duration = std::chrono::milliseconds(0),
                .error_message = std::string("Unexpected error: ") + e.what()
            };
            all_results.push_back(error_result);
        }
    }
    
    return all_results;
}

void ParallelTestRunner::stop() {
    stop_requested_ = true;
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    if (thread_pool_) {
        thread_pool_->shutdown();
    }
}

double ParallelTestRunner::get_cpu_usage() const {
    return current_cpu_usage_.load();
}

double ParallelTestRunner::get_memory_usage() const {
    return current_memory_usage_.load();
}

size_t ParallelTestRunner::get_active_threads() const {
    return thread_pool_ ? thread_pool_->active_workers() : 0;
}

size_t ParallelTestRunner::get_pending_tests() const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    return pending_tasks_.size();
}

void ParallelTestRunner::adjust_thread_pool_size() {
    // Dynamic thread pool adjustment based on system resources
    // This is a placeholder - actual implementation would require
    // recreating the thread pool with a new size
}

void ParallelTestRunner::monitor_resources() {
    while (!stop_requested_) {
        current_cpu_usage_ = calculate_cpu_usage();
        current_memory_usage_ = calculate_memory_usage();
        
        if (config_.adaptive_scheduling) {
            adjust_thread_pool_size();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

TestResult ParallelTestRunner::run_test_with_timeout(const TestTask& task) {
    auto start_time = std::chrono::steady_clock::now();
    TestResult result{
        .test_name = task.name,
        .success = true,
        .duration = std::chrono::milliseconds(0)
    };
    
    // Create a promise/future pair for the test execution
    std::promise<void> test_promise;
    std::future<void> test_future = test_promise.get_future();
    
    // Run the test in a separate thread
    std::thread test_thread([&task, &result, &test_promise]() {
        try {
            task.test_function();
            test_promise.set_value();
        } catch (const PBTException& e) {
            result.success = false;
            result.error_message = e.what();
            if (e.has_counterexample()) {
                result.counterexample = e.get_counterexample();
            }
            test_promise.set_exception(std::current_exception());
        } catch (const std::exception& e) {
            result.success = false;
            result.error_message = e.what();
            test_promise.set_exception(std::current_exception());
        } catch (...) {
            result.success = false;
            result.error_message = "Unknown error";
            test_promise.set_exception(std::current_exception());
        }
    });
    
    // Wait for the test with timeout
    if (test_future.wait_for(task.timeout) == std::future_status::timeout) {
        result.success = false;
        result.error_message = "Test timeout exceeded";
        
        // Detach the thread - we can't safely terminate it
        test_thread.detach();
    } else {
        test_thread.join();
        
        // Check if an exception was thrown
        try {
            test_future.get();
        } catch (...) {
            // Exception details already captured in result
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    return result;
}

double ParallelTestRunner::calculate_cpu_usage() {
#ifdef __linux__
    static auto last_time = std::chrono::steady_clock::now();
    static double last_cpu_time = 0.0;
    
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        double cpu_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6 +
                         usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
        
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(current_time - last_time).count();
        
        double cpu_usage = 0.0;
        if (elapsed > 0) {
            cpu_usage = (cpu_time - last_cpu_time) / elapsed;
        }
        
        last_time = current_time;
        last_cpu_time = cpu_time;
        
        return std::min(1.0, cpu_usage);
    }
#elif defined(__APPLE__)
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                       (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        static unsigned int last_user = 0, last_system = 0, last_idle = 0;
        
        unsigned int user = cpuinfo.cpu_ticks[CPU_STATE_USER];
        unsigned int system = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
        unsigned int idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
        
        unsigned int total_ticks = (user - last_user) + (system - last_system) + 
                                  (idle - last_idle);
        unsigned int used_ticks = (user - last_user) + (system - last_system);
        
        last_user = user;
        last_system = system;
        last_idle = idle;
        
        if (total_ticks > 0) {
            return static_cast<double>(used_ticks) / total_ticks;
        }
    }
#endif
    return 0.5;  // Default fallback
}

double ParallelTestRunner::calculate_memory_usage() {
#ifdef __linux__
    std::ifstream status("/proc/self/status");
    std::string line;
    
    long vm_rss = 0;
    while (std::getline(status, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string label;
            iss >> label >> vm_rss;
            break;
        }
    }
    
    struct sysinfo si;
    if (sysinfo(&si) == 0 && si.totalram > 0) {
        double total_mem = si.totalram * si.mem_unit / 1024.0;  // KB
        return vm_rss / total_mem;
    }
#elif defined(__APPLE__)
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO, 
                  (task_info_t)&info, &size) == KERN_SUCCESS) {
        int mib[2] = {CTL_HW, HW_MEMSIZE};
        uint64_t total_mem;
        size_t length = sizeof(total_mem);
        
        if (sysctl(mib, 2, &total_mem, &length, NULL, 0) == 0) {
            return static_cast<double>(info.resident_size) / total_mem;
        }
    }
#endif
    return 0.5;  // Default fallback
}

// TestDistributor implementation
TestDistributor::TestDistributor(
    std::vector<std::unique_ptr<ParallelTestRunner>> runners,
    const DistributionStrategy& strategy)
    : runners_(std::move(runners)), strategy_(strategy) {
    if (runners_.empty()) {
        throw PBTException(ErrorCategory::INVALID_ARGUMENT,
                         "TestDistributor requires at least one runner");
    }
}

std::vector<std::vector<TestResult>> TestDistributor::run_all() {
    std::vector<std::future<std::vector<TestResult>>> futures;
    
    for (auto& runner : runners_) {
        futures.push_back(std::async(std::launch::async, [&runner]() {
            return runner->run_all();
        }));
    }
    
    std::vector<std::vector<TestResult>> all_results;
    for (auto& future : futures) {
        all_results.push_back(future.get());
    }
    
    return all_results;
}

size_t TestDistributor::select_runner(const std::string& test_name) {
    switch (strategy_.type) {
        case DistributionStrategy::ROUND_ROBIN: {
            size_t index = round_robin_counter_.fetch_add(1) % runners_.size();
            return index;
        }
        
        case DistributionStrategy::LOAD_BALANCED: {
            // Select runner with least pending tests
            size_t best_index = 0;
            size_t min_pending = std::numeric_limits<size_t>::max();
            
            for (size_t i = 0; i < runners_.size(); ++i) {
                size_t pending = runners_[i]->get_pending_tests();
                if (pending < min_pending) {
                    min_pending = pending;
                    best_index = i;
                }
            }
            return best_index;
        }
        
        case DistributionStrategy::AFFINITY_BASED: {
            if (strategy_.custom_selector) {
                std::vector<ParallelTestRunner*> runner_ptrs;
                for (auto& runner : runners_) {
                    runner_ptrs.push_back(runner.get());
                }
                
                TestTask dummy_task{.name = test_name};
                return strategy_.custom_selector(dummy_task, runner_ptrs);
            }
            // Fall back to round-robin
            return round_robin_counter_.fetch_add(1) % runners_.size();
        }
        
        default:
            return 0;
    }
}

}  // namespace parallel
}  // namespace pbt
}  // namespace jwwlib