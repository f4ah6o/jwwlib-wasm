#pragma once

#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <optional>

namespace jww {
namespace pbt {
namespace reporting {

struct DataPoint {
    double value;
    size_t frequency;
    std::string category;
};

struct DistributionStats {
    double mean;
    double median;
    double std_deviation;
    double min_value;
    double max_value;
    double percentile_25;
    double percentile_75;
    double percentile_95;
    double percentile_99;
};

struct Histogram {
    std::vector<double> bin_edges;
    std::vector<size_t> frequencies;
    double bin_width;
};

class DataVisualizer {
public:
    static DistributionStats calculateStats(const std::vector<double>& data);
    
    static Histogram createHistogram(const std::vector<double>& data, 
                                   size_t num_bins = 20);
    
    static std::map<std::string, size_t> createFrequencyMap(
        const std::vector<std::string>& categorical_data);
    
    static std::string generateDistributionSvg(const Histogram& hist,
                                              int width = 600,
                                              int height = 400,
                                              const std::string& title = "");
    
    static std::string generateHeatmapSvg(
        const std::vector<std::vector<double>>& matrix,
        int width = 600,
        int height = 400,
        const std::string& title = "");
    
    static std::string generateScatterPlotSvg(
        const std::vector<std::pair<double, double>>& points,
        int width = 600,
        int height = 400,
        const std::string& x_label = "X",
        const std::string& y_label = "Y");

    static std::string generateBoxPlotSvg(
        const std::map<std::string, std::vector<double>>& data,
        int width = 600,
        int height = 400,
        const std::string& title = "");
    
    template<typename T>
    static std::vector<DataPoint> analyzeDataDistribution(
        const std::vector<T>& generated_values,
        std::function<double(const T&)> value_extractor);

private:
    static double calculatePercentile(const std::vector<double>& sorted_data, 
                                    double percentile);
    
    static std::string colorScale(double value, double min_val, double max_val);
    
    static std::string escapeXml(const std::string& text);
};

class EdgeCaseAnalyzer {
public:
    struct EdgeCase {
        std::string description;
        size_t occurrence_count;
        double coverage_percentage;
        std::vector<std::string> examples;
    };
    
    struct EdgeCaseCoverage {
        std::vector<EdgeCase> detected_cases;
        std::vector<std::string> missing_cases;
        double overall_coverage;
    };
    
    template<typename T>
    static EdgeCaseCoverage analyzeEdgeCases(
        const std::vector<T>& generated_values,
        const std::vector<std::function<bool(const T&)>>& edge_case_detectors,
        const std::vector<std::string>& edge_case_names);
    
    static std::string generateEdgeCaseReport(const EdgeCaseCoverage& coverage);
    
    static std::string generateCoverageSunburstSvg(
        const EdgeCaseCoverage& coverage,
        int size = 600);
};

template<typename T>
std::vector<DataPoint> DataVisualizer::analyzeDataDistribution(
    const std::vector<T>& generated_values,
    std::function<double(const T&)> value_extractor) {
    
    std::map<double, size_t> value_counts;
    
    for (const auto& val : generated_values) {
        double extracted = value_extractor(val);
        value_counts[extracted]++;
    }
    
    std::vector<DataPoint> distribution;
    for (const auto& [value, count] : value_counts) {
        distribution.push_back({value, count, ""});
    }
    
    return distribution;
}

template<typename T>
EdgeCaseAnalyzer::EdgeCaseCoverage EdgeCaseAnalyzer::analyzeEdgeCases(
    const std::vector<T>& generated_values,
    const std::vector<std::function<bool(const T&)>>& edge_case_detectors,
    const std::vector<std::string>& edge_case_names) {
    
    EdgeCaseCoverage coverage;
    size_t total_values = generated_values.size();
    
    for (size_t i = 0; i < edge_case_detectors.size(); ++i) {
        EdgeCase edge_case;
        edge_case.description = edge_case_names[i];
        edge_case.occurrence_count = 0;
        
        for (const auto& value : generated_values) {
            if (edge_case_detectors[i](value)) {
                edge_case.occurrence_count++;
                if (edge_case.examples.size() < 5) {
                    edge_case.examples.push_back(std::to_string(value));
                }
            }
        }
        
        edge_case.coverage_percentage = total_values > 0 ?
            (static_cast<double>(edge_case.occurrence_count) / total_values) * 100.0 : 0.0;
        
        if (edge_case.occurrence_count > 0) {
            coverage.detected_cases.push_back(edge_case);
        } else {
            coverage.missing_cases.push_back(edge_case.description);
        }
    }
    
    size_t detected_count = coverage.detected_cases.size();
    size_t total_cases = edge_case_detectors.size();
    coverage.overall_coverage = total_cases > 0 ?
        (static_cast<double>(detected_count) / total_cases) * 100.0 : 0.0;
    
    return coverage;
}

} // namespace reporting
} // namespace pbt
} // namespace jww