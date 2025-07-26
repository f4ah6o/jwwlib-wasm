#ifndef PBT_CIRCLE_GENERATOR_H
#define PBT_CIRCLE_GENERATOR_H

#include <rapidcheck.h>
#include "dl_entities.h"
#include "../include/jww_test_entities.h"

namespace jwwlib_pbt {
namespace generators {

/**
 * @brief Generator for JWW Circles
 */
class CircleGenerator {
private:
    static constexpr double MIN_COORD = -10000.0;
    static constexpr double MAX_COORD = 10000.0;
    static constexpr double MIN_RADIUS = 0.1;
    static constexpr double MAX_RADIUS = 5000.0;
    
    static rc::Gen<double> genCoordinate() {
        return rc::gen::inRange(MIN_COORD, MAX_COORD);
    }
    
    static rc::Gen<double> genRadius() {
        return rc::gen::inRange(MIN_RADIUS, MAX_RADIUS);
    }
    
public:
    /**
     * @brief Generate a JWW circle
     */
    static rc::Gen<JWWCircle> genCircle() {
        return rc::gen::apply(
            [](double x, double y, double radius, int layer, int color, int lineType) {
                JWWCircle circle;
                circle.center.x = x;
                circle.center.y = y;
                circle.radius = radius;
                circle.layerIndex = layer;
                circle.color = color;
                circle.lineType = lineType;
                return circle;
            },
            genCoordinate(), genCoordinate(), genRadius(),
            rc::gen::inRange(0, 15),  // layer index
            rc::gen::inRange(0, 255), // color
            rc::gen::inRange(0, 7)    // line type
        );
    }
    
    /**
     * @brief Generate a DL_CircleData (for compatibility)
     */
    static rc::Gen<DL_CircleData> arbitrary() {
        return rc::gen::apply(
            [](double cx, double cy, double radius) {
                return DL_CircleData(cx, cy, 0.0, radius);
            },
            genCoordinate(), genCoordinate(), genRadius()
        );
    }
    
    /**
     * @brief Generate a 2D circle (z=0)
     */
    static rc::Gen<DL_CircleData> arbitrary2D() {
        return rc::gen::apply(
            [](double cx, double cy, double radius) {
                return DL_CircleData(cx, cy, 0.0, radius);
            },
            genCoordinate(), genCoordinate(), genRadius()
        );
    }
    
    /**
     * @brief Generate a circle at origin
     */
    static rc::Gen<DL_CircleData> atOrigin() {
        return rc::gen::apply(
            [](double radius) {
                return DL_CircleData(0.0, 0.0, 0.0, radius);
            },
            genRadius()
        );
    }
    
    /**
     * @brief Generate a circle with specific radius
     */
    static rc::Gen<DL_CircleData> withRadius(double radius) {
        return rc::gen::apply(
            [radius](double cx, double cy) {
                return DL_CircleData(cx, cy, 0.0, radius);
            },
            genCoordinate(), genCoordinate()
        );
    }
    
    /**
     * @brief Generate a circle within a bounding box
     */
    static rc::Gen<DL_CircleData> inBoundingBox(double minX, double minY, double maxX, double maxY) {
        double maxRadius = std::min({
            maxX - minX,
            maxY - minY,
            Constraints::MAX_RADIUS
        }) / 2.0;
        
        return rc::gen::apply(
            [minX, minY, maxX, maxY](double cx, double cy, double radius) {
                // Ensure circle fits within bounding box
                double clampedCx = std::max(minX + radius, std::min(maxX - radius, cx));
                double clampedCy = std::max(minY + radius, std::min(maxY - radius, cy));
                return DL_CircleData(clampedCx, clampedCy, 0.0, radius);
            },
            rc::gen::inRange(minX, maxX),
            rc::gen::inRange(minY, maxY),
            rc::gen::inRange(MIN_RADIUS, maxRadius)
        );
    }
    
    /**
     * @brief Generate small circles (for detail work)
     */
    static rc::Gen<DL_CircleData> small() {
        return rc::gen::apply(
            [](double cx, double cy, double radius) {
                return DL_CircleData(cx, cy, 0.0, radius);
            },
            genCoordinate(), genCoordinate(),
            rc::gen::inRange(0.1, 10.0)
        );
    }
    
    /**
     * @brief Generate large circles
     */
    static rc::Gen<DL_CircleData> large() {
        return rc::gen::apply(
            [](double cx, double cy, double radius) {
                return DL_CircleData(cx, cy, 0.0, radius);
            },
            genCoordinate(), genCoordinate(),
            rc::gen::inRange(1000.0, MAX_RADIUS)
        );
    }
    
    /**
     * @brief Generate circles at grid points
     */
    static rc::Gen<DL_CircleData> onGrid(double gridSize = 10.0) {
        return rc::gen::apply(
            [gridSize](int gridX, int gridY, double radius) {
                return DL_CircleData(gridX * gridSize, gridY * gridSize, 0.0, radius);
            },
            rc::gen::inRange(-100, 100),
            rc::gen::inRange(-100, 100),
            genRadius()
        );
    }
    
    /**
     * @brief Generate concentric circles (same center, different radii)
     */
    static rc::Gen<std::vector<DL_CircleData>> concentric(size_t count = 3) {
        return rc::gen::apply(
            [count](double cx, double cy, double baseRadius) {
                std::vector<DL_CircleData> circles;
                for (size_t i = 0; i < count; ++i) {
                    double radius = baseRadius * (i + 1);
                    circles.push_back(DL_CircleData(cx, cy, 0.0, radius));
                }
                return circles;
            },
            genCoordinate(), genCoordinate(),
            rc::gen::inRange(1.0, 100.0)
        );
    }
    
    /**
     * @brief Generate circles with common architectural radii
     */
    static rc::Gen<DL_CircleData> architectural() {
        static const std::vector<double> commonRadii = {
            5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 40.0, 50.0,
            75.0, 100.0, 150.0, 200.0, 250.0, 300.0, 400.0, 500.0,
            750.0, 1000.0, 1500.0, 2000.0, 2500.0, 3000.0
        };
        
        return rc::gen::apply(
            [](double cx, double cy, double radius) {
                return DL_CircleData(cx, cy, 0.0, radius);
            },
            genCoordinate(), genCoordinate(),
            rc::gen::element(commonRadii)
        );
    }
    
    /**
     * @brief Generate circles with specific properties for testing
     */
    static rc::Gen<DL_CircleData> withProperties() {
        return rc::gen::oneOf(
            arbitrary2D(),
            small(),
            architectural(),
            rc::gen::weightedOneOf<DL_CircleData>({
                {5, arbitrary2D()},
                {2, atOrigin()},
                {1, large()},
                {3, architectural()}
            })
        );
    }
};

} // namespace generators
} // namespace jwwlib_pbt

#endif // PBT_CIRCLE_GENERATOR_H