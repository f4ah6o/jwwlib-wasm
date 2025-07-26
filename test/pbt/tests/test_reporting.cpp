#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <filesystem>
#include "reporting/html_report_generator.hpp"
#include "reporting/data_visualizer.hpp"
#include "reporting/test_metrics.hpp"

using namespace jww::pbt::reporting;

class ReportingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // テスト用の出力ディレクトリを作成
        test_output_dir_ = std::filesystem::temp_directory_path() / "pbt_test_reports";
        std::filesystem::create_directories(test_output_dir_);
    }
    
    void TearDown() override {
        // テスト後にディレクトリをクリーンアップ
        if (std::filesystem::exists(test_output_dir_)) {
            std::filesystem::remove_all(test_output_dir_);
        }
    }
    
    std::filesystem::path test_output_dir_;
};

TEST_F(ReportingTest, HtmlReportGeneratorBasic) {
    ReportConfig config;
    config.output_directory = test_output_dir_;
    config.report_title = "Test Report";
    
    HtmlReportGenerator generator(config);
    
    TestSuite suite;
    suite.name = "Test Suite 1";
    suite.description = "Basic test suite";
    suite.start_time = std::chrono::system_clock::now();
    
    TestResult result1;
    result1.property_name = "prop_always_positive";
    result1.passed = true;
    result1.test_cases_run = 100;
    result1.test_cases_failed = 0;
    result1.execution_time = std::chrono::milliseconds(150);
    suite.results.push_back(result1);
    
    TestResult result2;
    result2.property_name = "prop_sometimes_fails";
    result2.passed = false;
    result2.test_cases_run = 100;
    result2.test_cases_failed = 5;
    result2.execution_time = std::chrono::milliseconds(200);
    result2.failure_message = "Property failed for input: -42";
    result2.counterexample = "{ value: -42, expected: positive }";
    suite.results.push_back(result2);
    
    suite.end_time = std::chrono::system_clock::now();
    generator.addTestSuite(suite);
    
    generator.generateReport();
    
    auto report_path = generator.getReportPath();
    EXPECT_TRUE(std::filesystem::exists(report_path));
    
    // レポートファイルのサイズを確認
    EXPECT_GT(std::filesystem::file_size(report_path), 1000);
}

TEST_F(ReportingTest, DataVisualizerStats) {
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    
    auto stats = DataVisualizer::calculateStats(data);
    
    EXPECT_DOUBLE_EQ(stats.mean, 5.5);
    EXPECT_DOUBLE_EQ(stats.median, 5.5);
    EXPECT_DOUBLE_EQ(stats.min_value, 1.0);
    EXPECT_DOUBLE_EQ(stats.max_value, 10.0);
    EXPECT_NEAR(stats.std_deviation, 2.872, 0.001);
}

TEST_F(ReportingTest, DataVisualizerHistogram) {
    std::vector<double> data;
    for (int i = 0; i < 1000; ++i) {
        data.push_back(i % 100);
    }
    
    auto hist = DataVisualizer::createHistogram(data, 10);
    
    EXPECT_EQ(hist.bin_edges.size(), 11);
    EXPECT_EQ(hist.frequencies.size(), 10);
    EXPECT_DOUBLE_EQ(hist.bin_width, 9.9);
    
    // 各ビンには約100個のデータポイントが入るはず
    for (const auto& freq : hist.frequencies) {
        EXPECT_NEAR(freq, 100, 10);
    }
}

TEST_F(ReportingTest, EdgeCaseAnalyzer) {
    std::vector<int> test_values;
    for (int i = -50; i <= 50; ++i) {
        test_values.push_back(i);
    }
    
    std::vector<std::function<bool(const int&)>> detectors = {
        [](const int& val) { return val == 0; },
        [](const int& val) { return val < 0; },
        [](const int& val) { return val > 100; },
        [](const int& val) { return val == INT_MIN || val == INT_MAX; }
    };
    
    std::vector<std::string> names = {
        "Zero value",
        "Negative value",
        "Large positive value",
        "Integer boundary"
    };
    
    auto coverage = EdgeCaseAnalyzer::analyzeEdgeCases(test_values, detectors, names);
    
    EXPECT_EQ(coverage.detected_cases.size(), 2); // Zero and Negative
    EXPECT_EQ(coverage.missing_cases.size(), 2); // Large positive and Integer boundary
    EXPECT_DOUBLE_EQ(coverage.overall_coverage, 50.0);
}

TEST_F(ReportingTest, MetricsCollector) {
    auto& collector = MetricsCollector::getInstance();
    collector.reset();
    
    collector.startProperty("test_property");
    
    // テストケースを記録
    for (int i = 0; i < 10; ++i) {
        collector.recordTestCase("test_property", 
                               100, 
                               std::chrono::milliseconds(10), 
                               std::chrono::milliseconds(50), 
                               i < 8); // 80%成功
    }
    
    // 縮小を記録
    collector.recordShrinkAttempt("test_property", true, 50);
    collector.recordShrinkAttempt("test_property", false, 45);
    collector.recordShrinkAttempt("test_property", true, 25);
    
    // メモリ使用量を記録
    collector.recordMemoryUsage(1024 * 1024); // 1MB
    
    // エッジケースを記録
    collector.recordEdgeCaseHit("zero_value");
    collector.recordEdgeCaseHit("negative_value");
    
    collector.endProperty("test_property", true);
    
    auto metrics = collector.getPropertyMetrics("test_property");
    EXPECT_EQ(metrics.test_cases_generated, 10);
    EXPECT_EQ(metrics.test_cases_passed, 8);
    EXPECT_EQ(metrics.test_cases_failed, 2);
    EXPECT_DOUBLE_EQ(metrics.getSuccessRate(), 80.0);
    EXPECT_DOUBLE_EQ(metrics.getShrinkingEfficiency(), 66.67);
    
    auto memory = collector.getMemoryMetrics();
    EXPECT_EQ(memory.peak_memory_usage, 1024 * 1024);
    
    auto coverage = collector.getCoverageMetrics();
    EXPECT_EQ(coverage.total_edge_cases_found, 2);
}

TEST_F(ReportingTest, MetricsReporter) {
    auto& collector = MetricsCollector::getInstance();
    collector.reset();
    
    collector.startProperty("test_property1");
    collector.recordTestCase("test_property1", 100, 
                           std::chrono::milliseconds(10), 
                           std::chrono::milliseconds(50), true);
    collector.endProperty("test_property1", true);
    
    collector.startProperty("test_property2");
    collector.recordTestCase("test_property2", 200, 
                           std::chrono::milliseconds(20), 
                           std::chrono::milliseconds(100), false);
    collector.endProperty("test_property2", false);
    
    // HTML レポート生成
    MetricsReporter::ReportOptions options;
    options.format = "html";
    auto html_report = MetricsReporter::generateReport(collector, options);
    EXPECT_TRUE(html_report.find("<html>") != std::string::npos);
    EXPECT_TRUE(html_report.find("test_property1") != std::string::npos);
    EXPECT_TRUE(html_report.find("test_property2") != std::string::npos);
    
    // JSON レポート生成
    auto json_report = MetricsReporter::generateJsonReport(collector);
    EXPECT_TRUE(json_report.find("\"properties\"") != std::string::npos);
    EXPECT_TRUE(json_report.find("\"test_property1\"") != std::string::npos);
    
    // XML レポート生成
    auto xml_report = MetricsReporter::generateXmlReport(collector);
    EXPECT_TRUE(xml_report.find("<?xml") != std::string::npos);
    EXPECT_TRUE(xml_report.find("<pbt-metrics>") != std::string::npos);
}

TEST_F(ReportingTest, SVGGeneration) {
    // ヒストグラムSVG
    std::vector<double> data = {1, 2, 2, 3, 3, 3, 4, 4, 5};
    auto hist = DataVisualizer::createHistogram(data, 5);
    auto svg = DataVisualizer::generateDistributionSvg(hist, 600, 400, "Test Distribution");
    
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_TRUE(svg.find("Test Distribution") != std::string::npos);
    
    // 散布図SVG
    std::vector<std::pair<double, double>> points = {
        {1.0, 2.0}, {2.0, 3.0}, {3.0, 4.0}, {4.0, 3.0}, {5.0, 4.0}
    };
    auto scatter_svg = DataVisualizer::generateScatterPlotSvg(points, 600, 400, "X", "Y");
    
    EXPECT_TRUE(scatter_svg.find("<svg") != std::string::npos);
    EXPECT_TRUE(scatter_svg.find("<circle") != std::string::npos);
}

TEST_F(ReportingTest, IntegrationTest) {
    // 統合テスト: フルレポート生成
    auto& collector = MetricsCollector::getInstance();
    collector.reset();
    
    // 複数のプロパティをシミュレート
    std::vector<std::string> properties = {"prop_1", "prop_2", "prop_3"};
    
    for (const auto& prop : properties) {
        collector.startProperty(prop);
        
        for (int i = 0; i < 100; ++i) {
            bool passed = (rand() % 100) < 90; // 90%成功率
            collector.recordTestCase(prop, 
                                   rand() % 1000,
                                   std::chrono::milliseconds(rand() % 100),
                                   std::chrono::milliseconds(rand() % 500),
                                   passed);
            
            if (!passed && (rand() % 2)) {
                collector.recordShrinkAttempt(prop, true, rand() % 100);
            }
        }
        
        collector.recordMemoryUsage(rand() % (10 * 1024 * 1024));
        collector.endProperty(prop, true);
    }
    
    // HTMLレポート生成
    ReportConfig config;
    config.output_directory = test_output_dir_;
    config.report_title = "Integration Test Report";
    config.include_charts = true;
    config.include_coverage = true;
    
    HtmlReportGenerator generator(config);
    
    TestSuite suite;
    suite.name = "Integration Test Suite";
    suite.start_time = std::chrono::system_clock::now();
    
    for (const auto& prop : properties) {
        TestResult result;
        result.property_name = prop;
        auto metrics = collector.getPropertyMetrics(prop);
        result.passed = metrics.test_cases_failed == 0;
        result.test_cases_run = metrics.test_cases_generated;
        result.test_cases_failed = metrics.test_cases_failed;
        result.execution_time = metrics.total_execution_time;
        suite.results.push_back(result);
    }
    
    suite.end_time = std::chrono::system_clock::now();
    generator.addTestSuite(suite);
    generator.generateReport();
    
    auto report_path = generator.getReportPath();
    EXPECT_TRUE(std::filesystem::exists(report_path));
    
    // メトリクスレポートも生成
    auto metrics_report = MetricsReporter::generateReport(collector);
    EXPECT_FALSE(metrics_report.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}