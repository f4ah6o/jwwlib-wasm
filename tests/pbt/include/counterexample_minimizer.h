#ifndef PBT_COUNTEREXAMPLE_MINIMIZER_H
#define PBT_COUNTEREXAMPLE_MINIMIZER_H

#include <functional>
#include <vector>
#include <optional>
#include <chrono>

namespace jwwlib_pbt {

/**
 * @brief Class for minimizing counterexamples to find the simplest failing case
 * 
 * This class implements shrinking strategies to find minimal counterexamples
 * that still trigger property violations.
 */
template<typename T>
class CounterexampleMinimizer {
public:
    using PropertyFunction = std::function<bool(const T&)>;
    using ShrinkFunction = std::function<std::vector<T>(const T&)>;
    
    /**
     * @brief Configuration for minimization process
     */
    struct Config {
        size_t maxShrinkAttempts = 1000;
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000);
        bool verbose = false;
    };
    
    /**
     * @brief Result of minimization
     */
    struct Result {
        T minimalCounterexample;
        size_t shrinkSteps;
        std::chrono::milliseconds duration;
        bool timedOut;
    };
    
    CounterexampleMinimizer(PropertyFunction property, ShrinkFunction shrinker)
        : property_(property), shrinker_(shrinker) {}
    
    /**
     * @brief Minimize a counterexample
     * 
     * @param initial The initial failing counterexample
     * @param config Configuration for the minimization process
     * @return Result containing the minimal counterexample
     */
    Result minimize(const T& initial, const Config& config = Config{}) {
        auto startTime = std::chrono::steady_clock::now();
        
        T current = initial;
        size_t steps = 0;
        bool improved = true;
        
        while (improved && steps < config.maxShrinkAttempts) {
            improved = false;
            
            // Check timeout
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > config.timeout) {
                return Result{
                    current,
                    steps,
                    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed),
                    true
                };
            }
            
            // Generate shrink candidates
            std::vector<T> candidates = shrinker_(current);
            
            // Try each candidate
            for (const T& candidate : candidates) {
                try {
                    // If property returns false, we have a failing case
                    if (!property_(candidate)) {
                        current = candidate;
                        improved = true;
                        steps++;
                        
                        if (config.verbose) {
                            std::cout << "Found smaller counterexample at step " 
                                     << steps << std::endl;
                        }
                        break;
                    }
                } catch (...) {
                    // If property throws, it's also a failing case
                    current = candidate;
                    improved = true;
                    steps++;
                    
                    if (config.verbose) {
                        std::cout << "Found smaller counterexample (via exception) at step " 
                                 << steps << std::endl;
                    }
                    break;
                }
            }
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        );
        
        return Result{current, steps, duration, false};
    }
    
    /**
     * @brief Add custom shrink strategy
     */
    void addShrinkStrategy(ShrinkFunction strategy) {
        additionalStrategies_.push_back(strategy);
    }
    
private:
    PropertyFunction property_;
    ShrinkFunction shrinker_;
    std::vector<ShrinkFunction> additionalStrategies_;
};

/**
 * @brief Predefined shrink strategies for common types
 */
namespace Shrinkers {
    
    /**
     * @brief Shrink integers towards zero
     */
    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, std::vector<T>>::type
    shrinkIntegral(const T& value) {
        std::vector<T> candidates;
        
        if (value == 0) return candidates;
        
        // Try zero
        candidates.push_back(0);
        
        // Try halving
        if (value != 1 && value != -1) {
            candidates.push_back(value / 2);
        }
        
        // Try subtracting/adding 1
        if (value > 0) {
            candidates.push_back(value - 1);
        } else {
            candidates.push_back(value + 1);
        }
        
        return candidates;
    }
    
    /**
     * @brief Shrink floating point numbers
     */
    template<typename T>
    typename std::enable_if<std::is_floating_point<T>::value, std::vector<T>>::type
    shrinkFloating(const T& value) {
        std::vector<T> candidates;
        
        if (value == 0.0) return candidates;
        
        // Try zero
        candidates.push_back(0.0);
        
        // Try integers if close
        T rounded = std::round(value);
        if (std::abs(value - rounded) < 0.1) {
            candidates.push_back(rounded);
        }
        
        // Try halving
        candidates.push_back(value / 2.0);
        
        // Try reducing precision
        candidates.push_back(std::round(value * 100) / 100);
        
        return candidates;
    }
    
    /**
     * @brief Shrink strings by removing characters
     */
    inline std::vector<std::string> shrinkString(const std::string& value) {
        std::vector<std::string> candidates;
        
        if (value.empty()) return candidates;
        
        // Try empty string
        candidates.push_back("");
        
        // Try removing first/last character
        if (value.length() > 1) {
            candidates.push_back(value.substr(1));
            candidates.push_back(value.substr(0, value.length() - 1));
        }
        
        // Try first half
        if (value.length() > 2) {
            candidates.push_back(value.substr(0, value.length() / 2));
        }
        
        return candidates;
    }
    
    /**
     * @brief Shrink vectors by removing elements
     */
    template<typename T>
    std::vector<std::vector<T>> shrinkVector(const std::vector<T>& value) {
        std::vector<std::vector<T>> candidates;
        
        if (value.empty()) return candidates;
        
        // Try empty vector
        candidates.push_back({});
        
        // Try removing first/last element
        if (value.size() > 1) {
            std::vector<T> withoutFirst(value.begin() + 1, value.end());
            std::vector<T> withoutLast(value.begin(), value.end() - 1);
            candidates.push_back(withoutFirst);
            candidates.push_back(withoutLast);
        }
        
        // Try first half
        if (value.size() > 2) {
            std::vector<T> firstHalf(value.begin(), value.begin() + value.size() / 2);
            candidates.push_back(firstHalf);
        }
        
        return candidates;
    }
    
} // namespace Shrinkers

} // namespace jwwlib_pbt

#endif // PBT_COUNTEREXAMPLE_MINIMIZER_H