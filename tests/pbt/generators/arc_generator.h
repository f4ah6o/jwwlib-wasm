#ifndef PBT_ARC_GENERATOR_H
#define PBT_ARC_GENERATOR_H

#include <rapidcheck.h>
#include <algorithm>
#include <cmath>
#include "dl_entities.h"
#include "../include/jww_test_entities.h"

namespace jwwlib_pbt {
namespace generators {

/**
 * @brief Generator for JWW Arcs
 */
class ArcGenerator {
private:
    static constexpr double MIN_COORD = -10000.0;
    static constexpr double MAX_COORD = 10000.0;
    static constexpr double MIN_RADIUS = 0.1;
    static constexpr double MAX_RADIUS = 5000.0;
    static constexpr double MIN_ANGLE = 0.0;
    static constexpr double MAX_ANGLE = 360.0;
    
    static rc::Gen<double> genCoordinate() {
        return rc::gen::inRange(MIN_COORD, MAX_COORD);
    }
    
    static rc::Gen<double> genRadius() {
        return rc::gen::inRange(MIN_RADIUS, MAX_RADIUS);
    }
    
    static rc::Gen<double> genAngle() {
        return rc::gen::inRange(MIN_ANGLE, MAX_ANGLE);
    }
    
public:
    /**
     * @brief Generate a JWW arc
     */
    static rc::Gen<JWWArc> genArc() {
        return rc::gen::apply(
            [](double x, double y, double radius, double startAngle, double endAngle, 
               int layer, int color, int lineType) {
                JWWArc arc;
                arc.center.x = x;
                arc.center.y = y;
                arc.radius = radius;
                arc.startAngle = startAngle;
                arc.endAngle = endAngle;
                arc.layerIndex = layer;
                arc.color = color;
                arc.lineType = lineType;
                return arc;
            },
            genCoordinate(), genCoordinate(), genRadius(),
            genAngle(), genAngle(),
            rc::gen::inRange(0, 15),  // layer index
            rc::gen::inRange(0, 255), // color
            rc::gen::inRange(0, 7)    // line type
        );
    }
    
    /**
     * @brief Generate a DL_ArcData (for compatibility)
     */
    static rc::Gen<DL_ArcData> arbitrary() {
        return rc::gen::apply(
            [](double cx, double cy, double radius, double angle1, double angle2) {
                if (angle1 > angle2) {
                    std::swap(angle1, angle2);
                }
                return DL_ArcData(cx, cy, 0.0, radius, angle1, angle2);
            },
            genCoordinate(), genCoordinate(), genRadius(),
            genAngle(), genAngle()
        );
    }
    
    /**
     * @brief Generate a 2D arc (z=0)
     */
    static rc::Gen<DL_ArcData> arbitrary2D() {
        return rc::gen::apply(
            [](double cx, double cy, double radius, double angle1, double angle2) {
                if (angle1 > angle2) {
                    std::swap(angle1, angle2);
                }
                return DL_ArcData(cx, cy, 0.0, radius, angle1, angle2);
            },
            genCoordinate(), genCoordinate(), genRadius(),
            genAngle(), genAngle()
        );
    }
    
    /**
     * @brief Generate a quarter arc (90 degrees)
     */
    static rc::Gen<DL_ArcData> quarterArc() {
        return rc::gen::apply(
            [](double cx, double cy, double radius, int quadrant) {
                double startAngle = quadrant * 90.0;
                double endAngle = startAngle + 90.0;
                return DL_ArcData(cx, cy, 0.0, radius, startAngle, endAngle);
            },
            genCoordinate(), genCoordinate(), genRadius(),
            rc::gen::inRange(0, 4)
        );
    }
    
    /**
     * @brief Generate a semi-circle (180 degrees)
     */
    static rc::Gen<DL_ArcData> semiCircle() {
        return rc::gen::apply(
            [](double cx, double cy, double radius, double startAngle) {
                double endAngle = std::fmod(startAngle + 180.0, 360.0);
                return DL_ArcData(cx, cy, 0.0, radius, startAngle, endAngle);
            },
            genCoordinate(), genCoordinate(), genRadius(),
            genAngle()
        );
    }
    
    /**
     * @brief Generate an arc at origin
     */
    static rc::Gen<DL_ArcData> atOrigin() {
        return rc::gen::apply(
            [](double radius, double angle1, double angle2) {
                if (angle1 > angle2) {
                    std::swap(angle1, angle2);
                }
                return DL_ArcData(0.0, 0.0, 0.0, radius, angle1, angle2);
            },
            genRadius(), genAngle(), genAngle()
        );
    }
    
    /**
     * @brief Generate an arc with specific angular span
     */
    static rc::Gen<DL_ArcData> withAngularSpan(double spanDegrees) {
        return rc::gen::apply(
            [spanDegrees](double cx, double cy, double radius, double startAngle) {
                double endAngle = std::fmod(startAngle + spanDegrees, 360.0);
                return DL_ArcData(cx, cy, 0.0, radius, startAngle, endAngle);
            },
            genCoordinate(), genCoordinate(), genRadius(),
            genAngle()
        );
    }
    
    /**
     * @brief Generate arcs within a bounding box
     */
    static rc::Gen<DL_ArcData> inBoundingBox(double minX, double minY, double maxX, double maxY) {
        double maxRadius = std::min({
            maxX - minX,
            maxY - minY,
            MAX_RADIUS
        }) / 2.0;
        
        return rc::gen::apply(
            [minX, minY, maxX, maxY](double cx, double cy, double radius, 
                                     double angle1, double angle2) {
                // Ensure arc fits within bounding box
                double clampedCx = std::max(minX + radius, std::min(maxX - radius, cx));
                double clampedCy = std::max(minY + radius, std::min(maxY - radius, cy));
                if (angle1 > angle2) {
                    std::swap(angle1, angle2);
                }
                return DL_ArcData(clampedCx, clampedCy, 0.0, radius, angle1, angle2);
            },
            rc::gen::inRange(minX, maxX),
            rc::gen::inRange(minY, maxY),
            rc::gen::inRange(MIN_RADIUS, maxRadius),
            genAngle(), genAngle()
        );
    }
    
    /**
     * @brief Generate small arcs (for detail work)
     */
    static rc::Gen<DL_ArcData> small() {
        return rc::gen::apply(
            [](double cx, double cy, double radius, double angle1, double angle2) {
                if (angle1 > angle2) {
                    std::swap(angle1, angle2);
                }
                return DL_ArcData(cx, cy, 0.0, radius, angle1, angle2);
            },
            genCoordinate(), genCoordinate(),
            rc::gen::inRange(0.1, 10.0),
            genAngle(), genAngle()
        );
    }
    
    /**
     * @brief Generate architectural arcs (common angles)
     */
    static rc::Gen<DL_ArcData> architectural() {
        static const std::vector<std::pair<double, double>> commonArcs = {
            {0.0, 90.0},    // Quarter arc
            {90.0, 180.0},  // Quarter arc
            {180.0, 270.0}, // Quarter arc
            {270.0, 360.0}, // Quarter arc
            {0.0, 180.0},   // Semi-circle
            {180.0, 360.0}, // Semi-circle
            {0.0, 45.0},    // Eighth arc
            {0.0, 30.0},    // Twelfth arc
            {0.0, 60.0},    // Sixth arc
            {0.0, 120.0},   // Third arc
        };
        
        return rc::gen::apply(
            [](double cx, double cy, double radius, const std::pair<double, double>& angles) {
                return DL_ArcData(cx, cy, 0.0, radius, angles.first, angles.second);
            },
            genCoordinate(), genCoordinate(),
            rc::gen::oneOf(
                rc::gen::element<double>({50.0, 100.0, 150.0, 200.0, 300.0, 500.0}),
                genRadius()
            ),
            rc::gen::element(commonArcs)
        );
    }
    
    /**
     * @brief Generate full circles represented as arcs (0-360)
     */
    static rc::Gen<DL_ArcData> fullCircle() {
        return rc::gen::apply(
            [](double cx, double cy, double radius) {
                return DL_ArcData(cx, cy, 0.0, radius, 0.0, 360.0);
            },
            genCoordinate(), genCoordinate(), genRadius()
        );
    }
    
    /**
     * @brief Generate tiny arcs (almost points)
     */
    static rc::Gen<DL_ArcData> tiny() {
        return rc::gen::apply(
            [](double cx, double cy, double radius, double startAngle) {
                double endAngle = startAngle + *rc::gen::inRange(0.1, 1.0);
                return DL_ArcData(cx, cy, 0.0, radius, startAngle, endAngle);
            },
            genCoordinate(), genCoordinate(), genRadius(),
            genAngle()
        );
    }
    
    /**
     * @brief Generate arcs with properties for testing
     */
    static rc::Gen<DL_ArcData> withProperties() {
        return rc::gen::oneOf(
            arbitrary2D(),
            quarterArc(),
            semiCircle(),
            architectural(),
            rc::gen::weightedOneOf<DL_ArcData>({
                {5, arbitrary2D()},
                {3, quarterArc()},
                {2, semiCircle()},
                {3, architectural()},
                {1, fullCircle()},
                {1, tiny()}
            })
        );
    }
    
    /**
     * @brief Generate connected arcs (end of one arc connects to start of next)
     */
    static rc::Gen<std::vector<DL_ArcData>> connectedArcs(size_t count = 3) {
        return rc::gen::apply(
            [count](double startCx, double startCy, double baseRadius) {
                std::vector<DL_ArcData> arcs;
                double currentAngle = 0.0;
                double cx = startCx;
                double cy = startCy;
                
                for (size_t i = 0; i < count; ++i) {
                    double arcSpan = 360.0 / count;
                    double startAngle = currentAngle;
                    double endAngle = startAngle + arcSpan;
                    
                    arcs.push_back(DL_ArcData(cx, cy, 0.0, baseRadius, startAngle, endAngle));
                    
                    // Calculate next center position
                    double endRad = endAngle * M_PI / 180.0;
                    cx += baseRadius * cos(endRad) * 0.5;
                    cy += baseRadius * sin(endRad) * 0.5;
                    currentAngle = endAngle;
                }
                
                return arcs;
            },
            genCoordinate(), genCoordinate(),
            rc::gen::inRange(10.0, 100.0)
        );
    }
};

} // namespace generators
} // namespace jwwlib_pbt

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif // PBT_ARC_GENERATOR_H