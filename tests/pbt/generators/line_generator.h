#ifndef PBT_LINE_GENERATOR_H
#define PBT_LINE_GENERATOR_H

#include <rapidcheck.h>
#include "dl_entities.h"
#include "../include/jww_test_entities.h"

namespace jwwlib_pbt {
namespace generators {

/**
 * @brief Generator for JWW Lines
 */
class LineGenerator {
private:
    static constexpr double MIN_COORD = -10000.0;
    static constexpr double MAX_COORD = 10000.0;
    
    static rc::Gen<double> genCoordinate() {
        return rc::gen::inRange(MIN_COORD, MAX_COORD);
    }
    
public:
    /**
     * @brief Generate a completely random line
     */
    static rc::Gen<JWWLine> genLine() {
        return rc::gen::apply(
            [](double x1, double y1, double x2, double y2, int layer, int color, int lineType) {
                JWWLine line;
                line.start.x = x1;
                line.start.y = y1;
                line.end.x = x2;
                line.end.y = y2;
                line.layerIndex = layer;
                line.color = color;
                line.lineType = lineType;
                return line;
            },
            genCoordinate(), genCoordinate(),
            genCoordinate(), genCoordinate(),
            rc::gen::inRange(0, 15),  // layer index
            rc::gen::inRange(0, 255), // color
            rc::gen::inRange(0, 7)    // line type
        );
    }
    
    /**
     * @brief Generate a DL_LineData (for compatibility)
     */
    static rc::Gen<DL_LineData> arbitrary() {
        return rc::gen::apply(
            [](double x1, double y1, double x2, double y2) {
                return DL_LineData(x1, y1, 0.0, x2, y2, 0.0);
            },
            genCoordinate(), genCoordinate(),
            genCoordinate(), genCoordinate()
        );
    }
    
    /**
     * @brief Generate a horizontal line
     */
    static rc::Gen<JWWLine> horizontal() {
        return rc::gen::apply(
            [](double x1, double y, double x2) {
                JWWLine line;
                line.start.x = x1;
                line.start.y = y;
                line.end.x = x2;
                line.end.y = y;
                return line;
            },
            genCoordinate(), genCoordinate(), genCoordinate()
        );
    }
    
    /**
     * @brief Generate a vertical line
     */
    static rc::Gen<DL_LineData> vertical() {
        return rc::gen::apply(
            [](double x, double y1, double y2) {
                return DL_LineData(x, y1, 0.0, x, y2, 0.0);
            },
            genCoordinate(), genCoordinate(), genCoordinate()
        );
    }
    
    /**
     * @brief Generate a line with specific length
     */
    static rc::Gen<DL_LineData> withLength(double length) {
        return rc::gen::apply(
            [length](double x, double y, double angle) {
                double rad = angle * M_PI / 180.0;
                double x2 = x + length * cos(rad);
                double y2 = y + length * sin(rad);
                return DL_LineData(x, y, 0.0, x2, y2, 0.0);
            },
            genCoordinate(), genCoordinate(), genAngle()
        );
    }
    
    /**
     * @brief Generate a line within a bounding box
     */
    static rc::Gen<DL_LineData> inBoundingBox(double minX, double minY, double maxX, double maxY) {
        return rc::gen::apply(
            [](double x1, double y1, double x2, double y2) {
                return DL_LineData(x1, y1, 0.0, x2, y2, 0.0);
            },
            rc::gen::inRange(minX, maxX),
            rc::gen::inRange(minY, maxY),
            rc::gen::inRange(minX, maxX),
            rc::gen::inRange(minY, maxY)
        );
    }
    
    /**
     * @brief Generate orthogonal lines (horizontal or vertical)
     */
    static rc::Gen<DL_LineData> orthogonal() {
        return rc::gen::oneOf(horizontal(), vertical());
    }
    
    /**
     * @brief Generate diagonal lines (45, 135, 225, 315 degrees)
     */
    static rc::Gen<DL_LineData> diagonal() {
        return rc::gen::apply(
            [](double x, double y, double length, int direction) {
                double angle = direction * 45.0 * M_PI / 180.0;
                double x2 = x + length * cos(angle);
                double y2 = y + length * sin(angle);
                return DL_LineData(x, y, 0.0, x2, y2, 0.0);
            },
            genCoordinate(), genCoordinate(),
            rc::gen::inRange(10.0, 1000.0),
            rc::gen::element<int>({1, 3, 5, 7}) // 45, 135, 225, 315 degrees
        );
    }
    
    /**
     * @brief Generate zero-length lines (degenerate case)
     */
    static rc::Gen<DL_LineData> degenerate() {
        return rc::gen::apply(
            [](double x, double y, double z) {
                return DL_LineData(x, y, z, x, y, z);
            },
            genCoordinate(), genCoordinate(), genCoordinate()
        );
    }
    
    /**
     * @brief Generate lines with specific properties for testing
     */
    static rc::Gen<DL_LineData> withProperties() {
        return rc::gen::oneOf(
            arbitrary2D(),
            horizontal(),
            vertical(),
            orthogonal(),
            diagonal(),
            rc::gen::suchThat(arbitrary2D(), [](const DL_LineData& line) {
                // Filter out degenerate lines
                double dx = line.x2 - line.x1;
                double dy = line.y2 - line.y1;
                return (dx * dx + dy * dy) > 0.01; // Minimum length
            })
        );
    }
};

} // namespace generators
} // namespace jwwlib_pbt

#endif // PBT_LINE_GENERATOR_H