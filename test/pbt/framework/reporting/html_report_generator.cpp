#include "html_report_generator.hpp"
#include <algorithm>
#include <numeric>

namespace jww {
namespace pbt {
namespace reporting {

HtmlReportGenerator::HtmlReportGenerator(const ReportConfig& config)
    : config_(config) {
    if (!std::filesystem::exists(config_.output_directory)) {
        std::filesystem::create_directories(config_.output_directory);
    }
}

void HtmlReportGenerator::addTestSuite(const TestSuite& suite) {
    test_suites_.push_back(suite);
}

void HtmlReportGenerator::generateReport() {
    auto timestamp = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream filename;
    filename << "pbt_report_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".html";
    
    report_path_ = config_.output_directory / filename.str();
    
    std::string html_content = generateHtml();
    writeReport(html_content);
    
    if (config_.include_charts) {
        copyAssets();
    }
}

std::filesystem::path HtmlReportGenerator::getReportPath() const {
    return report_path_;
}

std::string HtmlReportGenerator::generateHtml() const {
    std::ostringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"ja\">\n";
    html << generateHeader();
    html << "<body>\n";
    html << "<div class=\"container\">\n";
    html << "<h1>" << escapeHtml(config_.report_title) << "</h1>\n";
    html << generateSummary();
    
    for (const auto& suite : test_suites_) {
        html << generateTestSuiteSection(suite);
    }
    
    if (config_.include_charts) {
        html << generateCharts();
    }
    
    if (config_.include_coverage) {
        html << generateCoverageSection();
    }
    
    if (config_.include_environment) {
        html << generateEnvironmentInfo();
    }
    
    html << "</div>\n";
    html << generateFooter();
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

std::string HtmlReportGenerator::generateHeader() const {
    std::ostringstream header;
    
    header << "<head>\n";
    header << "<meta charset=\"UTF-8\">\n";
    header << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    header << "<title>" << escapeHtml(config_.report_title) << "</title>\n";
    header << "<style>\n" << generateCss() << "</style>\n";
    
    if (config_.include_charts) {
        header << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n";
    }
    
    header << "</head>\n";
    
    return header.str();
}

std::string HtmlReportGenerator::generateSummary() const {
    size_t total_tests = 0;
    size_t passed_tests = 0;
    size_t failed_tests = 0;
    std::chrono::milliseconds total_duration(0);
    
    for (const auto& suite : test_suites_) {
        for (const auto& result : suite.results) {
            total_tests++;
            if (result.passed) {
                passed_tests++;
            } else {
                failed_tests++;
            }
            total_duration += result.execution_time;
        }
    }
    
    double pass_rate = total_tests > 0 ? 
        (static_cast<double>(passed_tests) / total_tests) * 100.0 : 0.0;
    
    std::ostringstream summary;
    summary << "<div class=\"summary\">\n";
    summary << "<h2>テスト実行サマリー</h2>\n";
    summary << "<div class=\"summary-stats\">\n";
    summary << "<div class=\"stat-box\">\n";
    summary << "<div class=\"stat-value\">" << total_tests << "</div>\n";
    summary << "<div class=\"stat-label\">総テスト数</div>\n";
    summary << "</div>\n";
    summary << "<div class=\"stat-box success\">\n";
    summary << "<div class=\"stat-value\">" << passed_tests << "</div>\n";
    summary << "<div class=\"stat-label\">成功</div>\n";
    summary << "</div>\n";
    summary << "<div class=\"stat-box failure\">\n";
    summary << "<div class=\"stat-value\">" << failed_tests << "</div>\n";
    summary << "<div class=\"stat-label\">失敗</div>\n";
    summary << "</div>\n";
    summary << "<div class=\"stat-box\">\n";
    summary << "<div class=\"stat-value\">" << std::fixed << std::setprecision(1) 
            << pass_rate << "%</div>\n";
    summary << "<div class=\"stat-label\">成功率</div>\n";
    summary << "</div>\n";
    summary << "<div class=\"stat-box\">\n";
    summary << "<div class=\"stat-value\">" << formatDuration(total_duration) << "</div>\n";
    summary << "<div class=\"stat-label\">総実行時間</div>\n";
    summary << "</div>\n";
    summary << "</div>\n";
    summary << "</div>\n";
    
    return summary.str();
}

std::string HtmlReportGenerator::generateTestSuiteSection(const TestSuite& suite) const {
    std::ostringstream section;
    
    section << "<div class=\"test-suite\">\n";
    section << "<h3>" << escapeHtml(suite.name) << "</h3>\n";
    
    if (!suite.description.empty()) {
        section << "<p class=\"suite-description\">" 
                << escapeHtml(suite.description) << "</p>\n";
    }
    
    section << "<table class=\"results-table\">\n";
    section << "<thead>\n";
    section << "<tr>\n";
    section << "<th>プロパティ名</th>\n";
    section << "<th>結果</th>\n";
    section << "<th>実行ケース数</th>\n";
    section << "<th>失敗数</th>\n";
    section << "<th>実行時間</th>\n";
    section << "<th>詳細</th>\n";
    section << "</tr>\n";
    section << "</thead>\n";
    section << "<tbody>\n";
    
    for (const auto& result : suite.results) {
        section << "<tr class=\"" << (result.passed ? "passed" : "failed") << "\">\n";
        section << "<td>" << escapeHtml(result.property_name) << "</td>\n";
        section << "<td class=\"result-cell\">" 
                << (result.passed ? "✓ PASS" : "✗ FAIL") << "</td>\n";
        section << "<td>" << result.test_cases_run << "</td>\n";
        section << "<td>" << result.test_cases_failed << "</td>\n";
        section << "<td>" << formatDuration(result.execution_time) << "</td>\n";
        section << "<td>\n";
        
        if (!result.passed && result.failure_message) {
            section << "<details>\n";
            section << "<summary>エラー詳細</summary>\n";
            section << "<pre class=\"error-message\">" 
                    << escapeHtml(*result.failure_message) << "</pre>\n";
            
            if (result.counterexample) {
                section << "<h4>反例:</h4>\n";
                section << "<pre class=\"counterexample\">" 
                        << escapeHtml(*result.counterexample) << "</pre>\n";
            }
            
            section << "</details>\n";
        }
        
        section << "</td>\n";
        section << "</tr>\n";
    }
    
    section << "</tbody>\n";
    section << "</table>\n";
    section << "</div>\n";
    
    return section.str();
}

std::string HtmlReportGenerator::generateCharts() const {
    std::ostringstream charts;
    
    charts << "<div class=\"charts-section\">\n";
    charts << "<h2>統計グラフ</h2>\n";
    charts << "<div class=\"charts-grid\">\n";
    
    charts << "<div class=\"chart-container\">\n";
    charts << "<canvas id=\"successRateChart\"></canvas>\n";
    charts << "</div>\n";
    
    charts << "<div class=\"chart-container\">\n";
    charts << "<canvas id=\"executionTimeChart\"></canvas>\n";
    charts << "</div>\n";
    
    charts << "<div class=\"chart-container\">\n";
    charts << "<canvas id=\"testDistributionChart\"></canvas>\n";
    charts << "</div>\n";
    
    charts << "<div class=\"chart-container\">\n";
    charts << "<canvas id=\"coverageChart\"></canvas>\n";
    charts << "</div>\n";
    
    charts << "</div>\n";
    charts << "</div>\n";
    
    charts << "<script>\n" << generateJavaScript() << "</script>\n";
    
    return charts.str();
}

std::string HtmlReportGenerator::generateCoverageSection() const {
    std::ostringstream coverage;
    
    coverage << "<div class=\"coverage-section\">\n";
    coverage << "<h2>カバレッジ情報</h2>\n";
    coverage << "<p>カバレッジ情報の実装は今後追加予定です。</p>\n";
    coverage << "</div>\n";
    
    return coverage.str();
}

std::string HtmlReportGenerator::generateEnvironmentInfo() const {
    std::ostringstream env;
    
    env << "<div class=\"environment-section\">\n";
    env << "<h2>実行環境</h2>\n";
    env << "<table class=\"env-table\">\n";
    
    if (!test_suites_.empty() && !test_suites_[0].environment_info.empty()) {
        for (const auto& [key, value] : test_suites_[0].environment_info) {
            env << "<tr>\n";
            env << "<td>" << escapeHtml(key) << "</td>\n";
            env << "<td>" << escapeHtml(value) << "</td>\n";
            env << "</tr>\n";
        }
    }
    
    env << "</table>\n";
    env << "</div>\n";
    
    return env.str();
}

std::string HtmlReportGenerator::generateFooter() const {
    std::ostringstream footer;
    
    footer << "<footer>\n";
    footer << "<p>Generated by JWW Property-Based Testing Framework</p>\n";
    footer << "<p>Report generated at: " 
           << formatTimestamp(std::chrono::system_clock::now()) << "</p>\n";
    footer << "</footer>\n";
    
    return footer.str();
}

std::string HtmlReportGenerator::generateCss() const {
    return R"(
body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    line-height: 1.6;
    color: #333;
    background-color: #f5f5f5;
    margin: 0;
    padding: 0;
}

.container {
    max-width: 1200px;
    margin: 0 auto;
    padding: 20px;
    background-color: white;
    box-shadow: 0 0 10px rgba(0,0,0,0.1);
}

h1, h2, h3 {
    color: #2c3e50;
}

.summary {
    background-color: #f8f9fa;
    padding: 20px;
    border-radius: 8px;
    margin-bottom: 30px;
}

.summary-stats {
    display: flex;
    gap: 20px;
    flex-wrap: wrap;
    margin-top: 20px;
}

.stat-box {
    flex: 1;
    min-width: 150px;
    text-align: center;
    padding: 20px;
    background-color: white;
    border-radius: 8px;
    box-shadow: 0 2px 4px rgba(0,0,0,0.1);
}

.stat-box.success {
    border-top: 4px solid #28a745;
}

.stat-box.failure {
    border-top: 4px solid #dc3545;
}

.stat-value {
    font-size: 2em;
    font-weight: bold;
    color: #2c3e50;
}

.stat-label {
    color: #6c757d;
    margin-top: 5px;
}

.test-suite {
    margin-bottom: 40px;
}

.results-table {
    width: 100%;
    border-collapse: collapse;
    margin-top: 20px;
}

.results-table th,
.results-table td {
    padding: 12px;
    text-align: left;
    border-bottom: 1px solid #dee2e6;
}

.results-table th {
    background-color: #f8f9fa;
    font-weight: 600;
    color: #495057;
}

.results-table tr.passed {
    background-color: #d4edda;
}

.results-table tr.failed {
    background-color: #f8d7da;
}

.result-cell {
    font-weight: bold;
}

.error-message,
.counterexample {
    background-color: #f8f9fa;
    padding: 10px;
    border-radius: 4px;
    overflow-x: auto;
    font-family: 'Courier New', monospace;
    font-size: 0.9em;
}

.charts-section {
    margin-top: 40px;
}

.charts-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
    gap: 30px;
    margin-top: 20px;
}

.chart-container {
    background-color: white;
    padding: 20px;
    border-radius: 8px;
    box-shadow: 0 2px 4px rgba(0,0,0,0.1);
}

footer {
    text-align: center;
    margin-top: 40px;
    padding-top: 20px;
    border-top: 1px solid #dee2e6;
    color: #6c757d;
}

details summary {
    cursor: pointer;
    color: #007bff;
}

details summary:hover {
    text-decoration: underline;
}
)";
}

std::string HtmlReportGenerator::generateJavaScript() const {
    auto chart_data = ChartDataGenerator::generateSuccessRateChart(test_suites_);
    auto time_data = ChartDataGenerator::generateExecutionTimeChart(test_suites_);
    auto dist_data = ChartDataGenerator::generateTestCaseDistribution(test_suites_);
    
    std::ostringstream js;
    
    js << "const ctx1 = document.getElementById('successRateChart').getContext('2d');\n";
    js << "new Chart(ctx1, {\n";
    js << "    type: 'bar',\n";
    js << "    data: {\n";
    js << "        labels: " << "[";
    for (size_t i = 0; i < chart_data.labels.size(); ++i) {
        if (i > 0) js << ", ";
        js << "'" << chart_data.labels[i] << "'";
    }
    js << "],\n";
    js << "        datasets: [{\n";
    js << "            label: '成功率 (%)',\n";
    js << "            data: " << "[";
    for (size_t i = 0; i < chart_data.values.size(); ++i) {
        if (i > 0) js << ", ";
        js << chart_data.values[i];
    }
    js << "],\n";
    js << "            backgroundColor: 'rgba(40, 167, 69, 0.6)',\n";
    js << "            borderColor: 'rgba(40, 167, 69, 1)',\n";
    js << "            borderWidth: 1\n";
    js << "        }]\n";
    js << "    },\n";
    js << "    options: {\n";
    js << "        responsive: true,\n";
    js << "        scales: {\n";
    js << "            y: {\n";
    js << "                beginAtZero: true,\n";
    js << "                max: 100\n";
    js << "            }\n";
    js << "        }\n";
    js << "    }\n";
    js << "});\n";
    
    return js.str();
}

std::string HtmlReportGenerator::formatDuration(std::chrono::milliseconds duration) const {
    auto ms = duration.count();
    if (ms < 1000) {
        return std::to_string(ms) + "ms";
    } else if (ms < 60000) {
        return std::to_string(ms / 1000.0).substr(0, 4) + "s";
    } else {
        auto minutes = ms / 60000;
        auto seconds = (ms % 60000) / 1000;
        return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
    }
}

std::string HtmlReportGenerator::formatTimestamp(
    std::chrono::system_clock::time_point time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string HtmlReportGenerator::escapeHtml(const std::string& text) const {
    std::string escaped;
    escaped.reserve(text.size());
    
    for (char c : text) {
        switch (c) {
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '&': escaped += "&amp;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += c; break;
        }
    }
    
    return escaped;
}

void HtmlReportGenerator::copyAssets() {
}

void HtmlReportGenerator::writeReport(const std::string& html_content) {
    std::ofstream file(report_path_);
    if (!file) {
        throw std::runtime_error("Failed to create report file: " + report_path_.string());
    }
    file << html_content;
}

ChartDataGenerator::ChartData ChartDataGenerator::generateSuccessRateChart(
    const std::vector<TestSuite>& suites) {
    ChartData data;
    data.chart_type = "bar";
    data.title = "テストスイート別成功率";
    
    for (const auto& suite : suites) {
        data.labels.push_back(suite.name);
        
        size_t passed = 0;
        size_t total = suite.results.size();
        
        for (const auto& result : suite.results) {
            if (result.passed) passed++;
        }
        
        double rate = total > 0 ? (static_cast<double>(passed) / total) * 100.0 : 0.0;
        data.values.push_back(rate);
    }
    
    return data;
}

ChartDataGenerator::ChartData ChartDataGenerator::generateExecutionTimeChart(
    const std::vector<TestSuite>& suites) {
    ChartData data;
    data.chart_type = "line";
    data.title = "実行時間の推移";
    
    for (const auto& suite : suites) {
        for (const auto& result : suite.results) {
            data.labels.push_back(result.property_name);
            data.values.push_back(result.execution_time.count());
        }
    }
    
    return data;
}

ChartDataGenerator::ChartData ChartDataGenerator::generateTestCaseDistribution(
    const std::vector<TestSuite>& suites) {
    ChartData data;
    data.chart_type = "pie";
    data.title = "テストケース分布";
    
    for (const auto& suite : suites) {
        data.labels.push_back(suite.name);
        
        size_t total_cases = 0;
        for (const auto& result : suite.results) {
            total_cases += result.test_cases_run;
        }
        
        data.values.push_back(static_cast<double>(total_cases));
    }
    
    return data;
}

ChartDataGenerator::ChartData ChartDataGenerator::generatePropertyCoverageChart(
    const std::vector<TestSuite>& suites) {
    ChartData data;
    data.chart_type = "radar";
    data.title = "プロパティカバレッジ";
    
    std::unordered_map<std::string, size_t> property_counts;
    
    for (const auto& suite : suites) {
        for (const auto& result : suite.results) {
            property_counts[result.property_name]++;
        }
    }
    
    for (const auto& [name, count] : property_counts) {
        data.labels.push_back(name);
        data.values.push_back(static_cast<double>(count));
    }
    
    return data;
}

} // namespace reporting
} // namespace pbt
} // namespace jww