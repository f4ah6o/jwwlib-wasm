#ifndef PBT_JWW_TEST_ENTITIES_H
#define PBT_JWW_TEST_ENTITIES_H

#include <string>
#include <vector>
#include <ctime>
#include "dl_entities.h"

namespace jwwlib_pbt {

/**
 * @brief Test structures for JWW entities
 * These are simplified versions for testing purposes
 */

struct JWWPoint {
    double x;
    double y;
};

struct JWWLine {
    JWWPoint start;
    JWWPoint end;
    int layerIndex;
    int color;
    int lineType;
    
    JWWLine() : layerIndex(0), color(0), lineType(0) {}
};

struct JWWCircle {
    JWWPoint center;
    double radius;
    int layerIndex;
    int color;
    int lineType;
    
    JWWCircle() : radius(1.0), layerIndex(0), color(0), lineType(0) {}
};

struct JWWArc {
    JWWPoint center;
    double radius;
    double startAngle;
    double endAngle;
    int layerIndex;
    int color;
    int lineType;
    
    JWWArc() : radius(1.0), startAngle(0.0), endAngle(90.0), 
               layerIndex(0), color(0), lineType(0) {}
};

struct JWWText {
    std::string content;
    JWWPoint position;
    double height;
    double angle;
    int layerIndex;
    int color;
    
    JWWText() : height(10.0), angle(0.0), layerIndex(0), color(0) {}
};

struct JWWBlock {
    std::string name;
    JWWPoint basePoint;
    std::vector<JWWLine> lines;
    std::vector<JWWCircle> circles;
    std::vector<JWWArc> arcs;
    std::vector<JWWText> texts;
};

struct JWWHeader {
    std::string version;
    std::string creator;
    std::string encoding;
    std::time_t createTime;
    std::time_t updateTime;
    double scale;
    std::string paperSize;
    
    JWWHeader() : version("8.03a"), creator("jwwlib-wasm"), 
                  encoding("Shift-JIS"), scale(1.0), paperSize("A4") {
        createTime = std::time(nullptr);
        updateTime = std::time(nullptr);
    }
};

struct JWWEntities {
    std::vector<JWWLine> lines;
    std::vector<JWWCircle> circles;
    std::vector<JWWArc> arcs;
    std::vector<JWWText> texts;
};

struct JWWDocument {
    JWWHeader header;
    std::vector<DL_Layer> layers;
    JWWEntities entities;
    std::vector<JWWBlock> blocks;
    
    JWWDocument() {
        // Always add default layer "0"
        layers.push_back(DL_Layer("0", 0));
    }
};

} // namespace jwwlib_pbt

#endif // PBT_JWW_TEST_ENTITIES_H