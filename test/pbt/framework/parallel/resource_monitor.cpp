#include "resource_monitor.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>

#ifdef __linux__
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

namespace jwwlib {
namespace pbt {
namespace parallel {

// ResourceMonitor implementation
ResourceMonitor::ResourceMonitor(const Config& config) : config_(config) {
    history_.reserve(config.history_size);
}

ResourceMonitor::~ResourceMonitor() {
    stop();
}

void ResourceMonitor::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    
    monitor_thread_ = std::thread([this] { monitor_loop(); });
}

void ResourceMonitor::stop() {
    if (!running_.exchange(false)) {
        return;  // Not running
    }
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

ResourceMetrics ResourceMonitor::get_current_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return current_metrics_;
}

std::vector<ResourceMetrics> ResourceMonitor::get_history() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return history_;
}

double ResourceMonitor::get_average_cpu_usage(std::chrono::milliseconds window) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (history_.empty()) return 0.0;
    
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - window;
    
    double sum = 0.0;
    size_t count = 0;
    
    for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
        if (it->timestamp < cutoff) break;
        sum += it->cpu_usage_percent;
        ++count;
    }
    
    return count > 0 ? sum / count : 0.0;
}

double ResourceMonitor::get_average_memory_usage(std::chrono::milliseconds window) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (history_.empty()) return 0.0;
    
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - window;
    
    double sum = 0.0;
    size_t count = 0;
    
    for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
        if (it->timestamp < cutoff) break;
        sum += it->memory_usage_percent;
        ++count;
    }
    
    return count > 0 ? sum / count : 0.0;
}

double ResourceMonitor::get_peak_cpu_usage(std::chrono::milliseconds window) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (history_.empty()) return 0.0;
    
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - window;
    
    double peak = 0.0;
    
    for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
        if (it->timestamp < cutoff) break;
        peak = std::max(peak, it->cpu_usage_percent);
    }
    
    return peak;
}

double ResourceMonitor::get_peak_memory_usage(std::chrono::milliseconds window) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (history_.empty()) return 0.0;
    
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - window;
    
    double peak = 0.0;
    
    for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
        if (it->timestamp < cutoff) break;
        peak = std::max(peak, it->memory_usage_percent);
    }
    
    return peak;
}

void ResourceMonitor::set_alert_callback(AlertCallback callback) {
    alert_callback_ = std::move(callback);
}

void ResourceMonitor::set_cpu_threshold(double threshold) {
    config_.cpu_alert_threshold = threshold;
}

void ResourceMonitor::set_memory_threshold(double threshold) {
    config_.memory_alert_threshold = threshold;
}

std::chrono::milliseconds ResourceMonitor::estimate_time_to_memory_limit(
    size_t limit_bytes) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (history_.size() < 2) {
        return std::chrono::milliseconds::max();
    }
    
    // Calculate memory growth rate using linear regression
    std::vector<double> x_values, y_values;
    auto start_time = history_.front().timestamp;
    
    for (const auto& metric : history_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            metric.timestamp - start_time).count();
        x_values.push_back(static_cast<double>(elapsed));
        y_values.push_back(static_cast<double>(metric.memory_usage_bytes));
    }
    
    // Simple linear regression
    double n = static_cast<double>(x_values.size());
    double sum_x = std::accumulate(x_values.begin(), x_values.end(), 0.0);
    double sum_y = std::accumulate(y_values.begin(), y_values.end(), 0.0);
    double sum_xy = 0.0, sum_x2 = 0.0;
    
    for (size_t i = 0; i < x_values.size(); ++i) {
        sum_xy += x_values[i] * y_values[i];
        sum_x2 += x_values[i] * x_values[i];
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    
    if (slope <= 0) {
        return std::chrono::milliseconds::max();  // Memory not growing
    }
    
    double current_memory = history_.back().memory_usage_bytes;
    double time_to_limit = (limit_bytes - current_memory) / slope;
    
    if (time_to_limit < 0) {
        return std::chrono::milliseconds(0);  // Already exceeded
    }
    
    return std::chrono::milliseconds(static_cast<long>(time_to_limit));
}

bool ResourceMonitor::is_resource_constrained() const {
    auto metrics = get_current_metrics();
    return metrics.cpu_usage_percent > config_.cpu_alert_threshold ||
           metrics.memory_usage_percent > config_.memory_alert_threshold;
}

void ResourceMonitor::monitor_loop() {
    while (running_) {
        auto metrics = collect_metrics();
        
        {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            current_metrics_ = metrics;
            add_to_history(metrics);
        }
        
        if (config_.enable_alerts) {
            check_alerts(metrics);
        }
        
        std::this_thread::sleep_for(config_.sampling_interval);
    }
}

ResourceMetrics ResourceMonitor::collect_metrics() {
    ResourceMetrics metrics;
    metrics.timestamp = std::chrono::system_clock::now();
    
#ifdef __linux__
    // CPU usage
    static auto last_time = std::chrono::steady_clock::now();
    static double last_cpu_time = 0.0;
    
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        double cpu_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6 +
                         usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
        
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(current_time - last_time).count();
        
        if (elapsed > 0 && last_cpu_time > 0) {
            double cpu_usage = (cpu_time - last_cpu_time) / elapsed;
            metrics.cpu_usage_percent = std::min(1.0, cpu_usage / sysconf(_SC_NPROCESSORS_ONLN));
        }
        
        last_time = current_time;
        last_cpu_time = cpu_time;
    }
    
    // Memory usage
    std::ifstream status("/proc/self/status");
    std::string line;
    
    while (std::getline(status, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string label;
            long vm_rss_kb;
            iss >> label >> vm_rss_kb;
            metrics.memory_usage_bytes = vm_rss_kb * 1024;
            break;
        }
    }
    
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        metrics.available_memory_bytes = si.freeram * si.mem_unit;
        if (si.totalram > 0) {
            metrics.memory_usage_percent = 
                static_cast<double>(metrics.memory_usage_bytes) / 
                (si.totalram * si.mem_unit);
        }
    }
    
    // Thread count
    std::ifstream stat("/proc/self/stat");
    if (stat) {
        std::string stat_line;
        std::getline(stat, stat_line);
        std::istringstream iss(stat_line);
        
        // Skip to the num_threads field (20th field)
        std::string field;
        for (int i = 0; i < 19; ++i) {
            iss >> field;
        }
        iss >> metrics.num_threads;
    }
    
#elif defined(__APPLE__)
    // CPU usage
    static mach_port_t host_port = mach_host_self();
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(host_port, HOST_CPU_LOAD_INFO,
                       (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        static unsigned int last_user = 0, last_system = 0, last_idle = 0, last_nice = 0;
        
        unsigned int user = cpuinfo.cpu_ticks[CPU_STATE_USER] - last_user;
        unsigned int system = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] - last_system;
        unsigned int idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE] - last_idle;
        unsigned int nice = cpuinfo.cpu_ticks[CPU_STATE_NICE] - last_nice;
        
        unsigned int total_ticks = user + system + idle + nice;
        unsigned int used_ticks = user + system + nice;
        
        if (total_ticks > 0) {
            metrics.cpu_usage_percent = static_cast<double>(used_ticks) / total_ticks;
        }
        
        last_user = cpuinfo.cpu_ticks[CPU_STATE_USER];
        last_system = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
        last_idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
        last_nice = cpuinfo.cpu_ticks[CPU_STATE_NICE];
    }
    
    // Memory usage
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO, 
                  (task_info_t)&info, &size) == KERN_SUCCESS) {
        metrics.memory_usage_bytes = info.resident_size;
    }
    
    // Total and available memory
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t total_mem;
    size_t length = sizeof(total_mem);
    
    if (sysctl(mib, 2, &total_mem, &length, NULL, 0) == 0) {
        vm_statistics64_data_t vm_stat;
        mach_msg_type_number_t host_size = sizeof(vm_stat) / sizeof(natural_t);
        
        if (host_statistics64(host_port, VM_STATISTICS, (host_info64_t)&vm_stat, 
                             &host_size) == KERN_SUCCESS) {
            metrics.available_memory_bytes = 
                (vm_stat.free_count + vm_stat.inactive_count) * vm_page_size;
            metrics.memory_usage_percent = 
                static_cast<double>(metrics.memory_usage_bytes) / total_mem;
        }
    }
    
    // Thread count
    thread_array_t thread_list;
    mach_msg_type_number_t thread_count;
    
    if (task_threads(mach_task_self(), &thread_list, &thread_count) == KERN_SUCCESS) {
        metrics.num_threads = thread_count;
        vm_deallocate(mach_task_self(), (vm_address_t)thread_list, 
                     thread_count * sizeof(thread_t));
    }
#else
    // Fallback values for unsupported platforms
    metrics.cpu_usage_percent = 0.5;
    metrics.memory_usage_percent = 0.5;
    metrics.memory_usage_bytes = 100 * 1024 * 1024;  // 100MB
    metrics.available_memory_bytes = 1024 * 1024 * 1024;  // 1GB
    metrics.num_threads = std::thread::hardware_concurrency();
#endif
    
    return metrics;
}

void ResourceMonitor::check_alerts(const ResourceMetrics& metrics) {
    if (!alert_callback_) return;
    
    // CPU alert
    if (metrics.cpu_usage_percent > config_.cpu_alert_threshold) {
        double last_level = last_cpu_alert_level_.exchange(metrics.cpu_usage_percent);
        if (last_level <= config_.cpu_alert_threshold) {
            std::stringstream msg;
            msg << "CPU usage exceeded threshold: " 
                << std::fixed << std::setprecision(1) 
                << (metrics.cpu_usage_percent * 100) << "%";
            alert_callback_(metrics, msg.str());
        }
    } else {
        last_cpu_alert_level_ = metrics.cpu_usage_percent;
    }
    
    // Memory alert
    if (metrics.memory_usage_percent > config_.memory_alert_threshold) {
        double last_level = last_memory_alert_level_.exchange(metrics.memory_usage_percent);
        if (last_level <= config_.memory_alert_threshold) {
            std::stringstream msg;
            msg << "Memory usage exceeded threshold: " 
                << std::fixed << std::setprecision(1) 
                << (metrics.memory_usage_percent * 100) << "%";
            alert_callback_(metrics, msg.str());
        }
    } else {
        last_memory_alert_level_ = metrics.memory_usage_percent;
    }
}

void ResourceMonitor::add_to_history(const ResourceMetrics& metrics) {
    if (history_.size() >= config_.history_size) {
        history_.erase(history_.begin());
    }
    history_.push_back(metrics);
}

// AdaptiveResourceManager implementation
AdaptiveResourceManager::AdaptiveResourceManager(
    std::shared_ptr<ResourceMonitor> monitor,
    const Config& config)
    : resource_monitor_(monitor), config_(config) {}

AdaptiveResourceManager::~AdaptiveResourceManager() {
    stop();
}

void AdaptiveResourceManager::register_thread_pool(const ThreadPoolInfo& pool_info) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    thread_pools_.push_back(pool_info);
}

void AdaptiveResourceManager::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    
    if (!resource_monitor_->is_running()) {
        resource_monitor_->start();
    }
    
    management_thread_ = std::thread([this] { management_loop(); });
}

void AdaptiveResourceManager::stop() {
    if (!running_.exchange(false)) {
        return;  // Not running
    }
    
    if (management_thread_.joinable()) {
        management_thread_.join();
    }
}

void AdaptiveResourceManager::scale_up(const std::string& pool_name, 
                                      size_t additional_threads) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    auto it = std::find_if(thread_pools_.begin(), thread_pools_.end(),
                          [&pool_name](const ThreadPoolInfo& info) {
                              return info.name == pool_name;
                          });
    
    if (it != thread_pools_.end()) {
        size_t current = it->get_current_size();
        size_t new_size = std::min(current + additional_threads, it->max_threads);
        it->set_size(new_size);
        
        if (config_.enable_logging) {
            log_adjustment(pool_name, current, new_size, "Manual scale up");
        }
    }
}

void AdaptiveResourceManager::scale_down(const std::string& pool_name, 
                                        size_t threads_to_remove) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    auto it = std::find_if(thread_pools_.begin(), thread_pools_.end(),
                          [&pool_name](const ThreadPoolInfo& info) {
                              return info.name == pool_name;
                          });
    
    if (it != thread_pools_.end()) {
        size_t current = it->get_current_size();
        size_t new_size = (current > threads_to_remove) ? 
                         current - threads_to_remove : it->min_threads;
        new_size = std::max(new_size, it->min_threads);
        it->set_size(new_size);
        
        if (config_.enable_logging) {
            log_adjustment(pool_name, current, new_size, "Manual scale down");
        }
    }
}

void AdaptiveResourceManager::set_all_pools(size_t thread_count) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    for (auto& pool : thread_pools_) {
        size_t current = pool.get_current_size();
        size_t new_size = std::clamp(thread_count, pool.min_threads, pool.max_threads);
        pool.set_size(new_size);
        
        if (config_.enable_logging) {
            log_adjustment(pool.name, current, new_size, "Set all pools");
        }
    }
}

std::vector<AdaptiveResourceManager::PoolState> 
AdaptiveResourceManager::get_pool_states() const {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    std::vector<PoolState> states;
    
    for (const auto& pool : thread_pools_) {
        states.push_back({
            .name = pool.name,
            .current_threads = pool.get_current_size(),
            .min_threads = pool.min_threads,
            .max_threads = pool.max_threads
        });
    }
    
    return states;
}

void AdaptiveResourceManager::management_loop() {
    while (running_) {
        adjust_thread_pools();
        std::this_thread::sleep_for(config_.adjustment_interval);
    }
}

void AdaptiveResourceManager::adjust_thread_pools() {
    auto metrics = resource_monitor_->get_current_metrics();
    
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    for (auto& pool : thread_pools_) {
        size_t current_size = pool.get_current_size();
        size_t optimal_size = calculate_optimal_threads(metrics, pool);
        
        if (optimal_size != current_size) {
            pool.set_size(optimal_size);
            
            if (config_.enable_logging) {
                std::stringstream reason;
                reason << "Adaptive adjustment (CPU: " 
                       << std::fixed << std::setprecision(1) 
                       << (metrics.cpu_usage_percent * 100) 
                       << "%, Mem: " 
                       << (metrics.memory_usage_percent * 100) << "%)";
                log_adjustment(pool.name, current_size, optimal_size, reason.str());
            }
        }
    }
}

size_t AdaptiveResourceManager::calculate_optimal_threads(
    const ResourceMetrics& metrics,
    const ThreadPoolInfo& pool) const {
    
    size_t current_size = pool.get_current_size();
    
    // Calculate adjustment based on resource usage
    double cpu_ratio = metrics.cpu_usage_percent / config_.target_cpu_usage;
    double memory_ratio = metrics.memory_usage_percent / config_.target_memory_usage;
    
    // Use the more constraining factor
    double constraint_ratio = std::max(cpu_ratio, memory_ratio);
    
    size_t optimal_size;
    
    if (constraint_ratio > 1.1) {
        // Resources overused, scale down
        double scale_factor = 1.0 - config_.adjustment_factor * (constraint_ratio - 1.0);
        optimal_size = static_cast<size_t>(current_size * scale_factor);
    } else if (constraint_ratio < 0.9) {
        // Resources underused, scale up
        double scale_factor = 1.0 + config_.adjustment_factor * (1.0 - constraint_ratio);
        optimal_size = static_cast<size_t>(current_size * scale_factor);
    } else {
        // Within acceptable range
        optimal_size = current_size;
    }
    
    // Apply constraints
    optimal_size = std::clamp(optimal_size, pool.min_threads, pool.max_threads);
    
    // Avoid small adjustments
    if (std::abs(static_cast<int>(optimal_size) - static_cast<int>(current_size)) < 2) {
        optimal_size = current_size;
    }
    
    return optimal_size;
}

void AdaptiveResourceManager::log_adjustment(const std::string& pool_name, 
                                            size_t old_size, 
                                            size_t new_size,
                                            const std::string& reason) {
    std::cout << "[AdaptiveResourceManager] Thread pool '" << pool_name 
              << "' adjusted from " << old_size << " to " << new_size 
              << " threads. Reason: " << reason << std::endl;
}

}  // namespace parallel
}  // namespace pbt
}  // namespace jwwlib