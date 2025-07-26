#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <rapidcheck.h>

namespace jwwlib {
namespace pbt {
namespace generators {

// Forward declaration
template <typename T>
class LazyGenerator;

// Helper trait to detect if a type is a LazyGenerator
template <typename T>
struct is_lazy_generator : std::false_type {};

template <typename T>
struct is_lazy_generator<LazyGenerator<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_lazy_generator_v = is_lazy_generator<T>::value;

// LazyGenerator: A generator that defers evaluation until needed
template <typename T>
class LazyGenerator {
public:
    using value_type = T;
    using generator_fn = std::function<rc::Gen<T>()>;

    // Construct from a function that returns a generator
    explicit LazyGenerator(generator_fn fn) : generator_fn_(std::move(fn)) {}

    // Construct from an existing generator (eager)
    explicit LazyGenerator(rc::Gen<T> gen) 
        : generator_fn_([gen = std::move(gen)]() { return gen; }) {}

    // Convert to RapidCheck generator
    operator rc::Gen<T>() const {
        return generator_fn_();
    }

    // Force evaluation
    rc::Gen<T> force() const {
        return generator_fn_();
    }

    // Map: Transform the generated values lazily
    template <typename F>
    auto map(F&& f) const -> LazyGenerator<decltype(f(std::declval<T>()))> {
        using U = decltype(f(std::declval<T>()));
        return LazyGenerator<U>([gen_fn = generator_fn_, f = std::forward<F>(f)]() {
            return rc::gen::map(gen_fn(), f);
        });
    }

    // FlatMap: Chain generators lazily
    template <typename F>
    auto flatMap(F&& f) const -> LazyGenerator<typename decltype(f(std::declval<T>()))::value_type> {
        using U = typename decltype(f(std::declval<T>()))::value_type;
        return LazyGenerator<U>([gen_fn = generator_fn_, f = std::forward<F>(f)]() {
            return rc::gen::mapcat(gen_fn(), [f](const T& value) {
                if constexpr (is_lazy_generator_v<decltype(f(value))>) {
                    return f(value).force();
                } else {
                    return f(value);
                }
            });
        });
    }

    // Filter: Add constraints lazily
    LazyGenerator<T> filter(std::function<bool(const T&)> predicate) const {
        return LazyGenerator<T>([gen_fn = generator_fn_, predicate]() {
            return rc::gen::suchThat(gen_fn(), predicate);
        });
    }

    // Scale: Adjust size parameter lazily
    LazyGenerator<T> scale(double factor) const {
        return LazyGenerator<T>([gen_fn = generator_fn_, factor]() {
            return rc::gen::scale(factor, gen_fn());
        });
    }

    // WithSize: Fix the size parameter
    LazyGenerator<T> withSize(int size) const {
        return LazyGenerator<T>([gen_fn = generator_fn_, size]() {
            return rc::gen::resize(size, gen_fn());
        });
    }

private:
    generator_fn generator_fn_;
};

// Helper functions for creating lazy generators

// Create a lazy generator from a value
template <typename T>
LazyGenerator<T> lazy_just(T value) {
    return LazyGenerator<T>([value = std::move(value)]() {
        return rc::gen::just(value);
    });
}

// Create a lazy generator that chooses from a collection
template <typename Container>
auto lazy_element_of(Container container) 
    -> LazyGenerator<typename Container::value_type> {
    using T = typename Container::value_type;
    return LazyGenerator<T>([container = std::move(container)]() {
        return rc::gen::elementOf(container);
    });
}

// Create a lazy generator that chooses between multiple generators
template <typename T, typename... Gens>
LazyGenerator<T> lazy_one_of(Gens... gens) {
    return LazyGenerator<T>([gens...]() {
        return rc::gen::oneOf(gens.force()...);
    });
}

// Lazy container generators

// Lazy vector generator with size range
template <typename T>
LazyGenerator<std::vector<T>> lazy_vector(LazyGenerator<T> elem_gen, 
                                         size_t min_size = 0, 
                                         size_t max_size = 100) {
    return LazyGenerator<std::vector<T>>([elem_gen, min_size, max_size]() {
        return rc::gen::container<std::vector<T>>(
            rc::gen::inRange<size_t>(min_size, max_size + 1),
            elem_gen.force()
        );
    });
}

// Lazy non-empty vector generator
template <typename T>
LazyGenerator<std::vector<T>> lazy_non_empty_vector(LazyGenerator<T> elem_gen,
                                                   size_t max_size = 100) {
    return lazy_vector(elem_gen, 1, max_size);
}

// Lazy fixed-size vector generator
template <typename T>
LazyGenerator<std::vector<T>> lazy_vector_of_size(LazyGenerator<T> elem_gen,
                                                 size_t size) {
    return LazyGenerator<std::vector<T>>([elem_gen, size]() {
        return rc::gen::container<std::vector<T>>(size, elem_gen.force());
    });
}

// Combinator for building complex generators lazily

template <typename T>
class LazyGeneratorBuilder {
public:
    LazyGeneratorBuilder() = default;

    // Add a base generator
    LazyGeneratorBuilder& with_base(LazyGenerator<T> gen, double weight = 1.0) {
        weighted_generators_.emplace_back(std::move(gen), weight);
        return *this;
    }

    // Add a recursive case
    LazyGeneratorBuilder& with_recursive(
        std::function<LazyGenerator<T>(LazyGenerator<T>)> recursive_fn,
        double weight = 1.0) {
        recursive_cases_.emplace_back(std::move(recursive_fn), weight);
        return *this;
    }

    // Build the final generator with proper recursion handling
    LazyGenerator<T> build() const {
        return LazyGenerator<T>([this]() {
            return rc::gen::recursive<T>([this](const rc::Gen<T>& recurse) {
                std::vector<rc::Gen<T>> all_gens;
                
                // Add base generators
                for (const auto& [gen, weight] : weighted_generators_) {
                    all_gens.push_back(rc::gen::scale(weight, gen.force()));
                }
                
                // Add recursive generators
                for (const auto& [recursive_fn, weight] : recursive_cases_) {
                    LazyGenerator<T> lazy_recurse([recurse]() { return recurse; });
                    auto recursive_gen = recursive_fn(lazy_recurse);
                    all_gens.push_back(rc::gen::scale(weight, recursive_gen.force()));
                }
                
                return rc::gen::oneOf(all_gens);
            });
        });
    }

private:
    std::vector<std::pair<LazyGenerator<T>, double>> weighted_generators_;
    std::vector<std::pair<std::function<LazyGenerator<T>(LazyGenerator<T>)>, double>> 
        recursive_cases_;
};

// Memory-efficient shrinking for lazy generators

template <typename T>
class LazyShrinkable {
public:
    using shrink_fn = std::function<std::vector<LazyGenerator<T>>(const T&)>;

    LazyShrinkable(LazyGenerator<T> gen, shrink_fn shrinker)
        : generator_(std::move(gen)), shrinker_(std::move(shrinker)) {}

    LazyGenerator<T> with_custom_shrink() const {
        return LazyGenerator<T>([gen = generator_, shrink = shrinker_]() {
            return rc::gen::shrinkable<T>([gen, shrink](const rc::Random& random, int size) {
                auto value = rc::gen::exec(gen.force(), random, size);
                
                return rc::shrinkable::map(
                    rc::shrinkable::all<rc::Shrinkable<T>>(),
                    [value, shrink](const std::vector<rc::Shrinkable<T>>& shrinkables) {
                        std::vector<rc::Shrinkable<T>> result;
                        auto lazy_shrinks = shrink(value);
                        
                        for (const auto& lazy_gen : lazy_shrinks) {
                            result.push_back(rc::shrinkable::just(
                                rc::gen::exec(lazy_gen.force(), random, size)
                            ));
                        }
                        
                        return rc::shrinkable::shrinkRecur(value, result);
                    }
                );
            });
        });
    }

private:
    LazyGenerator<T> generator_;
    shrink_fn shrinker_;
};

// Utility for creating generators that share expensive computations

template <typename T, typename SharedData>
class SharedLazyGenerator {
public:
    using generator_fn = std::function<rc::Gen<T>(const SharedData&)>;
    using shared_fn = std::function<SharedData()>;

    SharedLazyGenerator(shared_fn compute_shared, generator_fn gen_fn)
        : compute_shared_(std::move(compute_shared)), 
          generator_fn_(std::move(gen_fn)) {}

    LazyGenerator<T> instantiate() const {
        // Compute shared data once per test case
        auto shared_ptr = std::make_shared<std::optional<SharedData>>();
        
        return LazyGenerator<T>([shared_ptr, compute = compute_shared_, 
                                gen_fn = generator_fn_]() {
            if (!shared_ptr->has_value()) {
                *shared_ptr = compute();
            }
            return gen_fn(shared_ptr->value());
        });
    }

private:
    shared_fn compute_shared_;
    generator_fn generator_fn_;
};

// Helper function for creating shared lazy generators
template <typename T, typename SharedData>
SharedLazyGenerator<T, SharedData> lazy_with_shared(
    std::function<SharedData()> compute_shared,
    std::function<rc::Gen<T>(const SharedData&)> gen_fn) {
    return SharedLazyGenerator<T, SharedData>(
        std::move(compute_shared), std::move(gen_fn));
}

}  // namespace generators
}  // namespace pbt
}  // namespace jwwlib