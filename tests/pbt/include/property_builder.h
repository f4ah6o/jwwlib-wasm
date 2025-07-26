#ifndef PBT_PROPERTY_BUILDER_H
#define PBT_PROPERTY_BUILDER_H

#include "property_base.h"
#include <vector>
#include <memory>

namespace jwwlib_pbt {

/**
 * @brief Builder class for composing multiple properties
 * 
 * This class allows combining multiple properties into composite properties
 * with various logical relationships (AND, OR, IMPLIES).
 */
class PropertyBuilder {
public:
    /**
     * @brief Interface for runnable properties
     */
    class IProperty {
    public:
        virtual ~IProperty() = default;
        virtual void check() const = 0;
        virtual const std::string& getName() const = 0;
    };
    
    /**
     * @brief Wrapper to make PropertyBase runnable
     */
    template<typename T>
    class PropertyWrapper : public IProperty {
    public:
        explicit PropertyWrapper(std::shared_ptr<PropertyBase<T>> property)
            : property_(property) {}
        
        void check() const override {
            property_->check();
        }
        
        const std::string& getName() const override {
            return property_->getName();
        }
        
    private:
        std::shared_ptr<PropertyBase<T>> property_;
    };
    
    /**
     * @brief Composite property that checks all sub-properties
     */
    class CompositeProperty : public IProperty {
    public:
        enum class CompositionType {
            AND,     // All properties must pass
            OR,      // At least one property must pass
            SEQUENCE // Properties are checked in order
        };
        
        CompositeProperty(const std::string& name, CompositionType type)
            : name_(name), type_(type) {}
        
        void addProperty(std::shared_ptr<IProperty> property) {
            properties_.push_back(property);
        }
        
        void check() const override {
            switch (type_) {
                case CompositionType::AND:
                    checkAll();
                    break;
                case CompositionType::OR:
                    checkAny();
                    break;
                case CompositionType::SEQUENCE:
                    checkSequence();
                    break;
            }
        }
        
        const std::string& getName() const override {
            return name_;
        }
        
    private:
        void checkAll() const {
            for (const auto& prop : properties_) {
                prop->check();
            }
        }
        
        void checkAny() const {
            bool anyPassed = false;
            std::vector<std::string> failures;
            
            for (const auto& prop : properties_) {
                try {
                    prop->check();
                    anyPassed = true;
                    break;
                } catch (const std::exception& e) {
                    failures.push_back(prop->getName() + ": " + e.what());
                }
            }
            
            if (!anyPassed) {
                std::string message = "All properties failed:\n";
                for (const auto& failure : failures) {
                    message += "  - " + failure + "\n";
                }
                throw std::runtime_error(message);
            }
        }
        
        void checkSequence() const {
            for (size_t i = 0; i < properties_.size(); ++i) {
                try {
                    properties_[i]->check();
                } catch (const std::exception& e) {
                    throw std::runtime_error(
                        "Property " + std::to_string(i + 1) + " of " + 
                        std::to_string(properties_.size()) + " failed: " + e.what()
                    );
                }
            }
        }
        
        std::string name_;
        CompositionType type_;
        std::vector<std::shared_ptr<IProperty>> properties_;
    };
    
    /**
     * @brief Create a property that requires all sub-properties to pass
     */
    static std::shared_ptr<CompositeProperty> all(const std::string& name) {
        return std::make_shared<CompositeProperty>(
            name, CompositeProperty::CompositionType::AND
        );
    }
    
    /**
     * @brief Create a property that requires at least one sub-property to pass
     */
    static std::shared_ptr<CompositeProperty> any(const std::string& name) {
        return std::make_shared<CompositeProperty>(
            name, CompositeProperty::CompositionType::OR
        );
    }
    
    /**
     * @brief Create a property that checks sub-properties in sequence
     */
    static std::shared_ptr<CompositeProperty> sequence(const std::string& name) {
        return std::make_shared<CompositeProperty>(
            name, CompositeProperty::CompositionType::SEQUENCE
        );
    }
    
    /**
     * @brief Create an implication property (if A then B)
     */
    template<typename T>
    static std::shared_ptr<PropertyBase<T>> implies(
        const std::string& name,
        std::function<bool(const T&)> condition,
        std::function<void(const T&)> consequence
    ) {
        auto property = std::make_shared<PropertyBase<T>>(
            name, 
            "Implication property: if condition holds, then consequence must hold"
        );
        
        property->withProperty([condition, consequence](const T& value) {
            if (condition(value)) {
                consequence(value);
            }
        });
        
        return property;
    }
    
    /**
     * @brief Create a property that checks invariants are preserved
     */
    template<typename T, typename R>
    static std::shared_ptr<PropertyBase<T>> invariant(
        const std::string& name,
        std::function<R(const T&)> operation,
        std::function<bool(const T&, const R&)> invariantCheck
    ) {
        auto property = std::make_shared<PropertyBase<T>>(
            name,
            "Invariant property: operation preserves invariant"
        );
        
        property->withProperty([operation, invariantCheck](const T& value) {
            R result = operation(value);
            RC_ASSERT(invariantCheck(value, result));
        });
        
        return property;
    }
    
    /**
     * @brief Create a property group for related properties
     */
    class PropertyGroup {
    public:
        PropertyGroup(const std::string& name) : name_(name) {}
        
        template<typename T>
        void add(std::shared_ptr<PropertyBase<T>> property) {
            properties_.push_back(
                std::make_shared<PropertyWrapper<T>>(property)
            );
        }
        
        void runAll() const {
            std::cout << "Running property group: " << name_ << std::endl;
            for (const auto& prop : properties_) {
                std::cout << "  Checking: " << prop->getName() << "...";
                prop->check();
                std::cout << " PASSED" << std::endl;
            }
        }
        
    private:
        std::string name_;
        std::vector<std::shared_ptr<IProperty>> properties_;
    };
};

} // namespace jwwlib_pbt

#endif // PBT_PROPERTY_BUILDER_H