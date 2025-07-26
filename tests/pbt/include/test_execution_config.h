#ifndef PBT_TEST_EXECUTION_CONFIG_H
#define PBT_TEST_EXECUTION_CONFIG_H

#include <chrono>
#include <string>
#include <optional>
#include <functional>

namespace jwwlib_pbt {

/**
 * @brief Configuration for test execution
 * 
 * This class holds all configuration parameters for running property-based tests,
 * including number of test cases, timeouts, and resource limits.
 */
class TestExecutionConfig {
public:
    /**
     * @brief Default configuration values
     */
    static constexpr size_t DEFAULT_NUM_TESTS = 100;
    static constexpr size_t DEFAULT_MAX_SIZE = 100;
    static constexpr size_t DEFAULT_SEED = 0; // 0 means random seed
    static constexpr auto DEFAULT_TIMEOUT = std::chrono::seconds(30);
    static constexpr size_t DEFAULT_MAX_MEMORY_MB = 1024;
    static constexpr size_t DEFAULT_WORKER_THREADS = 1;
    
    /**
     * @brief Test reporting verbosity levels
     */
    enum class Verbosity {
        QUIET,      // Only report failures
        NORMAL,     // Report test progress and failures
        VERBOSE,    // Report detailed information
        DEBUG       // Report everything including generated values
    };
    
    /**
     * @brief Memory checking mode
     */
    enum class MemoryCheckMode {
        NONE,           // No memory checking
        BASIC,          // Basic leak detection
        VALGRIND,       // Use Valgrind if available
        ADDRESS_SANITIZER // Use AddressSanitizer if compiled with it
    };
    
    TestExecutionConfig() = default;
    
    // Builder pattern methods
    TestExecutionConfig& withNumTests(size_t num) {
        numTests_ = num;
        return *this;
    }
    
    TestExecutionConfig& withMaxSize(size_t size) {
        maxSize_ = size;
        return *this;
    }
    
    TestExecutionConfig& withSeed(size_t seed) {
        seed_ = seed;
        return *this;
    }
    
    TestExecutionConfig& withTimeout(std::chrono::milliseconds timeout) {
        timeout_ = timeout;
        return *this;
    }
    
    TestExecutionConfig& withMaxMemoryMB(size_t mb) {
        maxMemoryMB_ = mb;
        return *this;
    }
    
    TestExecutionConfig& withVerbosity(Verbosity level) {
        verbosity_ = level;
        return *this;
    }
    
    TestExecutionConfig& withMemoryCheck(MemoryCheckMode mode) {
        memoryCheckMode_ = mode;
        return *this;
    }
    
    TestExecutionConfig& withWorkerThreads(size_t threads) {
        workerThreads_ = threads;
        return *this;
    }
    
    TestExecutionConfig& withProgressCallback(
        std::function<void(size_t current, size_t total)> callback
    ) {
        progressCallback_ = callback;
        return *this;
    }
    
    TestExecutionConfig& withOutputFile(const std::string& path) {
        outputFile_ = path;
        return *this;
    }
    
    TestExecutionConfig& withJUnitXML(const std::string& path) {
        junitXmlPath_ = path;
        return *this;
    }
    
    TestExecutionConfig& withFailFast(bool enable) {
        failFast_ = enable;
        return *this;
    }
    
    TestExecutionConfig& withShrinking(bool enable) {
        enableShrinking_ = enable;
        return *this;
    }
    
    TestExecutionConfig& withReplay(const std::string& replayFile) {
        replayFile_ = replayFile;
        return *this;
    }
    
    // Getters
    size_t getNumTests() const { return numTests_; }
    size_t getMaxSize() const { return maxSize_; }
    size_t getSeed() const { return seed_; }
    std::chrono::milliseconds getTimeout() const { return timeout_; }
    size_t getMaxMemoryMB() const { return maxMemoryMB_; }
    Verbosity getVerbosity() const { return verbosity_; }
    MemoryCheckMode getMemoryCheckMode() const { return memoryCheckMode_; }
    size_t getWorkerThreads() const { return workerThreads_; }
    bool isFailFast() const { return failFast_; }
    bool isShrinkingEnabled() const { return enableShrinking_; }
    
    const std::optional<std::string>& getOutputFile() const { return outputFile_; }
    const std::optional<std::string>& getJUnitXmlPath() const { return junitXmlPath_; }
    const std::optional<std::string>& getReplayFile() const { return replayFile_; }
    
    const std::optional<std::function<void(size_t, size_t)>>& 
    getProgressCallback() const { return progressCallback_; }
    
    /**
     * @brief Load configuration from environment variables
     */
    static TestExecutionConfig fromEnvironment() {
        TestExecutionConfig config;
        
        // Check environment variables
        if (const char* numTests = std::getenv("PBT_NUM_TESTS")) {
            config.numTests_ = std::stoul(numTests);
        }
        
        if (const char* seed = std::getenv("PBT_SEED")) {
            config.seed_ = std::stoul(seed);
        }
        
        if (const char* timeout = std::getenv("PBT_TIMEOUT_MS")) {
            config.timeout_ = std::chrono::milliseconds(std::stoul(timeout));
        }
        
        if (const char* threads = std::getenv("PBT_WORKER_THREADS")) {
            config.workerThreads_ = std::stoul(threads);
        }
        
        if (const char* verbosity = std::getenv("PBT_VERBOSITY")) {
            std::string v(verbosity);
            if (v == "quiet") config.verbosity_ = Verbosity::QUIET;
            else if (v == "normal") config.verbosity_ = Verbosity::NORMAL;
            else if (v == "verbose") config.verbosity_ = Verbosity::VERBOSE;
            else if (v == "debug") config.verbosity_ = Verbosity::DEBUG;
        }
        
        return config;
    }
    
    /**
     * @brief Merge with another configuration (other takes precedence)
     */
    TestExecutionConfig merge(const TestExecutionConfig& other) const {
        TestExecutionConfig result = *this;
        
        if (other.numTests_ != DEFAULT_NUM_TESTS) result.numTests_ = other.numTests_;
        if (other.maxSize_ != DEFAULT_MAX_SIZE) result.maxSize_ = other.maxSize_;
        if (other.seed_ != DEFAULT_SEED) result.seed_ = other.seed_;
        if (other.timeout_ != DEFAULT_TIMEOUT) result.timeout_ = other.timeout_;
        if (other.maxMemoryMB_ != DEFAULT_MAX_MEMORY_MB) {
            result.maxMemoryMB_ = other.maxMemoryMB_;
        }
        if (other.workerThreads_ != DEFAULT_WORKER_THREADS) {
            result.workerThreads_ = other.workerThreads_;
        }
        
        // Always take other's values for these
        result.verbosity_ = other.verbosity_;
        result.memoryCheckMode_ = other.memoryCheckMode_;
        result.failFast_ = other.failFast_;
        result.enableShrinking_ = other.enableShrinking_;
        
        // Optional values
        if (other.outputFile_) result.outputFile_ = other.outputFile_;
        if (other.junitXmlPath_) result.junitXmlPath_ = other.junitXmlPath_;
        if (other.replayFile_) result.replayFile_ = other.replayFile_;
        if (other.progressCallback_) result.progressCallback_ = other.progressCallback_;
        
        return result;
    }
    
private:
    size_t numTests_ = DEFAULT_NUM_TESTS;
    size_t maxSize_ = DEFAULT_MAX_SIZE;
    size_t seed_ = DEFAULT_SEED;
    std::chrono::milliseconds timeout_ = DEFAULT_TIMEOUT;
    size_t maxMemoryMB_ = DEFAULT_MAX_MEMORY_MB;
    size_t workerThreads_ = DEFAULT_WORKER_THREADS;
    
    Verbosity verbosity_ = Verbosity::NORMAL;
    MemoryCheckMode memoryCheckMode_ = MemoryCheckMode::NONE;
    bool failFast_ = false;
    bool enableShrinking_ = true;
    
    std::optional<std::string> outputFile_;
    std::optional<std::string> junitXmlPath_;
    std::optional<std::string> replayFile_;
    std::optional<std::function<void(size_t, size_t)>> progressCallback_;
};

} // namespace jwwlib_pbt

#endif // PBT_TEST_EXECUTION_CONFIG_H