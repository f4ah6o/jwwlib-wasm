#include "data_visualizer.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace jww {
namespace pbt {
namespace reporting {

DistributionStats DataVisualizer::calculateStats(const std::vector<double>& data) {
    DistributionStats stats;
    
    if (data.empty()) {
        return stats;
    }
    
    std::vector<double> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());
    
    stats.min_value = sorted_data.front();
    stats.max_value = sorted_data.back();
    
    double sum = std::accumulate(sorted_data.begin(), sorted_data.end(), 0.0);
    stats.mean = sum / sorted_data.size();
    
    stats.median = calculatePercentile(sorted_data, 50.0);
    stats.percentile_25 = calculatePercentile(sorted_data, 25.0);
    stats.percentile_75 = calculatePercentile(sorted_data, 75.0);
    stats.percentile_95 = calculatePercentile(sorted_data, 95.0);
    stats.percentile_99 = calculatePercentile(sorted_data, 99.0);
    
    double variance = 0.0;
    for (const auto& value : data) {
        variance += std::pow(value - stats.mean, 2);
    }
    stats.std_deviation = std::sqrt(variance / data.size());
    
    return stats;
}

Histogram DataVisualizer::createHistogram(const std::vector<double>& data, 
                                        size_t num_bins) {
    Histogram hist;
    
    if (data.empty() || num_bins == 0) {
        return hist;
    }
    
    auto [min_it, max_it] = std::minmax_element(data.begin(), data.end());
    double min_val = *min_it;
    double max_val = *max_it;
    
    hist.bin_width = (max_val - min_val) / num_bins;
    
    for (size_t i = 0; i <= num_bins; ++i) {
        hist.bin_edges.push_back(min_val + i * hist.bin_width);
    }
    
    hist.frequencies.resize(num_bins, 0);
    
    for (const auto& value : data) {
        size_t bin_index = static_cast<size_t>((value - min_val) / hist.bin_width);
        if (bin_index >= num_bins) {
            bin_index = num_bins - 1;
        }
        hist.frequencies[bin_index]++;
    }
    
    return hist;
}

std::map<std::string, size_t> DataVisualizer::createFrequencyMap(
    const std::vector<std::string>& categorical_data) {
    std::map<std::string, size_t> freq_map;
    
    for (const auto& category : categorical_data) {
        freq_map[category]++;
    }
    
    return freq_map;
}

std::string DataVisualizer::generateDistributionSvg(const Histogram& hist,
                                                   int width,
                                                   int height,
                                                   const std::string& title) {
    std::ostringstream svg;
    
    const int margin = 50;
    const int plot_width = width - 2 * margin;
    const int plot_height = height - 2 * margin;
    
    svg << "<svg width=\"" << width << "\" height=\"" << height << "\">\n";
    
    if (!title.empty()) {
        svg << "<text x=\"" << width / 2 << "\" y=\"30\" "
            << "text-anchor=\"middle\" font-size=\"16\" font-weight=\"bold\">"
            << escapeXml(title) << "</text>\n";
    }
    
    svg << "<g transform=\"translate(" << margin << "," << margin << ")\">\n";
    
    svg << "<rect x=\"0\" y=\"0\" width=\"" << plot_width << "\" height=\"" 
        << plot_height << "\" fill=\"none\" stroke=\"black\"/>\n";
    
    if (!hist.frequencies.empty()) {
        size_t max_freq = *std::max_element(hist.frequencies.begin(), hist.frequencies.end());
        double bar_width = static_cast<double>(plot_width) / hist.frequencies.size();
        
        for (size_t i = 0; i < hist.frequencies.size(); ++i) {
            double bar_height = (static_cast<double>(hist.frequencies[i]) / max_freq) * plot_height;
            double x = i * bar_width;
            double y = plot_height - bar_height;
            
            svg << "<rect x=\"" << x << "\" y=\"" << y << "\" "
                << "width=\"" << bar_width - 1 << "\" height=\"" << bar_height << "\" "
                << "fill=\"steelblue\" stroke=\"white\"/>\n";
            
            if (i % 5 == 0 && i < hist.bin_edges.size() - 1) {
                svg << "<text x=\"" << x << "\" y=\"" << plot_height + 20 << "\" "
                    << "font-size=\"10\" text-anchor=\"middle\">"
                    << std::fixed << std::setprecision(1) << hist.bin_edges[i] 
                    << "</text>\n";
            }
        }
        
        for (int i = 0; i <= 5; ++i) {
            double y = plot_height - (i * plot_height / 5.0);
            size_t value = static_cast<size_t>(i * max_freq / 5.0);
            
            svg << "<line x1=\"-5\" y1=\"" << y << "\" x2=\"0\" y2=\"" << y 
                << "\" stroke=\"black\"/>\n";
            svg << "<text x=\"-10\" y=\"" << y + 5 << "\" "
                << "font-size=\"10\" text-anchor=\"end\">"
                << value << "</text>\n";
        }
    }
    
    svg << "</g>\n";
    svg << "</svg>\n";
    
    return svg.str();
}

std::string DataVisualizer::generateHeatmapSvg(
    const std::vector<std::vector<double>>& matrix,
    int width,
    int height,
    const std::string& title) {
    
    std::ostringstream svg;
    
    if (matrix.empty() || matrix[0].empty()) {
        return svg.str();
    }
    
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();
    
    for (const auto& row : matrix) {
        for (const auto& val : row) {
            min_val = std::min(min_val, val);
            max_val = std::max(max_val, val);
        }
    }
    
    const int margin = 50;
    const int plot_width = width - 2 * margin;
    const int plot_height = height - 2 * margin;
    
    svg << "<svg width=\"" << width << "\" height=\"" << height << "\">\n";
    
    if (!title.empty()) {
        svg << "<text x=\"" << width / 2 << "\" y=\"30\" "
            << "text-anchor=\"middle\" font-size=\"16\" font-weight=\"bold\">"
            << escapeXml(title) << "</text>\n";
    }
    
    svg << "<g transform=\"translate(" << margin << "," << margin << ")\">\n";
    
    double cell_width = static_cast<double>(plot_width) / matrix[0].size();
    double cell_height = static_cast<double>(plot_height) / matrix.size();
    
    for (size_t i = 0; i < matrix.size(); ++i) {
        for (size_t j = 0; j < matrix[i].size(); ++j) {
            double x = j * cell_width;
            double y = i * cell_height;
            std::string color = colorScale(matrix[i][j], min_val, max_val);
            
            svg << "<rect x=\"" << x << "\" y=\"" << y << "\" "
                << "width=\"" << cell_width << "\" height=\"" << cell_height << "\" "
                << "fill=\"" << color << "\" stroke=\"white\" stroke-width=\"0.5\"/>\n";
        }
    }
    
    svg << "</g>\n";
    
    const int legend_width = 20;
    const int legend_height = plot_height;
    const int legend_x = width - margin + 10;
    
    svg << "<g transform=\"translate(" << legend_x << "," << margin << ")\">\n";
    
    for (int i = 0; i <= legend_height; ++i) {
        double value = min_val + (max_val - min_val) * (1.0 - static_cast<double>(i) / legend_height);
        std::string color = colorScale(value, min_val, max_val);
        
        svg << "<rect x=\"0\" y=\"" << i << "\" "
            << "width=\"" << legend_width << "\" height=\"1\" "
            << "fill=\"" << color << "\"/>\n";
    }
    
    svg << "<rect x=\"0\" y=\"0\" width=\"" << legend_width << "\" height=\"" 
        << legend_height << "\" fill=\"none\" stroke=\"black\"/>\n";
    
    for (int i = 0; i <= 5; ++i) {
        double y = i * legend_height / 5.0;
        double value = max_val - (max_val - min_val) * i / 5.0;
        
        svg << "<line x1=\"" << legend_width << "\" y1=\"" << y 
            << "\" x2=\"" << legend_width + 5 << "\" y2=\"" << y 
            << "\" stroke=\"black\"/>\n";
        svg << "<text x=\"" << legend_width + 10 << "\" y=\"" << y + 5 << "\" "
            << "font-size=\"10\">"
            << std::fixed << std::setprecision(1) << value << "</text>\n";
    }
    
    svg << "</g>\n";
    svg << "</svg>\n";
    
    return svg.str();
}

std::string DataVisualizer::generateScatterPlotSvg(
    const std::vector<std::pair<double, double>>& points,
    int width,
    int height,
    const std::string& x_label,
    const std::string& y_label) {
    
    std::ostringstream svg;
    
    if (points.empty()) {
        return svg.str();
    }
    
    double min_x = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double min_y = std::numeric_limits<double>::max();
    double max_y = std::numeric_limits<double>::lowest();
    
    for (const auto& [x, y] : points) {
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
    }
    
    const int margin = 60;
    const int plot_width = width - 2 * margin;
    const int plot_height = height - 2 * margin;
    
    svg << "<svg width=\"" << width << "\" height=\"" << height << "\">\n";
    
    svg << "<g transform=\"translate(" << margin << "," << margin << ")\">\n";
    
    svg << "<rect x=\"0\" y=\"0\" width=\"" << plot_width << "\" height=\"" 
        << plot_height << "\" fill=\"none\" stroke=\"black\"/>\n";
    
    for (const auto& [x, y] : points) {
        double plot_x = (x - min_x) / (max_x - min_x) * plot_width;
        double plot_y = plot_height - (y - min_y) / (max_y - min_y) * plot_height;
        
        svg << "<circle cx=\"" << plot_x << "\" cy=\"" << plot_y << "\" "
            << "r=\"3\" fill=\"steelblue\" opacity=\"0.6\"/>\n";
    }
    
    svg << "<text x=\"" << plot_width / 2 << "\" y=\"" << plot_height + 40 << "\" "
        << "text-anchor=\"middle\" font-size=\"12\">"
        << escapeXml(x_label) << "</text>\n";
    
    svg << "<text x=\"" << -plot_height / 2 << "\" y=\"-40\" "
        << "text-anchor=\"middle\" font-size=\"12\" "
        << "transform=\"rotate(-90)\">"
        << escapeXml(y_label) << "</text>\n";
    
    svg << "</g>\n";
    svg << "</svg>\n";
    
    return svg.str();
}

std::string DataVisualizer::generateBoxPlotSvg(
    const std::map<std::string, std::vector<double>>& data,
    int width,
    int height,
    const std::string& title) {
    
    std::ostringstream svg;
    
    if (data.empty()) {
        return svg.str();
    }
    
    const int margin = 60;
    const int plot_width = width - 2 * margin;
    const int plot_height = height - 2 * margin;
    
    svg << "<svg width=\"" << width << "\" height=\"" << height << "\">\n";
    
    if (!title.empty()) {
        svg << "<text x=\"" << width / 2 << "\" y=\"30\" "
            << "text-anchor=\"middle\" font-size=\"16\" font-weight=\"bold\">"
            << escapeXml(title) << "</text>\n";
    }
    
    svg << "<g transform=\"translate(" << margin << "," << margin << ")\">\n";
    
    double box_width = static_cast<double>(plot_width) / data.size();
    size_t index = 0;
    
    double global_min = std::numeric_limits<double>::max();
    double global_max = std::numeric_limits<double>::lowest();
    
    for (const auto& [name, values] : data) {
        if (!values.empty()) {
            auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
            global_min = std::min(global_min, *min_it);
            global_max = std::max(global_max, *max_it);
        }
    }
    
    for (const auto& [name, values] : data) {
        if (values.empty()) continue;
        
        std::vector<double> sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());
        
        double q1 = calculatePercentile(sorted_values, 25.0);
        double median = calculatePercentile(sorted_values, 50.0);
        double q3 = calculatePercentile(sorted_values, 75.0);
        double iqr = q3 - q1;
        double whisker_low = q1 - 1.5 * iqr;
        double whisker_high = q3 + 1.5 * iqr;
        
        whisker_low = std::max(whisker_low, sorted_values.front());
        whisker_high = std::min(whisker_high, sorted_values.back());
        
        double x_center = (index + 0.5) * box_width;
        double box_half_width = box_width * 0.3;
        
        auto scale_y = [&](double value) {
            return plot_height - (value - global_min) / (global_max - global_min) * plot_height;
        };
        
        svg << "<line x1=\"" << x_center << "\" y1=\"" << scale_y(whisker_low) 
            << "\" x2=\"" << x_center << "\" y2=\"" << scale_y(whisker_high) 
            << "\" stroke=\"black\" stroke-width=\"1\"/>\n";
        
        svg << "<rect x=\"" << x_center - box_half_width << "\" y=\"" << scale_y(q3) 
            << "\" width=\"" << 2 * box_half_width << "\" height=\"" 
            << scale_y(q1) - scale_y(q3) << "\" fill=\"lightblue\" stroke=\"black\"/>\n";
        
        svg << "<line x1=\"" << x_center - box_half_width << "\" y1=\"" << scale_y(median) 
            << "\" x2=\"" << x_center + box_half_width << "\" y2=\"" << scale_y(median) 
            << "\" stroke=\"black\" stroke-width=\"2\"/>\n";
        
        svg << "<text x=\"" << x_center << "\" y=\"" << plot_height + 20 << "\" "
            << "text-anchor=\"middle\" font-size=\"10\">"
            << escapeXml(name) << "</text>\n";
        
        index++;
    }
    
    svg << "</g>\n";
    svg << "</svg>\n";
    
    return svg.str();
}

double DataVisualizer::calculatePercentile(const std::vector<double>& sorted_data, 
                                          double percentile) {
    if (sorted_data.empty()) {
        return 0.0;
    }
    
    double index = (percentile / 100.0) * (sorted_data.size() - 1);
    size_t lower_index = static_cast<size_t>(std::floor(index));
    size_t upper_index = static_cast<size_t>(std::ceil(index));
    
    if (lower_index == upper_index) {
        return sorted_data[lower_index];
    }
    
    double lower_value = sorted_data[lower_index];
    double upper_value = sorted_data[upper_index];
    double fraction = index - lower_index;
    
    return lower_value + fraction * (upper_value - lower_value);
}

std::string DataVisualizer::colorScale(double value, double min_val, double max_val) {
    double normalized = (value - min_val) / (max_val - min_val);
    
    int r = static_cast<int>(255 * (1.0 - normalized));
    int b = static_cast<int>(255 * normalized);
    
    std::ostringstream color;
    color << "rgb(" << r << ", 0, " << b << ")";
    return color.str();
}

std::string DataVisualizer::escapeXml(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    
    for (char c : text) {
        switch (c) {
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '&': escaped += "&amp;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&apos;"; break;
            default: escaped += c; break;
        }
    }
    
    return escaped;
}

std::string EdgeCaseAnalyzer::generateEdgeCaseReport(const EdgeCaseCoverage& coverage) {
    std::ostringstream report;
    
    report << "<div class=\"edge-case-report\">\n";
    report << "<h3>エッジケースカバレッジ分析</h3>\n";
    report << "<p>全体カバレッジ: " << std::fixed << std::setprecision(1) 
           << coverage.overall_coverage << "%</p>\n";
    
    report << "<h4>検出されたエッジケース:</h4>\n";
    report << "<table class=\"edge-case-table\">\n";
    report << "<thead>\n";
    report << "<tr>\n";
    report << "<th>エッジケース</th>\n";
    report << "<th>発生回数</th>\n";
    report << "<th>カバレッジ</th>\n";
    report << "<th>例</th>\n";
    report << "</tr>\n";
    report << "</thead>\n";
    report << "<tbody>\n";
    
    for (const auto& edge_case : coverage.detected_cases) {
        report << "<tr>\n";
        report << "<td>" << edge_case.description << "</td>\n";
        report << "<td>" << edge_case.occurrence_count << "</td>\n";
        report << "<td>" << std::fixed << std::setprecision(2) 
               << edge_case.coverage_percentage << "%</td>\n";
        report << "<td>";
        
        for (size_t i = 0; i < edge_case.examples.size(); ++i) {
            if (i > 0) report << ", ";
            report << edge_case.examples[i];
        }
        
        report << "</td>\n";
        report << "</tr>\n";
    }
    
    report << "</tbody>\n";
    report << "</table>\n";
    
    if (!coverage.missing_cases.empty()) {
        report << "<h4>未検出のエッジケース:</h4>\n";
        report << "<ul>\n";
        for (const auto& missing : coverage.missing_cases) {
            report << "<li>" << missing << "</li>\n";
        }
        report << "</ul>\n";
    }
    
    report << "</div>\n";
    
    return report.str();
}

std::string EdgeCaseAnalyzer::generateCoverageSunburstSvg(
    const EdgeCaseCoverage& coverage,
    int size) {
    
    std::ostringstream svg;
    
    svg << "<svg width=\"" << size << "\" height=\"" << size << "\">\n";
    
    int center_x = size / 2;
    int center_y = size / 2;
    int radius = size / 2 - 20;
    
    double total_angle = 0.0;
    
    for (const auto& edge_case : coverage.detected_cases) {
        double angle = (edge_case.coverage_percentage / 100.0) * 2 * M_PI;
        
        double x1 = center_x + radius * std::cos(total_angle);
        double y1 = center_y + radius * std::sin(total_angle);
        double x2 = center_x + radius * std::cos(total_angle + angle);
        double y2 = center_y + radius * std::sin(total_angle + angle);
        
        svg << "<path d=\"M " << center_x << " " << center_y 
            << " L " << x1 << " " << y1 
            << " A " << radius << " " << radius << " 0 " 
            << (angle > M_PI ? "1" : "0") << " 1 " 
            << x2 << " " << y2 << " Z\" "
            << "fill=\"hsl(" << (total_angle * 180 / M_PI) << ", 70%, 50%)\" "
            << "stroke=\"white\" stroke-width=\"2\"/>\n";
        
        double text_angle = total_angle + angle / 2;
        double text_radius = radius * 0.7;
        double text_x = center_x + text_radius * std::cos(text_angle);
        double text_y = center_y + text_radius * std::sin(text_angle);
        
        svg << "<text x=\"" << text_x << "\" y=\"" << text_y << "\" "
            << "text-anchor=\"middle\" font-size=\"10\" fill=\"white\">"
            << edge_case.description << "</text>\n";
        
        total_angle += angle;
    }
    
    svg << "<circle cx=\"" << center_x << "\" cy=\"" << center_y << "\" "
        << "r=\"" << radius / 3 << "\" fill=\"white\"/>\n";
    
    svg << "<text x=\"" << center_x << "\" y=\"" << center_y << "\" "
        << "text-anchor=\"middle\" font-size=\"14\" font-weight=\"bold\">"
        << std::fixed << std::setprecision(1) << coverage.overall_coverage << "%</text>\n";
    
    svg << "</svg>\n";
    
    return svg.str();
}

} // namespace reporting
} // namespace pbt
} // namespace jww