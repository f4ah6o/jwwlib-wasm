#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace jwwlib {
namespace pbt {
namespace parallel {

struct ResourceMetrics {
    double cpu_usage_percent;
    double memory_usage_percent;
    size_t memory_usage_bytes;
    size_t available_memory_bytes;
    size_t num_threads;
    std::chrono::system_clock::time_point timestamp;
};

class ResourceMonitor {
public:
    struct Config {
        std::chrono::milliseconds sampling_interval{500};
        size_t history_size = 60;  // Keep last 30 seconds of history
        bool enable_alerts = true;
        double cpu_alert_threshold = 0.9;      // 90%
        double memory_alert_threshold = 0.85;  // 85%
    };

    using AlertCallback = std::function<void(const ResourceMetrics&, const std::string&)>;

    explicit ResourceMonitor(const Config& config = Config{});
    ~ResourceMonitor();

    void start();
    void stop();
    bool is_running() const { return running_.load(); }

    // Get current metrics
    ResourceMetrics get_current_metrics() const;
    std::vector<ResourceMetrics> get_history() const;
    
    // Get aggregated metrics
    double get_average_cpu_usage(std::chrono::milliseconds window) const;
    double get_average_memory_usage(std::chrono::milliseconds window) const;
    double get_peak_cpu_usage(std::chrono::milliseconds window) const;
    double get_peak_memory_usage(std::chrono::milliseconds window) const;

    // Alert management
    void set_alert_callback(AlertCallback callback);
    void set_cpu_threshold(double threshold);
    void set_memory_threshold(double threshold);

    // Resource predictions
    std::chrono::milliseconds estimate_time_to_memory_limit(size_t limit_bytes) const;
    bool is_resource_constrained() const;

private:
    Config config_;
    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
    
    mutable std::mutex metrics_mutex_;
    std::vector<ResourceMetrics> history_;
    ResourceMetrics current_metrics_;
    
    AlertCallback alert_callback_;
    std::atomic<double> last_cpu_alert_level_{0.0};
    std::atomic<double> last_memory_alert_level_{0.0};
    
    void monitor_loop();
    static ResourceMetrics collect_metrics();
    void check_alerts(const ResourceMetrics& metrics);
    void add_to_history(const ResourceMetrics& metrics);
};

// Adaptive resource manager that adjusts thread pool sizes based on metrics
class AdaptiveResourceManager {
public:
    struct ThreadPoolInfo {
        std::string name;
        std::function<size_t()> get_current_size;
        std::function<void(size_t)> set_size;
        size_t min_threads = 1;
        size_t max_threads = std::thread::hardware_concurrency();
    };

    struct Config {
        double target_cpu_usage = 0.7;       // Try to maintain 70% CPU usage
        double target_memory_usage = 0.6;    // Try to maintain 60% memory usage
        std::chrono::milliseconds adjustment_interval{5000};  // Adjust every 5 seconds
        double adjustment_factor = 0.2;      // Adjust by 20% each time
        bool enable_logging = false;
    };

    AdaptiveResourceManager(std::shared_ptr<ResourceMonitor> monitor,
                           const Config& config = Config{});
    ~AdaptiveResourceManager();

    void register_thread_pool(const ThreadPoolInfo& pool_info);
    void start();
    void stop();

    // Manual controls
    void scale_up(const std::string& pool_name, size_t additional_threads = 1);
    void scale_down(const std::string& pool_name, size_t threads_to_remove = 1);
    void set_all_pools(size_t thread_count);

    // Get current state
    struct PoolState {
        std::string name;
        size_t current_threads;
        size_t min_threads;
        size_t max_threads;
    };
    std::vector<PoolState> get_pool_states() const;

private:
    std::shared_ptr<ResourceMonitor> resource_monitor_;
    Config config_;
    std::vector<ThreadPoolInfo> thread_pools_;
    std::atomic<bool> running_{false};
    std::thread management_thread_;
    mutable std::mutex pools_mutex_;
    
    void management_loop();
    void adjust_thread_pools();
    size_t calculate_optimal_threads(const ResourceMetrics& metrics,
                                   const ThreadPoolInfo& pool) const;
    void log_adjustment(const std::string& pool_name, 
                       size_t old_size, 
                       size_t new_size,
                       const std::string& reason);
};

}  // namespace parallel
}  // namespace pbt
}  // namespace jwwlib