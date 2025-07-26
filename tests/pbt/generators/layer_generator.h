#ifndef PBT_LAYER_GENERATOR_H
#define PBT_LAYER_GENERATOR_H

#include <rapidcheck.h>
#include <string>
#include <vector>
#include <memory>
#include "dl_entities.h"
#include "jwwdoc.h"
#include "text_generator.h"

namespace jwwlib_pbt {
namespace generators {

/**
 * @brief Layer structure generators for JWW
 */
class LayerGenerator {
public:
    /**
     * @brief Generate a single DL_Layer
     */
    static rc::Gen<DL_Layer> genLayer() {
        return rc::gen::apply(
            [](const std::string& name, int flags) {
                return DL_Layer(name, flags);
            },
            rc::gen::oneOf(
                // ASCII layer names
                rc::gen::apply(
                    [](const std::string& prefix, int num) {
                        return prefix + std::to_string(num);
                    },
                    rc::gen::element<std::string>({"Layer", "LAYER", "L", "level"}),
                    rc::gen::inRange(0, 99)
                ),
                // Japanese layer names
                TextGenerator::genArchitecturalTerm(),
                // Standard CAD layer names
                rc::gen::element<std::string>({
                    "0", "Defpoints", "Dimensions", "Text", "Hatches",
                    "Construction", "Hidden", "Center", "Phantom"
                })
            ),
            rc::gen::inRange(0, 7) // Layer flags: 1=frozen, 2=frozen by default, 4=locked
        );
    }
    
    /**
     * @brief Generate a layer with specific characteristics
     */
    static rc::Gen<DL_Layer> genLayerWithType(const std::string& type) {
        if (type == "frozen") {
            return rc::gen::apply(
                [](const std::string& name) {
                    return DL_Layer(name, 1); // Frozen flag
                },
                genLayerName()
            );
        } else if (type == "locked") {
            return rc::gen::apply(
                [](const std::string& name) {
                    return DL_Layer(name, 4); // Locked flag
                },
                genLayerName()
            );
        } else if (type == "frozen_by_default") {
            return rc::gen::apply(
                [](const std::string& name) {
                    return DL_Layer(name, 2); // Frozen by default flag
                },
                genLayerName()
            );
        } else {
            return genLayer();
        }
    }
    
    /**
     * @brief Generate a complete layer structure for a JWW document
     */
    static rc::Gen<std::vector<DL_Layer>> genLayerStructure() {
        return rc::gen::oneOf(
            // Minimal layer structure
            rc::gen::just(std::vector<DL_Layer>{
                DL_Layer("0", 0) // Default layer "0" is always present
            }),
            
            // Standard architectural layers
            rc::gen::just(std::vector<DL_Layer>{
                DL_Layer("0", 0),
                DL_Layer("通り芯", 0),
                DL_Layer("壁", 0),
                DL_Layer("建具", 0),
                DL_Layer("寸法", 0),
                DL_Layer("文字", 0),
                DL_Layer("家具", 0),
                DL_Layer("設備", 0)
            }),
            
            // Mixed layers
            rc::gen::apply(
                [](const std::vector<DL_Layer>& customLayers) {
                    std::vector<DL_Layer> layers;
                    layers.push_back(DL_Layer("0", 0)); // Always include layer "0"
                    
                    // Add custom layers, ensuring unique names
                    std::set<std::string> usedNames{"0"};
                    for (const auto& layer : customLayers) {
                        if (usedNames.find(layer.name) == usedNames.end()) {
                            layers.push_back(layer);
                            usedNames.insert(layer.name);
                        }
                    }
                    
                    return layers;
                },
                rc::gen::container<std::vector<DL_Layer>>(
                    genLayer(),
                    rc::gen::inRange(size_t(1), size_t(15))
                )
            )
        );
    }
    
    /**
     * @brief Generate layer group structure (JWW specific)
     */
    static rc::Gen<std::map<int, std::vector<int>>> genLayerGroups() {
        return rc::gen::apply(
            []() {
                std::map<int, std::vector<int>> groups;
                
                // JWW supports up to 16 layer groups (0-15)
                int numGroups = *rc::gen::inRange(1, 5);
                
                for (int g = 0; g < numGroups; ++g) {
                    std::vector<int> layersInGroup;
                    int numLayersInGroup = *rc::gen::inRange(1, 8);
                    
                    for (int l = 0; l < numLayersInGroup; ++l) {
                        layersInGroup.push_back(*rc::gen::inRange(0, 255));
                    }
                    
                    groups[g] = layersInGroup;
                }
                
                return groups;
            }
        );
    }
    
private:
    /**
     * @brief Generate layer names
     */
    static rc::Gen<std::string> genLayerName() {
        return rc::gen::oneOf(
            // ASCII names
            rc::gen::apply(
                [](const std::string& prefix, int num) {
                    return prefix + std::to_string(num);
                },
                rc::gen::element<std::string>({"Layer", "LAYER", "L"}),
                rc::gen::inRange(0, 99)
            ),
            // Japanese architectural layer names
            rc::gen::element<std::string>({
                "通り芯", "壁", "柱", "建具", "寸法", "文字",
                "家具", "設備", "仕上", "構造", "基礎", "外構",
                "電気", "給排水", "空調", "防災", "詳細", "凡例"
            }),
            // Standard CAD layer names
            rc::gen::element<std::string>({
                "0", "Defpoints", "Dims", "Text", "Hatch",
                "Construction", "Hidden", "Center", "Phantom",
                "Viewport", "Title", "Grid", "Guide"
            })
        );
    }
};

} // namespace generators
} // namespace jwwlib_pbt

#endif // PBT_LAYER_GENERATOR_H