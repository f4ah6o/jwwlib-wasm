#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <ctime>
#include <optional>

namespace jww {
namespace pbt {
namespace reporting {

struct TestResult {
    std::string property_name;
    bool passed;
    size_t test_cases_run;
    size_t test_cases_failed;
    std::chrono::milliseconds execution_time;
    std::optional<std::string> failure_message;
    std::optional<std::string> counterexample;
    std::unordered_map<std::string, std::string> metadata;
};

struct TestSuite {
    std::string name;
    std::string description;
    std::vector<TestResult> results;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::unordered_map<std::string, std::string> environment_info;
};

struct ReportConfig {
    std::filesystem::path output_directory;
    std::string report_title = "Property-Based Test Report";
    bool include_charts = true;
    bool include_coverage = true;
    bool include_timings = true;
    bool include_environment = true;
    std::string css_theme = "default";
};

class HtmlReportGenerator {
public:
    explicit HtmlReportGenerator(const ReportConfig& config);

    void addTestSuite(const TestSuite& suite);
    void generateReport();
    
    std::filesystem::path getReportPath() const;

private:
    ReportConfig config_;
    std::vector<TestSuite> test_suites_;
    std::filesystem::path report_path_;

    std::string generateHtml() const;
    std::string generateHeader() const;
    std::string generateSummary() const;
    std::string generateTestSuiteSection(const TestSuite& suite) const;
    std::string generateCharts() const;
    std::string generateCoverageSection() const;
    std::string generateEnvironmentInfo() const;
    std::string generateFooter() const;
    
    std::string generateCss() const;
    std::string generateJavaScript() const;
    
    std::string formatDuration(std::chrono::milliseconds duration) const;
    std::string formatTimestamp(std::chrono::system_clock::time_point time) const;
    std::string escapeHtml(const std::string& text) const;
    
    void copyAssets();
    void writeReport(const std::string& html_content);
};

class ChartDataGenerator {
public:
    struct ChartData {
        std::vector<std::string> labels;
        std::vector<double> values;
        std::string chart_type;
        std::string title;
    };

    static ChartData generateSuccessRateChart(const std::vector<TestSuite>& suites);
    static ChartData generateExecutionTimeChart(const std::vector<TestSuite>& suites);
    static ChartData generateTestCaseDistribution(const std::vector<TestSuite>& suites);
    static ChartData generatePropertyCoverageChart(const std::vector<TestSuite>& suites);
};

} // namespace reporting
} // namespace pbt
} // namespace jww