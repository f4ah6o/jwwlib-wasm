#ifndef PBT_JWW_ENTITY_GENERATORS_H
#define PBT_JWW_ENTITY_GENERATORS_H

#include <rapidcheck.h>
#include "dl_entities.h"
#include "jwwdoc.h"

// Include all specific generators
#include "line_generator.h"
#include "circle_generator.h"
#include "arc_generator.h"
#include "text_generator.h"
#include "layer_generator.h"
#include "document_generator.h"

namespace jwwlib_pbt {
namespace generators {

/**
 * @brief Common constraints for JWW coordinates
 */
namespace Constraints {
    // JWW coordinate system bounds (in mm)
    constexpr double MIN_COORD = -10000.0;
    constexpr double MAX_COORD = 10000.0;
    
    // Reasonable size constraints
    constexpr double MIN_RADIUS = 0.1;
    constexpr double MAX_RADIUS = 5000.0;
    
    // Angle constraints (in degrees)
    constexpr double MIN_ANGLE = 0.0;
    constexpr double MAX_ANGLE = 360.0;
    
    // Text constraints
    constexpr double MIN_TEXT_HEIGHT = 0.1;
    constexpr double MAX_TEXT_HEIGHT = 1000.0;
    constexpr size_t MAX_TEXT_LENGTH = 1024;
    
    // Layer constraints
    constexpr int MAX_LAYERS = 256;
    constexpr size_t MAX_LAYER_NAME_LENGTH = 256;
}

/**
 * @brief Generate valid JWW coordinates
 */
inline rc::Gen<double> genCoordinate() {
    return rc::gen::inRange(Constraints::MIN_COORD, Constraints::MAX_COORD);
}

/**
 * @brief Generate valid radius values
 */
inline rc::Gen<double> genRadius() {
    return rc::gen::inRange(Constraints::MIN_RADIUS, Constraints::MAX_RADIUS);
}

/**
 * @brief Generate valid angle values in degrees
 */
inline rc::Gen<double> genAngle() {
    return rc::gen::inRange(Constraints::MIN_ANGLE, Constraints::MAX_ANGLE);
}

/**
 * @brief Generate valid text height
 */
inline rc::Gen<double> genTextHeight() {
    return rc::gen::inRange(Constraints::MIN_TEXT_HEIGHT, Constraints::MAX_TEXT_HEIGHT);
}

/**
 * @brief Generate valid layer flags
 */
inline rc::Gen<int> genLayerFlags() {
    return rc::gen::inRange(0, 7); // 1=frozen, 2=frozen by default, 4=locked
}

/**
 * @brief Generate valid layer names
 */
inline rc::Gen<std::string> genLayerName() {
    return rc::gen::apply(
        [](const std::string& prefix, int num) {
            return prefix + std::to_string(num);
        },
        rc::gen::element<std::string>({"Layer", "レイヤ", "図面", "寸法", "文字"}),
        rc::gen::inRange(0, 99)
    );
}

/**
 * @brief Generate Japanese text (Shift-JIS compatible)
 */
inline rc::Gen<std::string> genJapaneseText() {
    // Common Japanese characters that are safe in Shift-JIS
    static const std::vector<std::string> japaneseWords = {
        "建築", "図面", "寸法", "平面図", "立面図", "断面図",
        "基礎", "壁", "柱", "梁", "屋根", "窓", "扉",
        "１階", "２階", "３階", "地下", "屋上",
        "北", "南", "東", "西", "中央",
        "ＧＬ", "ＦＬ", "ＳＬ", "天井高", "階高"
    };
    
    return rc::gen::oneOf(
        rc::gen::element(japaneseWords),
        rc::gen::apply(
            [](const std::string& word, int num) {
                return word + std::to_string(num);
            },
            rc::gen::element(japaneseWords),
            rc::gen::inRange(1, 99)
        )
    );
}

/**
 * @brief Generate ASCII text
 */
inline rc::Gen<std::string> genAsciiText() {
    return rc::gen::container<std::string>(
        rc::gen::inRange('A', 'Z'),
        rc::gen::inRange(size_t(1), size_t(32))
    );
}

/**
 * @brief Generate mixed text (ASCII and Japanese)
 */
inline rc::Gen<std::string> genMixedText() {
    return rc::gen::oneOf(
        genAsciiText(),
        genJapaneseText(),
        rc::gen::apply(
            [](const std::string& ascii, const std::string& japanese) {
                return ascii + "_" + japanese;
            },
            genAsciiText(),
            genJapaneseText()
        )
    );
}

} // namespace generators
} // namespace jwwlib_pbt

#endif // PBT_JWW_ENTITY_GENERATORS_H