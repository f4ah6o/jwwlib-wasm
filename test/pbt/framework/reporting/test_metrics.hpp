#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <optional>

namespace jww {
namespace pbt {
namespace reporting {

struct PropertyMetrics {
    std::string property_name;
    std::atomic<size_t> test_cases_generated{0};
    std::atomic<size_t> test_cases_passed{0};
    std::atomic<size_t> test_cases_failed{0};
    std::atomic<size_t> shrink_attempts{0};
    std::atomic<size_t> shrink_successes{0};
    
    std::chrono::milliseconds total_generation_time{0};
    std::chrono::milliseconds total_execution_time{0};
    std::chrono::milliseconds total_shrinking_time{0};
    
    size_t max_generated_size = 0;
    size_t min_counterexample_size = std::numeric_limits<size_t>::max();
    
    std::vector<double> generation_times;
    std::vector<double> execution_times;
    
    double getSuccessRate() const;
    double getAverageGenerationTime() const;
    double getAverageExecutionTime() const;
    double getShrinkingEfficiency() const;
};

struct MemoryMetrics {
    size_t peak_memory_usage = 0;
    size_t current_memory_usage = 0;
    size_t total_allocations = 0;
    size_t total_deallocations = 0;
    
    std::vector<std::pair<std::chrono::system_clock::time_point, size_t>> memory_timeline;
    
    void recordMemoryUsage();
};

struct CoverageMetrics {
    std::unordered_map<std::string, size_t> edge_case_hits;
    std::unordered_map<std::string, size_t> value_distribution;
    std::unordered_map<std::string, bool> property_coverage;
    
    size_t total_unique_inputs = 0;
    size_t total_edge_cases_found = 0;
    
    double getEdgeCaseCoverage() const;
    double getPropertyCoverage() const;
    std::vector<std::string> getMissingEdgeCases() const;
};

class MetricsCollector {
public:
    static MetricsCollector& getInstance();
    
    void startProperty(const std::string& property_name);
    void endProperty(const std::string& property_name, bool passed);
    
    void recordTestCase(const std::string& property_name,
                       size_t input_size,
                       std::chrono::milliseconds generation_time,
                       std::chrono::milliseconds execution_time,
                       bool passed);
    
    void recordShrinkAttempt(const std::string& property_name,
                            bool successful,
                            size_t new_size);
    
    void recordMemoryUsage(size_t current_usage);
    void recordEdgeCaseHit(const std::string& edge_case_name);
    void recordValueDistribution(const std::string& value_category);
    
    PropertyMetrics getPropertyMetrics(const std::string& property_name) const;
    std::vector<PropertyMetrics> getAllPropertyMetrics() const;
    
    MemoryMetrics getMemoryMetrics() const;
    CoverageMetrics getCoverageMetrics() const;
    
    void reset();
    void exportMetrics(const std::string& filename) const;
    
private:
    MetricsCollector() = default;
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, std::unique_ptr<PropertyMetrics>> property_metrics_;
    std::unique_ptr<MemoryMetrics> memory_metrics_;
    std::unique_ptr<CoverageMetrics> coverage_metrics_;
    
    std::chrono::system_clock::time_point collection_start_time_;
};

class MetricsReporter {
public:
    struct ReportOptions {
        bool include_property_breakdown = true;
        bool include_memory_analysis = true;
        bool include_coverage_report = true;
        bool include_performance_trends = true;
        bool include_recommendations = true;
        std::string format = "html"; // html, json, xml
    };
    
    static std::string generateReport(const MetricsCollector& collector,
                                     const ReportOptions& options = {});
    
    static std::string generateJsonReport(const MetricsCollector& collector);
    static std::string generateXmlReport(const MetricsCollector& collector);
    
private:
    static std::string generateHtmlReport(const MetricsCollector& collector,
                                         const ReportOptions& options);
    
    static std::string generatePropertyBreakdown(const std::vector<PropertyMetrics>& metrics);
    static std::string generateMemoryAnalysis(const MemoryMetrics& metrics);
    static std::string generateCoverageReport(const CoverageMetrics& metrics);
    static std::string generatePerformanceTrends(const std::vector<PropertyMetrics>& metrics);
    static std::string generateRecommendations(const MetricsCollector& collector);
};

class MetricsTimer {
public:
    explicit MetricsTimer(const std::string& property_name,
                         const std::string& phase);
    ~MetricsTimer();
    
    void stop();
    std::chrono::milliseconds elapsed() const;
    
private:
    std::string property_name_;
    std::string phase_;
    std::chrono::steady_clock::time_point start_time_;
    bool stopped_ = false;
};

} // namespace reporting
} // namespace pbt
} // namespace jww