#pragma once

#include "../framework/generators/lazy_generator.hpp"
#include "jww_generators.hpp"
#include <memory>
#include <vector>

namespace jwwlib {
namespace pbt {
namespace generators {

// Lazy generators for JWW entities that defer computation

// Lazy line generator
inline LazyGenerator<JWWLine> lazy_jww_line() {
    return LazyGenerator<JWWLine>([]() {
        return jww_line();
    });
}

// Lazy circle generator
inline LazyGenerator<JWWCircle> lazy_jww_circle() {
    return LazyGenerator<JWWCircle>([]() {
        return jww_circle();
    });
}

// Lazy arc generator
inline LazyGenerator<JWWArc> lazy_jww_arc() {
    return LazyGenerator<JWWArc>([]() {
        return jww_arc();
    });
}

// Lazy text generator with shared character set computation
inline LazyGenerator<JWWText> lazy_jww_text() {
    auto shared_gen = lazy_with_shared<JWWText, std::vector<std::string>>(
        []() {
            // Compute valid Shift-JIS strings once
            return std::vector<std::string>{
                "テスト", "図面", "寸法", "建築", "設計",
                "Test", "Drawing", "Dimension", "Architecture", "Design"
            };
        },
        [](const std::vector<std::string>& valid_strings) {
            return rc::gen::map(
                rc::gen::tuple(
                    rc::gen::elementOf(valid_strings),
                    coordinate_gen(),
                    rc::gen::inRange(0.0, 360.0),
                    rc::gen::inRange(1.0, 100.0),
                    rc::gen::inRange(1.0, 100.0)
                ),
                [](const auto& tuple) {
                    const auto& [text, pos, angle, width, height] = tuple;
                    return JWWText{text, pos, angle, width, height};
                }
            );
        }
    );
    
    return shared_gen.instantiate();
}

// Lazy layer generator that builds layers incrementally
inline LazyGenerator<JWWLayer> lazy_jww_layer(size_t max_entities = 100) {
    return LazyGeneratorBuilder<JWWLayer>()
        .with_base(LazyGenerator<JWWLayer>([]() {
            // Base case: empty layer
            return rc::gen::map(
                rc::gen::tuple(
                    layer_name_gen(),
                    rc::gen::inRange(0, 16),
                    rc::gen::arbitrary<bool>()
                ),
                [](const auto& tuple) {
                    const auto& [name, group, visible] = tuple;
                    return JWWLayer{name, {}, group, visible};
                }
            );
        }), 0.1)
        .with_recursive([max_entities](LazyGenerator<JWWLayer> recurse) {
            // Recursive case: add entities to layer
            return LazyGenerator<JWWLayer>([recurse, max_entities]() {
                return rc::gen::mapcat(
                    recurse.force(),
                    [max_entities](const JWWLayer& base_layer) {
                        if (base_layer.entities.size() >= max_entities) {
                            return rc::gen::just(base_layer);
                        }
                        
                        // Lazily choose which entity type to add
                        return rc::gen::map(
                            rc::gen::oneOf(
                                rc::gen::map(jww_line(), [](const JWWLine& line) {
                                    return JWWEntity{line};
                                }),
                                rc::gen::map(jww_circle(), [](const JWWCircle& circle) {
                                    return JWWEntity{circle};
                                }),
                                rc::gen::map(jww_arc(), [](const JWWArc& arc) {
                                    return JWWEntity{arc};
                                }),
                                rc::gen::map(jww_text_gen(), [](const JWWText& text) {
                                    return JWWEntity{text};
                                })
                            ),
                            [base_layer](const JWWEntity& entity) {
                                JWWLayer new_layer = base_layer;
                                new_layer.entities.push_back(entity);
                                return new_layer;
                            }
                        );
                    }
                );
            });
        }, 0.9)
        .build();
}

// Lazy document generator with memory-efficient layer generation
inline LazyGenerator<JWWDocument> lazy_jww_document(
    size_t min_layers = 1,
    size_t max_layers = 16,
    size_t max_entities_per_layer = 50) {
    
    return LazyGenerator<JWWDocument>([min_layers, max_layers, max_entities_per_layer]() {
        // Generate layer count first
        return rc::gen::mapcat(
            rc::gen::inRange(min_layers, max_layers + 1),
            [max_entities_per_layer](size_t layer_count) {
                // Generate layers lazily
                std::vector<rc::Gen<JWWLayer>> layer_gens;
                for (size_t i = 0; i < layer_count; ++i) {
                    layer_gens.push_back(
                        lazy_jww_layer(max_entities_per_layer).force()
                    );
                }
                
                return rc::gen::map(
                    rc::gen::tuple(
                        rc::gen::sequenceContainer<std::vector<JWWLayer>>(layer_gens),
                        document_info_gen()
                    ),
                    [](const auto& tuple) {
                        const auto& [layers, info] = tuple;
                        return JWWDocument{layers, info};
                    }
                );
            }
        );
    });
}

// Specialized lazy generators for testing specific scenarios

// Generate documents with predominantly one entity type
inline LazyGenerator<JWWDocument> lazy_jww_document_with_entity_bias(
    EntityType primary_type,
    double bias = 0.8) {
    
    return LazyGenerator<JWWDocument>([primary_type, bias]() {
        auto biased_entity_gen = [primary_type, bias]() {
            if (rc::gen::exec(rc::gen::arbitrary<double>()) < bias) {
                switch (primary_type) {
                    case EntityType::LINE:
                        return rc::gen::map(jww_line(), 
                            [](const JWWLine& line) { return JWWEntity{line}; });
                    case EntityType::CIRCLE:
                        return rc::gen::map(jww_circle(), 
                            [](const JWWCircle& circle) { return JWWEntity{circle}; });
                    case EntityType::ARC:
                        return rc::gen::map(jww_arc(), 
                            [](const JWWArc& arc) { return JWWEntity{arc}; });
                    case EntityType::TEXT:
                        return rc::gen::map(jww_text_gen(), 
                            [](const JWWText& text) { return JWWEntity{text}; });
                    default:
                        return jww_entity();
                }
            } else {
                return jww_entity();
            }
        };
        
        return rc::gen::map(
            rc::gen::tuple(
                rc::gen::container<std::vector<JWWLayer>>(
                    rc::gen::inRange<size_t>(1, 5),
                    rc::gen::map(
                        rc::gen::tuple(
                            layer_name_gen(),
                            rc::gen::container<std::vector<JWWEntity>>(
                                rc::gen::inRange<size_t>(10, 50),
                                biased_entity_gen()
                            ),
                            rc::gen::inRange(0, 16),
                            rc::gen::arbitrary<bool>()
                        ),
                        [](const auto& tuple) {
                            const auto& [name, entities, group, visible] = tuple;
                            return JWWLayer{name, entities, group, visible};
                        }
                    )
                ),
                document_info_gen()
            ),
            [](const auto& tuple) {
                const auto& [layers, info] = tuple;
                return JWWDocument{layers, info};
            }
        );
    });
}

// Generate large documents lazily with controlled memory usage
inline LazyGenerator<JWWDocument> lazy_large_jww_document(
    size_t target_entity_count = 10000) {
    
    return LazyGenerator<JWWDocument>([target_entity_count]() {
        // Calculate layer distribution
        size_t layer_count = std::min<size_t>(16, target_entity_count / 100 + 1);
        size_t entities_per_layer = target_entity_count / layer_count;
        
        // Use shared entity templates to reduce memory
        auto shared_templates = lazy_with_shared<JWWDocument, 
            std::tuple<std::vector<JWWLine>, std::vector<JWWCircle>>>(
            []() {
                // Generate template entities once
                auto line_templates = rc::gen::exec(
                    rc::gen::container<std::vector<JWWLine>>(10, jww_line())
                );
                auto circle_templates = rc::gen::exec(
                    rc::gen::container<std::vector<JWWCircle>>(10, jww_circle())
                );
                return std::make_tuple(line_templates, circle_templates);
            },
            [layer_count, entities_per_layer](const auto& templates) {
                const auto& [line_templates, circle_templates] = templates;
                
                // Generate layers using templates with variations
                return rc::gen::map(
                    rc::gen::container<std::vector<JWWLayer>>(
                        layer_count,
                        rc::gen::map(
                            rc::gen::tuple(
                                layer_name_gen(),
                                rc::gen::inRange(0, 16),
                                rc::gen::arbitrary<bool>()
                            ),
                            [&line_templates, &circle_templates, entities_per_layer]
                            (const auto& layer_props) {
                                const auto& [name, group, visible] = layer_props;
                                
                                std::vector<JWWEntity> entities;
                                entities.reserve(entities_per_layer);
                                
                                for (size_t i = 0; i < entities_per_layer; ++i) {
                                    if (i % 2 == 0) {
                                        // Use line template with position offset
                                        auto line = line_templates[i % line_templates.size()];
                                        line.start.x += i * 10.0;
                                        line.end.x += i * 10.0;
                                        entities.push_back(JWWEntity{line});
                                    } else {
                                        // Use circle template with position offset
                                        auto circle = circle_templates[i % circle_templates.size()];
                                        circle.center.x += i * 10.0;
                                        entities.push_back(JWWEntity{circle});
                                    }
                                }
                                
                                return JWWLayer{name, entities, group, visible};
                            }
                        )
                    ),
                    [](const std::vector<JWWLayer>& layers) {
                        auto info = rc::gen::exec(document_info_gen());
                        return JWWDocument{layers, info};
                    }
                );
            }
        );
        
        return shared_templates.instantiate().force();
    });
}

}  // namespace generators
}  // namespace pbt
}  // namespace jwwlib