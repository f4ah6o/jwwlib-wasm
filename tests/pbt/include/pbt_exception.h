#ifndef PBT_EXCEPTION_H
#define PBT_EXCEPTION_H

#include <exception>
#include <string>
#include <sstream>

namespace jwwlib_pbt {

/**
 * @brief Base exception class for PBT framework
 */
class PBTException : public std::exception {
public:
    explicit PBTException(const std::string& message) : message_(message) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
protected:
    std::string message_;
};

/**
 * @brief Exception thrown when a property violation is detected
 */
class PropertyViolationException : public PBTException {
public:
    PropertyViolationException(
        const std::string& propertyName,
        const std::string& violation,
        const std::string& counterexample = ""
    ) : PBTException(buildMessage(propertyName, violation, counterexample)),
        propertyName_(propertyName),
        violation_(violation),
        counterexample_(counterexample) {}
    
    const std::string& getPropertyName() const { return propertyName_; }
    const std::string& getViolation() const { return violation_; }
    const std::string& getCounterexample() const { return counterexample_; }
    
private:
    static std::string buildMessage(
        const std::string& propertyName,
        const std::string& violation,
        const std::string& counterexample
    ) {
        std::stringstream ss;
        ss << "Property violation in '" << propertyName << "': " << violation;
        if (!counterexample.empty()) {
            ss << "\nCounterexample: " << counterexample;
        }
        return ss.str();
    }
    
    std::string propertyName_;
    std::string violation_;
    std::string counterexample_;
};

/**
 * @brief Exception thrown when generator configuration is invalid
 */
class GeneratorException : public PBTException {
public:
    GeneratorException(const std::string& generatorType, const std::string& issue)
        : PBTException("Generator '" + generatorType + "' error: " + issue),
          generatorType_(generatorType),
          issue_(issue) {}
    
    const std::string& getGeneratorType() const { return generatorType_; }
    const std::string& getIssue() const { return issue_; }
    
private:
    std::string generatorType_;
    std::string issue_;
};

/**
 * @brief Exception thrown when test execution fails
 */
class TestExecutionException : public PBTException {
public:
    enum class Phase {
        SETUP,
        GENERATION,
        EXECUTION,
        VERIFICATION,
        TEARDOWN
    };
    
    TestExecutionException(Phase phase, const std::string& details)
        : PBTException(buildMessage(phase, details)),
          phase_(phase),
          details_(details) {}
    
    Phase getPhase() const { return phase_; }
    const std::string& getDetails() const { return details_; }
    
private:
    static std::string buildMessage(Phase phase, const std::string& details) {
        std::string phaseStr;
        switch (phase) {
            case Phase::SETUP: phaseStr = "Setup"; break;
            case Phase::GENERATION: phaseStr = "Generation"; break;
            case Phase::EXECUTION: phaseStr = "Execution"; break;
            case Phase::VERIFICATION: phaseStr = "Verification"; break;
            case Phase::TEARDOWN: phaseStr = "Teardown"; break;
        }
        return "Test execution failed during " + phaseStr + " phase: " + details;
    }
    
    Phase phase_;
    std::string details_;
};

/**
 * @brief Exception thrown when memory safety checks fail
 */
class MemorySafetyException : public PBTException {
public:
    enum class Type {
        LEAK,
        BUFFER_OVERFLOW,
        USE_AFTER_FREE,
        DOUBLE_FREE,
        UNINITIALIZED_READ
    };
    
    MemorySafetyException(Type type, const std::string& location, size_t size = 0)
        : PBTException(buildMessage(type, location, size)),
          type_(type),
          location_(location),
          size_(size) {}
    
    Type getType() const { return type_; }
    const std::string& getLocation() const { return location_; }
    size_t getSize() const { return size_; }
    
private:
    static std::string buildMessage(Type type, const std::string& location, size_t size) {
        std::stringstream ss;
        ss << "Memory safety violation: ";
        
        switch (type) {
            case Type::LEAK:
                ss << "Memory leak detected";
                if (size > 0) ss << " (" << size << " bytes)";
                break;
            case Type::BUFFER_OVERFLOW:
                ss << "Buffer overflow detected";
                break;
            case Type::USE_AFTER_FREE:
                ss << "Use after free detected";
                break;
            case Type::DOUBLE_FREE:
                ss << "Double free detected";
                break;
            case Type::UNINITIALIZED_READ:
                ss << "Uninitialized memory read";
                break;
        }
        
        ss << " at " << location;
        return ss.str();
    }
    
    Type type_;
    std::string location_;
    size_t size_;
};

} // namespace jwwlib_pbt

#endif // PBT_EXCEPTION_H