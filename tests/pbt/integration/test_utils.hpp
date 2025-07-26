#pragma once

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <chrono>
#include <fstream>
#include <sstream>

namespace jwwlib {
namespace pbt {

/**
 * @brief Utilities for test execution and reporting
 */
class TestUtils {
public:
    /**
     * @brief Save counterexample to file for debugging
     */
    template<typename T>
    static void saveCounterexample(const std::string& test_name, 
                                  const T& counterexample,
                                  const std::string& seed) {
        std::string filename = "counterexample_" + test_name + "_" + seed + ".txt";
        std::ofstream file(filename);
        
        if (file.is_open()) {
            file << "Test: " << test_name << "\n";
            file << "Seed: " << seed << "\n";
            file << "Counterexample:\n";
            file << rc::detail::toString(counterexample) << "\n";
            file.close();
            
            std::cout << "Counterexample saved to: " << filename << "\n";
        }
    }

    /**
     * @brief Measure execution time of a function
     */
    template<typename Func>
    static double measureTime(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    /**
     * @brief Run test with timeout
     */
    template<typename Func>
    static bool runWithTimeout(Func&& func, int timeout_ms) {
        std::atomic<bool> completed{false};
        std::atomic<bool> timed_out{false};
        
        std::thread worker([&]() {
            func();
            completed = true;
        });
        
        std::thread timeout_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            if (!completed) {
                timed_out = true;
            }
        });
        
        worker.join();
        timeout_thread.join();
        
        return !timed_out;
    }

    /**
     * @brief Generate test report in markdown format
     */
    static void generateMarkdownReport(const std::string& filename,
                                     const std::vector<std::string>& test_names,
                                     const std::vector<bool>& results,
                                     const std::vector<double>& times) {
        std::ofstream file(filename);
        
        if (file.is_open()) {
            file << "# Property-Based Testing Report\n\n";
            file << "Generated: " << getCurrentTimestamp() << "\n\n";
            
            file << "## Summary\n\n";
            int passed = std::count(results.begin(), results.end(), true);
            int total = results.size();
            file << "- Total tests: " << total << "\n";
            file << "- Passed: " << passed << "\n";
            file << "- Failed: " << (total - passed) << "\n";
            file << "- Success rate: " << (100.0 * passed / total) << "%\n\n";
            
            file << "## Test Results\n\n";
            file << "| Test Name | Result | Time (ms) |\n";
            file << "|-----------|--------|----------|\n";
            
            for (size_t i = 0; i < test_names.size(); i++) {
                file << "| " << test_names[i] 
                     << " | " << (results[i] ? "✅ PASS" : "❌ FAIL")
                     << " | " << times[i] << " |\n";
            }
            
            file.close();
        }
    }

    /**
     * @brief Compare two files for equality
     */
    static bool filesEqual(const std::string& file1, const std::string& file2) {
        std::ifstream f1(file1, std::ios::binary);
        std::ifstream f2(file2, std::ios::binary);
        
        if (!f1.is_open() || !f2.is_open()) {
            return false;
        }
        
        return std::equal(
            std::istreambuf_iterator<char>(f1),
            std::istreambuf_iterator<char>(),
            std::istreambuf_iterator<char>(f2)
        );
    }

private:
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

/**
 * @brief Custom test environment for setup/teardown
 */
class PBTEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "=== Property-Based Testing Environment Setup ===\n";
        
        // Create directories for test outputs
        std::filesystem::create_directories("pbt_results");
        std::filesystem::create_directories("pbt_counterexamples");
        
        // Initialize any global resources
        initializeTestResources();
    }

    void TearDown() override {
        std::cout << "=== Property-Based Testing Environment Teardown ===\n";
        
        // Generate final report
        generateFinalReport();
        
        // Clean up resources
        cleanupTestResources();
    }

private:
    void initializeTestResources() {
        // Initialize any shared resources
    }

    void cleanupTestResources() {
        // Clean up any shared resources
    }

    void generateFinalReport() {
        // Generate comprehensive test report
    }
};

} // namespace pbt
} // namespace jwwlib