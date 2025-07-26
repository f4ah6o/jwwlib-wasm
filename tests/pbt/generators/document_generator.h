#ifndef PBT_DOCUMENT_GENERATOR_H
#define PBT_DOCUMENT_GENERATOR_H

#include <rapidcheck.h>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <ctime>
#include "dl_entities.h"
#include "../include/jww_test_entities.h"
#include "text_generator.h"
#include "layer_generator.h"

namespace jwwlib_pbt {
namespace generators {

/**
 * @brief JWWDocument generator for comprehensive testing
 */
class DocumentGenerator {
public:
    /**
     * @brief Generate a minimal valid JWWDocument
     */
    static rc::Gen<std::unique_ptr<JWWDocument>> genMinimalDocument() {
        return rc::gen::apply(
            []() {
                auto doc = std::make_unique<JWWDocument>();
                
                // Set minimal required properties
                doc->header.version = "8.03a";
                doc->header.creator = "jwwlib-wasm-pbt";
                
                // Add default layer
                doc->layers.push_back(DL_Layer("0", 0));
                
                return doc;
            }
        );
    }
    
    /**
     * @brief Generate a JWWDocument with basic entities
     */
    static rc::Gen<std::unique_ptr<JWWDocument>> genBasicDocument() {
        return rc::gen::apply(
            [](const std::vector<DL_Layer>& layers,
               const std::vector<JWWLine>& lines,
               const std::vector<JWWCircle>& circles,
               const std::vector<JWWArc>& arcs) {
                
                auto doc = std::make_unique<JWWDocument>();
                
                // Set header
                doc->header.version = "8.03a";
                doc->header.creator = "jwwlib-wasm-pbt";
                doc->header.createTime = std::time(nullptr);
                doc->header.updateTime = std::time(nullptr);
                
                // Add layers
                doc->layers = layers;
                
                // Add entities
                for (const auto& line : lines) {
                    doc->entities.lines.push_back(line);
                }
                for (const auto& circle : circles) {
                    doc->entities.circles.push_back(circle);
                }
                for (const auto& arc : arcs) {
                    doc->entities.arcs.push_back(arc);
                }
                
                return doc;
            },
            LayerGenerator::genLayerStructure(),
            rc::gen::container<std::vector<JWWLine>>(
                LineGenerator::genLine(),
                rc::gen::inRange(size_t(0), size_t(100))
            ),
            rc::gen::container<std::vector<JWWCircle>>(
                CircleGenerator::genCircle(),
                rc::gen::inRange(size_t(0), size_t(50))
            ),
            rc::gen::container<std::vector<JWWArc>>(
                ArcGenerator::genArc(),
                rc::gen::inRange(size_t(0), size_t(50))
            )
        );
    }
    
    /**
     * @brief Generate a JWWDocument with text entities
     */
    static rc::Gen<std::unique_ptr<JWWDocument>> genDocumentWithText() {
        return rc::gen::apply(
            [](const std::vector<DL_Layer>& layers,
               const std::vector<std::string>& textContents,
               const std::vector<double>& textHeights,
               const std::vector<double>& xPositions,
               const std::vector<double>& yPositions) {
                
                auto doc = std::make_unique<JWWDocument>();
                
                // Set header
                doc->header.version = "8.03a";
                doc->header.creator = "jwwlib-wasm-pbt";
                doc->header.encoding = "Shift-JIS";
                
                // Add layers
                doc->layers = layers;
                
                // Add text entities
                size_t numTexts = std::min({
                    textContents.size(),
                    textHeights.size(),
                    xPositions.size(),
                    yPositions.size()
                });
                
                for (size_t i = 0; i < numTexts; ++i) {
                    JWWText text;
                    text.content = textContents[i];
                    text.height = textHeights[i];
                    text.position.x = xPositions[i];
                    text.position.y = yPositions[i];
                    text.angle = 0.0;
                    text.layerIndex = i % layers.size();
                    
                    doc->entities.texts.push_back(text);
                }
                
                return doc;
            },
            LayerGenerator::genLayerStructure(),
            rc::gen::container<std::vector<std::string>>(
                TextGenerator::genMixedShiftJISText(),
                rc::gen::inRange(size_t(1), size_t(50))
            ),
            rc::gen::container<std::vector<double>>(
                rc::gen::inRange(1.0, 100.0),
                rc::gen::inRange(size_t(1), size_t(50))
            ),
            rc::gen::container<std::vector<double>>(
                rc::gen::inRange(-5000.0, 5000.0),
                rc::gen::inRange(size_t(1), size_t(50))
            ),
            rc::gen::container<std::vector<double>>(
                rc::gen::inRange(-5000.0, 5000.0),
                rc::gen::inRange(size_t(1), size_t(50))
            )
        );
    }
    
    /**
     * @brief Generate a complex JWWDocument with all entity types
     */
    static rc::Gen<std::unique_ptr<JWWDocument>> genComplexDocument() {
        return rc::gen::apply(
            []() {
                auto doc = std::make_unique<JWWDocument>();
                
                // Generate comprehensive layer structure
                auto layers = *LayerGenerator::genLayerStructure();
                doc->layers = layers;
                
                // Set header with full information
                doc->header.version = "8.03a";
                doc->header.creator = "jwwlib-wasm-pbt";
                doc->header.encoding = "Shift-JIS";
                doc->header.createTime = std::time(nullptr);
                doc->header.updateTime = std::time(nullptr);
                doc->header.scale = *rc::gen::element<double>({1.0, 10.0, 20.0, 50.0, 100.0, 200.0, 500.0});
                doc->header.paperSize = *rc::gen::element<std::string>({"A4", "A3", "A2", "A1", "A0"});
                
                // Generate entities distributed across layers
                int numLines = *rc::gen::inRange(10, 200);
                for (int i = 0; i < numLines; ++i) {
                    auto line = *LineGenerator::genLine();
                    line.layerIndex = i % layers.size();
                    doc->entities.lines.push_back(line);
                }
                
                int numCircles = *rc::gen::inRange(5, 100);
                for (int i = 0; i < numCircles; ++i) {
                    auto circle = *CircleGenerator::genCircle();
                    circle.layerIndex = i % layers.size();
                    doc->entities.circles.push_back(circle);
                }
                
                int numArcs = *rc::gen::inRange(5, 100);
                for (int i = 0; i < numArcs; ++i) {
                    auto arc = *ArcGenerator::genArc();
                    arc.layerIndex = i % layers.size();
                    doc->entities.arcs.push_back(arc);
                }
                
                int numTexts = *rc::gen::inRange(5, 50);
                for (int i = 0; i < numTexts; ++i) {
                    JWWText text;
                    text.content = *TextGenerator::genMixedShiftJISText();
                    text.height = *rc::gen::inRange(1.0, 100.0);
                    text.position.x = *rc::gen::inRange(-5000.0, 5000.0);
                    text.position.y = *rc::gen::inRange(-5000.0, 5000.0);
                    text.angle = *rc::gen::inRange(0.0, 360.0);
                    text.layerIndex = i % layers.size();
                    doc->entities.texts.push_back(text);
                }
                
                // Add blocks/groups
                int numBlocks = *rc::gen::inRange(0, 10);
                for (int i = 0; i < numBlocks; ++i) {
                    JWWBlock block;
                    block.name = "Block_" + std::to_string(i);
                    block.basePoint.x = *rc::gen::inRange(-1000.0, 1000.0);
                    block.basePoint.y = *rc::gen::inRange(-1000.0, 1000.0);
                    
                    // Add some entities to the block
                    int numBlockEntities = *rc::gen::inRange(1, 10);
                    for (int j = 0; j < numBlockEntities; ++j) {
                        auto line = *LineGenerator::genLine();
                        block.lines.push_back(line);
                    }
                    
                    doc->blocks.push_back(block);
                }
                
                return doc;
            }
        );
    }
    
    /**
     * @brief Generate a JWWDocument with specific characteristics for testing
     */
    static rc::Gen<std::unique_ptr<JWWDocument>> genDocumentWithCharacteristics(
        const std::string& characteristic) {
        
        if (characteristic == "empty") {
            return genMinimalDocument();
        } else if (characteristic == "text_heavy") {
            return genDocumentWithText();
        } else if (characteristic == "large") {
            return rc::gen::apply(
                []() {
                    auto doc = std::make_unique<JWWDocument>();
                    
                    // Generate many entities
                    int numEntities = *rc::gen::inRange(1000, 5000);
                    for (int i = 0; i < numEntities; ++i) {
                        auto line = *LineGenerator::genLine();
                        doc->entities.lines.push_back(line);
                    }
                    
                    return doc;
                }
            );
        } else if (characteristic == "multi_layer") {
            return rc::gen::apply(
                []() {
                    auto doc = std::make_unique<JWWDocument>();
                    
                    // Generate many layers
                    int numLayers = *rc::gen::inRange(50, 256);
                    for (int i = 0; i < numLayers; ++i) {
                        doc->layers.push_back(
                            DL_Layer("Layer_" + std::to_string(i), 0)
                        );
                    }
                    
                    return doc;
                }
            );
        } else {
            return genComplexDocument();
        }
    }
};

} // namespace generators
} // namespace jwwlib_pbt

#endif // PBT_DOCUMENT_GENERATOR_H