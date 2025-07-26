#include "test_metrics.hpp"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <fstream>

namespace jww {
namespace pbt {
namespace reporting {

double PropertyMetrics::getSuccessRate() const {
    size_t total = test_cases_passed + test_cases_failed;
    return total > 0 ? (static_cast<double>(test_cases_passed) / total) * 100.0 : 0.0;
}

double PropertyMetrics::getAverageGenerationTime() const {
    return generation_times.empty() ? 0.0 :
        std::accumulate(generation_times.begin(), generation_times.end(), 0.0) / generation_times.size();
}

double PropertyMetrics::getAverageExecutionTime() const {
    return execution_times.empty() ? 0.0 :
        std::accumulate(execution_times.begin(), execution_times.end(), 0.0) / execution_times.size();
}

double PropertyMetrics::getShrinkingEfficiency() const {
    return shrink_attempts > 0 ? 
        (static_cast<double>(shrink_successes) / shrink_attempts) * 100.0 : 0.0;
}

void MemoryMetrics::recordMemoryUsage() {
    auto now = std::chrono::system_clock::now();
    memory_timeline.push_back({now, current_memory_usage});
    
    if (current_memory_usage > peak_memory_usage) {
        peak_memory_usage = current_memory_usage;
    }
}

double CoverageMetrics::getEdgeCaseCoverage() const {
    if (edge_case_hits.empty()) return 0.0;
    
    size_t covered = 0;
    for (const auto& [name, hits] : edge_case_hits) {
        if (hits > 0) covered++;
    }
    
    return (static_cast<double>(covered) / edge_case_hits.size()) * 100.0;
}

double CoverageMetrics::getPropertyCoverage() const {
    if (property_coverage.empty()) return 0.0;
    
    size_t covered = std::count_if(property_coverage.begin(), property_coverage.end(),
                                  [](const auto& pair) { return pair.second; });
    
    return (static_cast<double>(covered) / property_coverage.size()) * 100.0;
}

std::vector<std::string> CoverageMetrics::getMissingEdgeCases() const {
    std::vector<std::string> missing;
    
    for (const auto& [name, hits] : edge_case_hits) {
        if (hits == 0) {
            missing.push_back(name);
        }
    }
    
    return missing;
}

MetricsCollector& MetricsCollector::getInstance() {
    static MetricsCollector instance;
    return instance;
}

void MetricsCollector::startProperty(const std::string& property_name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (property_metrics_.find(property_name) == property_metrics_.end()) {
        property_metrics_[property_name] = std::make_unique<PropertyMetrics>();
        property_metrics_[property_name]->property_name = property_name;
    }
    
    if (!memory_metrics_) {
        memory_metrics_ = std::make_unique<MemoryMetrics>();
    }
    
    if (!coverage_metrics_) {
        coverage_metrics_ = std::make_unique<CoverageMetrics>();
    }
    
    if (collection_start_time_ == std::chrono::system_clock::time_point{}) {
        collection_start_time_ = std::chrono::system_clock::now();
    }
}

void MetricsCollector::endProperty(const std::string& property_name, bool passed) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (coverage_metrics_) {
        coverage_metrics_->property_coverage[property_name] = passed;
    }
}

void MetricsCollector::recordTestCase(const std::string& property_name,
                                     size_t input_size,
                                     std::chrono::milliseconds generation_time,
                                     std::chrono::milliseconds execution_time,
                                     bool passed) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = property_metrics_.find(property_name);
    if (it == property_metrics_.end()) {
        return;
    }
    
    auto& metrics = *it->second;
    
    metrics.test_cases_generated++;
    if (passed) {
        metrics.test_cases_passed++;
    } else {
        metrics.test_cases_failed++;
    }
    
    metrics.total_generation_time += generation_time;
    metrics.total_execution_time += execution_time;
    
    metrics.generation_times.push_back(generation_time.count());
    metrics.execution_times.push_back(execution_time.count());
    
    if (input_size > metrics.max_generated_size) {
        metrics.max_generated_size = input_size;
    }
    
    if (coverage_metrics_) {
        coverage_metrics_->total_unique_inputs++;
    }
}

void MetricsCollector::recordShrinkAttempt(const std::string& property_name,
                                          bool successful,
                                          size_t new_size) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = property_metrics_.find(property_name);
    if (it == property_metrics_.end()) {
        return;
    }
    
    auto& metrics = *it->second;
    
    metrics.shrink_attempts++;
    if (successful) {
        metrics.shrink_successes++;
        if (new_size < metrics.min_counterexample_size) {
            metrics.min_counterexample_size = new_size;
        }
    }
}

void MetricsCollector::recordMemoryUsage(size_t current_usage) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (!memory_metrics_) {
        memory_metrics_ = std::make_unique<MemoryMetrics>();
    }
    
    memory_metrics_->current_memory_usage = current_usage;
    memory_metrics_->recordMemoryUsage();
}

void MetricsCollector::recordEdgeCaseHit(const std::string& edge_case_name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (!coverage_metrics_) {
        coverage_metrics_ = std::make_unique<CoverageMetrics>();
    }
    
    coverage_metrics_->edge_case_hits[edge_case_name]++;
    coverage_metrics_->total_edge_cases_found++;
}

void MetricsCollector::recordValueDistribution(const std::string& value_category) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (!coverage_metrics_) {
        coverage_metrics_ = std::make_unique<CoverageMetrics>();
    }
    
    coverage_metrics_->value_distribution[value_category]++;
}

PropertyMetrics MetricsCollector::getPropertyMetrics(const std::string& property_name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = property_metrics_.find(property_name);
    if (it != property_metrics_.end()) {
        return *it->second;
    }
    
    return PropertyMetrics{};
}

std::vector<PropertyMetrics> MetricsCollector::getAllPropertyMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::vector<PropertyMetrics> all_metrics;
    for (const auto& [name, metrics] : property_metrics_) {
        all_metrics.push_back(*metrics);
    }
    
    return all_metrics;
}

MemoryMetrics MetricsCollector::getMemoryMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (memory_metrics_) {
        return *memory_metrics_;
    }
    
    return MemoryMetrics{};
}

CoverageMetrics MetricsCollector::getCoverageMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (coverage_metrics_) {
        return *coverage_metrics_;
    }
    
    return CoverageMetrics{};
}

void MetricsCollector::reset() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    property_metrics_.clear();
    memory_metrics_.reset();
    coverage_metrics_.reset();
    collection_start_time_ = std::chrono::system_clock::time_point{};
}

void MetricsCollector::exportMetrics(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to open metrics file: " + filename);
    }
    
    file << MetricsReporter::generateJsonReport(*this);
}

std::string MetricsReporter::generateReport(const MetricsCollector& collector,
                                           const ReportOptions& options) {
    if (options.format == "json") {
        return generateJsonReport(collector);
    } else if (options.format == "xml") {
        return generateXmlReport(collector);
    } else {
        return generateHtmlReport(collector, options);
    }
}

std::string MetricsReporter::generateHtmlReport(const MetricsCollector& collector,
                                               const ReportOptions& options) {
    std::ostringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "<title>Property-Based Testing Metrics Report</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html << "table { border-collapse: collapse; width: 100%; margin: 20px 0; }\n";
    html << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    html << "th { background-color: #f2f2f2; }\n";
    html << ".metric-value { font-weight: bold; color: #2c3e50; }\n";
    html << ".success { color: #27ae60; }\n";
    html << ".failure { color: #e74c3c; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";
    
    html << "<h1>Property-Based Testing Metrics Report</h1>\n";
    
    auto all_metrics = collector.getAllPropertyMetrics();
    
    if (options.include_property_breakdown) {
        html << generatePropertyBreakdown(all_metrics);
    }
    
    if (options.include_memory_analysis) {
        html << generateMemoryAnalysis(collector.getMemoryMetrics());
    }
    
    if (options.include_coverage_report) {
        html << generateCoverageReport(collector.getCoverageMetrics());
    }
    
    if (options.include_performance_trends) {
        html << generatePerformanceTrends(all_metrics);
    }
    
    if (options.include_recommendations) {
        html << generateRecommendations(collector);
    }
    
    html << "</body>\n</html>\n";
    
    return html.str();
}

std::string MetricsReporter::generatePropertyBreakdown(const std::vector<PropertyMetrics>& metrics) {
    std::ostringstream html;
    
    html << "<h2>プロパティ別メトリクス</h2>\n";
    html << "<table>\n";
    html << "<tr>\n";
    html << "<th>プロパティ名</th>\n";
    html << "<th>生成ケース数</th>\n";
    html << "<th>成功数</th>\n";
    html << "<th>失敗数</th>\n";
    html << "<th>成功率</th>\n";
    html << "<th>平均生成時間</th>\n";
    html << "<th>平均実行時間</th>\n";
    html << "<th>縮小効率</th>\n";
    html << "</tr>\n";
    
    for (const auto& metric : metrics) {
        html << "<tr>\n";
        html << "<td>" << metric.property_name << "</td>\n";
        html << "<td class=\"metric-value\">" << metric.test_cases_generated << "</td>\n";
        html << "<td class=\"metric-value success\">" << metric.test_cases_passed << "</td>\n";
        html << "<td class=\"metric-value failure\">" << metric.test_cases_failed << "</td>\n";
        html << "<td class=\"metric-value\">" << std::fixed << std::setprecision(1) 
             << metric.getSuccessRate() << "%</td>\n";
        html << "<td class=\"metric-value\">" << std::fixed << std::setprecision(2) 
             << metric.getAverageGenerationTime() << " ms</td>\n";
        html << "<td class=\"metric-value\">" << std::fixed << std::setprecision(2) 
             << metric.getAverageExecutionTime() << " ms</td>\n";
        html << "<td class=\"metric-value\">" << std::fixed << std::setprecision(1) 
             << metric.getShrinkingEfficiency() << "%</td>\n";
        html << "</tr>\n";
    }
    
    html << "</table>\n";
    
    return html.str();
}

std::string MetricsReporter::generateMemoryAnalysis(const MemoryMetrics& metrics) {
    std::ostringstream html;
    
    html << "<h2>メモリ使用状況分析</h2>\n";
    html << "<table>\n";
    html << "<tr><td>ピークメモリ使用量</td><td class=\"metric-value\">" 
         << (metrics.peak_memory_usage / 1024.0 / 1024.0) << " MB</td></tr>\n";
    html << "<tr><td>現在のメモリ使用量</td><td class=\"metric-value\">" 
         << (metrics.current_memory_usage / 1024.0 / 1024.0) << " MB</td></tr>\n";
    html << "<tr><td>総アロケーション数</td><td class=\"metric-value\">" 
         << metrics.total_allocations << "</td></tr>\n";
    html << "<tr><td>総デアロケーション数</td><td class=\"metric-value\">" 
         << metrics.total_deallocations << "</td></tr>\n";
    html << "</table>\n";
    
    return html.str();
}

std::string MetricsReporter::generateCoverageReport(const CoverageMetrics& metrics) {
    std::ostringstream html;
    
    html << "<h2>カバレッジレポート</h2>\n";
    html << "<table>\n";
    html << "<tr><td>エッジケースカバレッジ</td><td class=\"metric-value\">" 
         << std::fixed << std::setprecision(1) << metrics.getEdgeCaseCoverage() << "%</td></tr>\n";
    html << "<tr><td>プロパティカバレッジ</td><td class=\"metric-value\">" 
         << std::fixed << std::setprecision(1) << metrics.getPropertyCoverage() << "%</td></tr>\n";
    html << "<tr><td>ユニーク入力数</td><td class=\"metric-value\">" 
         << metrics.total_unique_inputs << "</td></tr>\n";
    html << "<tr><td>発見されたエッジケース数</td><td class=\"metric-value\">" 
         << metrics.total_edge_cases_found << "</td></tr>\n";
    html << "</table>\n";
    
    auto missing = metrics.getMissingEdgeCases();
    if (!missing.empty()) {
        html << "<h3>未検出のエッジケース</h3>\n";
        html << "<ul>\n";
        for (const auto& edge_case : missing) {
            html << "<li>" << edge_case << "</li>\n";
        }
        html << "</ul>\n";
    }
    
    return html.str();
}

std::string MetricsReporter::generatePerformanceTrends(const std::vector<PropertyMetrics>& metrics) {
    std::ostringstream html;
    
    html << "<h2>パフォーマンストレンド</h2>\n";
    
    if (metrics.empty()) {
        html << "<p>データがありません</p>\n";
        return html.str();
    }
    
    double total_gen_time = 0.0;
    double total_exec_time = 0.0;
    size_t total_cases = 0;
    
    for (const auto& metric : metrics) {
        total_gen_time += metric.total_generation_time.count();
        total_exec_time += metric.total_execution_time.count();
        total_cases += metric.test_cases_generated;
    }
    
    html << "<p>総テストケース数: <span class=\"metric-value\">" << total_cases << "</span></p>\n";
    html << "<p>総生成時間: <span class=\"metric-value\">" 
         << std::fixed << std::setprecision(2) << (total_gen_time / 1000.0) << " 秒</span></p>\n";
    html << "<p>総実行時間: <span class=\"metric-value\">" 
         << std::fixed << std::setprecision(2) << (total_exec_time / 1000.0) << " 秒</span></p>\n";
    
    return html.str();
}

std::string MetricsReporter::generateRecommendations(const MetricsCollector& collector) {
    std::ostringstream html;
    
    html << "<h2>推奨事項</h2>\n";
    html << "<ul>\n";
    
    auto metrics = collector.getAllPropertyMetrics();
    auto coverage = collector.getCoverageMetrics();
    
    for (const auto& metric : metrics) {
        if (metric.getSuccessRate() < 90.0) {
            html << "<li>プロパティ \"" << metric.property_name 
                 << "\" の成功率が低いです（" << std::fixed << std::setprecision(1) 
                 << metric.getSuccessRate() << "%）。プロパティ定義を見直してください。</li>\n";
        }
        
        if (metric.getAverageExecutionTime() > 1000.0) {
            html << "<li>プロパティ \"" << metric.property_name 
                 << "\" の実行時間が長いです（平均 " << std::fixed << std::setprecision(2) 
                 << metric.getAverageExecutionTime() << " ms）。最適化を検討してください。</li>\n";
        }
    }
    
    if (coverage.getEdgeCaseCoverage() < 80.0) {
        html << "<li>エッジケースカバレッジが低いです（" << std::fixed << std::setprecision(1) 
             << coverage.getEdgeCaseCoverage() << "%）。ジェネレータの改善を検討してください。</li>\n";
    }
    
    html << "</ul>\n";
    
    return html.str();
}

std::string MetricsReporter::generateJsonReport(const MetricsCollector& collector) {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"properties\": [\n";
    
    auto metrics = collector.getAllPropertyMetrics();
    for (size_t i = 0; i < metrics.size(); ++i) {
        const auto& metric = metrics[i];
        json << "    {\n";
        json << "      \"name\": \"" << metric.property_name << "\",\n";
        json << "      \"test_cases_generated\": " << metric.test_cases_generated << ",\n";
        json << "      \"test_cases_passed\": " << metric.test_cases_passed << ",\n";
        json << "      \"test_cases_failed\": " << metric.test_cases_failed << ",\n";
        json << "      \"success_rate\": " << metric.getSuccessRate() << ",\n";
        json << "      \"average_generation_time\": " << metric.getAverageGenerationTime() << ",\n";
        json << "      \"average_execution_time\": " << metric.getAverageExecutionTime() << ",\n";
        json << "      \"shrinking_efficiency\": " << metric.getShrinkingEfficiency() << "\n";
        json << "    }";
        if (i < metrics.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ],\n";
    
    auto memory = collector.getMemoryMetrics();
    json << "  \"memory\": {\n";
    json << "    \"peak_usage\": " << memory.peak_memory_usage << ",\n";
    json << "    \"current_usage\": " << memory.current_memory_usage << ",\n";
    json << "    \"total_allocations\": " << memory.total_allocations << ",\n";
    json << "    \"total_deallocations\": " << memory.total_deallocations << "\n";
    json << "  },\n";
    
    auto coverage = collector.getCoverageMetrics();
    json << "  \"coverage\": {\n";
    json << "    \"edge_case_coverage\": " << coverage.getEdgeCaseCoverage() << ",\n";
    json << "    \"property_coverage\": " << coverage.getPropertyCoverage() << ",\n";
    json << "    \"unique_inputs\": " << coverage.total_unique_inputs << ",\n";
    json << "    \"edge_cases_found\": " << coverage.total_edge_cases_found << "\n";
    json << "  }\n";
    json << "}\n";
    
    return json.str();
}

std::string MetricsReporter::generateXmlReport(const MetricsCollector& collector) {
    std::ostringstream xml;
    
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<pbt-metrics>\n";
    
    auto metrics = collector.getAllPropertyMetrics();
    xml << "  <properties>\n";
    for (const auto& metric : metrics) {
        xml << "    <property name=\"" << metric.property_name << "\">\n";
        xml << "      <test-cases-generated>" << metric.test_cases_generated << "</test-cases-generated>\n";
        xml << "      <test-cases-passed>" << metric.test_cases_passed << "</test-cases-passed>\n";
        xml << "      <test-cases-failed>" << metric.test_cases_failed << "</test-cases-failed>\n";
        xml << "      <success-rate>" << metric.getSuccessRate() << "</success-rate>\n";
        xml << "      <average-generation-time>" << metric.getAverageGenerationTime() << "</average-generation-time>\n";
        xml << "      <average-execution-time>" << metric.getAverageExecutionTime() << "</average-execution-time>\n";
        xml << "      <shrinking-efficiency>" << metric.getShrinkingEfficiency() << "</shrinking-efficiency>\n";
        xml << "    </property>\n";
    }
    xml << "  </properties>\n";
    
    auto memory = collector.getMemoryMetrics();
    xml << "  <memory>\n";
    xml << "    <peak-usage>" << memory.peak_memory_usage << "</peak-usage>\n";
    xml << "    <current-usage>" << memory.current_memory_usage << "</current-usage>\n";
    xml << "    <total-allocations>" << memory.total_allocations << "</total-allocations>\n";
    xml << "    <total-deallocations>" << memory.total_deallocations << "</total-deallocations>\n";
    xml << "  </memory>\n";
    
    auto coverage = collector.getCoverageMetrics();
    xml << "  <coverage>\n";
    xml << "    <edge-case-coverage>" << coverage.getEdgeCaseCoverage() << "</edge-case-coverage>\n";
    xml << "    <property-coverage>" << coverage.getPropertyCoverage() << "</property-coverage>\n";
    xml << "    <unique-inputs>" << coverage.total_unique_inputs << "</unique-inputs>\n";
    xml << "    <edge-cases-found>" << coverage.total_edge_cases_found << "</edge-cases-found>\n";
    xml << "  </coverage>\n";
    
    xml << "</pbt-metrics>\n";
    
    return xml.str();
}

MetricsTimer::MetricsTimer(const std::string& property_name,
                          const std::string& phase)
    : property_name_(property_name),
      phase_(phase),
      start_time_(std::chrono::steady_clock::now()) {
}

MetricsTimer::~MetricsTimer() {
    if (!stopped_) {
        stop();
    }
}

void MetricsTimer::stop() {
    if (!stopped_) {
        stopped_ = true;
    }
}

std::chrono::milliseconds MetricsTimer::elapsed() const {
    auto end_time = stopped_ ? std::chrono::steady_clock::now() : std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
}

} // namespace reporting
} // namespace pbt
} // namespace jww