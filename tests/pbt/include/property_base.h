#ifndef PBT_PROPERTY_BASE_H
#define PBT_PROPERTY_BASE_H

#include <rapidcheck.h>
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace jwwlib_pbt {

/**
 * @brief Base template class for all property definitions
 * 
 * This class provides the foundation for defining properties that can be tested
 * using RapidCheck. It includes support for preconditions, labels, and 
 * classification of test cases.
 * 
 * @tparam T The type of value being tested
 */
template<typename T>
class PropertyBase {
public:
    using GeneratorType = rc::Gen<T>;
    using PropertyFunction = std::function<void(const T&)>;
    using PreconditionFunction = std::function<bool(const T&)>;
    
    /**
     * @brief Construct a new Property Base object
     * 
     * @param name Name of the property for reporting
     * @param description Detailed description of what the property tests
     */
    PropertyBase(const std::string& name, const std::string& description)
        : name_(name), description_(description) {}
    
    virtual ~PropertyBase() = default;
    
    /**
     * @brief Set the generator for this property
     * 
     * @param gen RapidCheck generator for type T
     * @return PropertyBase& Reference for method chaining
     */
    PropertyBase& withGenerator(GeneratorType gen) {
        generator_ = gen;
        return *this;
    }
    
    /**
     * @brief Add a precondition that must be satisfied
     * 
     * @param precondition Function that returns true if the input is valid
     * @return PropertyBase& Reference for method chaining
     */
    PropertyBase& withPrecondition(PreconditionFunction precondition) {
        preconditions_.push_back(precondition);
        return *this;
    }
    
    /**
     * @brief Add a classification label for test case categorization
     * 
     * @param classifier Function that returns a label for the input
     * @return PropertyBase& Reference for method chaining
     */
    PropertyBase& withClassification(std::function<std::string(const T&)> classifier) {
        classifiers_.push_back(classifier);
        return *this;
    }
    
    /**
     * @brief Set the property function to test
     * 
     * @param property Function that throws on property violation
     * @return PropertyBase& Reference for method chaining
     */
    PropertyBase& withProperty(PropertyFunction property) {
        property_ = property;
        return *this;
    }
    
    /**
     * @brief Check if all preconditions are satisfied
     * 
     * @param value The value to check
     * @return true if all preconditions pass
     */
    bool checkPreconditions(const T& value) const {
        for (const auto& precondition : preconditions_) {
            if (!precondition(value)) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Get classification labels for a value
     * 
     * @param value The value to classify
     * @return std::vector<std::string> List of classification labels
     */
    std::vector<std::string> getClassifications(const T& value) const {
        std::vector<std::string> labels;
        for (const auto& classifier : classifiers_) {
            labels.push_back(classifier(value));
        }
        return labels;
    }
    
    /**
     * @brief Execute the property test
     * 
     * This method runs the property test using RapidCheck
     */
    virtual void check() const {
        RC_ASSERT(rc::check(
            name_,
            [this](const T& value) {
                // Skip if preconditions not met
                RC_PRE(checkPreconditions(value));
                
                // Classify the test case
                for (const auto& label : getClassifications(value)) {
                    RC_CLASSIFY(true, label);
                }
                
                // Run the actual property
                if (property_) {
                    property_(value);
                }
            },
            generator_
        ));
    }
    
    // Getters
    const std::string& getName() const { return name_; }
    const std::string& getDescription() const { return description_; }
    
protected:
    std::string name_;
    std::string description_;
    GeneratorType generator_;
    PropertyFunction property_;
    std::vector<PreconditionFunction> preconditions_;
    std::vector<std::function<std::string(const T&)>> classifiers_;
};

/**
 * @brief Specialization for properties with multiple input types
 * 
 * @tparam Args Variadic template for multiple input types
 */
template<typename... Args>
class MultiPropertyBase {
public:
    using PropertyFunction = std::function<void(const Args&...)>;
    using GeneratorTuple = std::tuple<rc::Gen<Args>...>;
    
    MultiPropertyBase(const std::string& name, const std::string& description)
        : name_(name), description_(description) {}
    
    virtual ~MultiPropertyBase() = default;
    
    MultiPropertyBase& withGenerators(rc::Gen<Args>... gens) {
        generators_ = std::make_tuple(gens...);
        return *this;
    }
    
    MultiPropertyBase& withProperty(PropertyFunction property) {
        property_ = property;
        return *this;
    }
    
    virtual void check() const {
        std::apply([this](auto... gens) {
            RC_ASSERT(rc::check(
                name_,
                [this](const Args&... args) {
                    if (property_) {
                        property_(args...);
                    }
                },
                gens...
            ));
        }, generators_);
    }
    
protected:
    std::string name_;
    std::string description_;
    GeneratorTuple generators_;
    PropertyFunction property_;
};

} // namespace jwwlib_pbt

#endif // PBT_PROPERTY_BASE_H