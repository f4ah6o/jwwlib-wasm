// WASM bindings for jwwlib
// This file will contain Embind bindings for JavaScript interface

#ifdef EMSCRIPTEN
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

#include "dl_jww.h"
#include "dl_creationinterface.h"
#include <vector>
#include <memory>

// Simple entity data structures for JavaScript
struct JSLineData {
    double x1, y1, x2, y2;
};

struct JSCircleData {
    double cx, cy, radius;
};

struct JSArcData {
    double cx, cy, radius;
    double angle1, angle2;
};

// JavaScript-friendly creation interface
class JSCreationInterface : public DL_CreationInterface {
private:
    std::vector<JSLineData> lines;
    std::vector<JSCircleData> circles;
    std::vector<JSArcData> arcs;
    
public:
    // Getters for JavaScript
    const std::vector<JSLineData>& getLines() const { return lines; }
    const std::vector<JSCircleData>& getCircles() const { return circles; }
    const std::vector<JSArcData>& getArcs() const { return arcs; }
    
    // Clear all data
    void clear() {
        lines.clear();
        circles.clear();
        arcs.clear();
    }
    
    // DL_CreationInterface implementation
    virtual void addLayer(const DL_LayerData& /*data*/) override {}
    virtual void addBlock(const DL_BlockData& /*data*/) override {}
    virtual void endBlock() override {}
    virtual void addPoint(const DL_PointData& /*data*/) override {}
    
    virtual void addLine(const DL_LineData& data) override {
        lines.push_back({data.x1, data.y1, data.x2, data.y2});
    }
    
    virtual void addArc(const DL_ArcData& data) override {
        arcs.push_back({data.cx, data.cy, data.radius, data.angle1, data.angle2});
    }
    
    virtual void addCircle(const DL_CircleData& data) override {
        circles.push_back({data.cx, data.cy, data.radius});
    }
    
    virtual void addEllipse(const DL_EllipseData& /*data*/) override {}
    virtual void addPolyline(const DL_PolylineData& /*data*/) override {}
    virtual void addVertex(const DL_VertexData& /*data*/) override {}
    virtual void addSpline(const DL_SplineData& /*data*/) override {}
    virtual void addControlPoint(const DL_ControlPointData& /*data*/) override {}
    virtual void addKnot(const DL_KnotData& /*data*/) override {}
    virtual void addInsert(const DL_InsertData& /*data*/) override {}
    virtual void addTrace(const DL_TraceData& /*data*/) override {}
    virtual void add3dFace(const DL_3dFaceData& /*data*/) override {}
    virtual void addSolid(const DL_SolidData& /*data*/) override {}
    virtual void addMText(const DL_MTextData& /*data*/) override {}
    virtual void addMTextChunk(const char* /*text*/) override {}
    virtual void addText(const DL_TextData& /*data*/) override {}
    virtual void addDimAlign(const DL_DimensionData& /*data*/, const DL_DimAlignedData& /*edata*/) override {}
    virtual void addDimLinear(const DL_DimensionData& /*data*/, const DL_DimLinearData& /*edata*/) override {}
    virtual void addDimRadial(const DL_DimensionData& /*data*/, const DL_DimRadialData& /*edata*/) override {}
    virtual void addDimDiametric(const DL_DimensionData& /*data*/, const DL_DimDiametricData& /*edata*/) override {}
    virtual void addDimAngular(const DL_DimensionData& /*data*/, const DL_DimAngularData& /*edata*/) override {}
    virtual void addDimAngular3P(const DL_DimensionData& /*data*/, const DL_DimAngular3PData& /*edata*/) override {}
    virtual void addDimOrdinate(const DL_DimensionData& /*data*/, const DL_DimOrdinateData& /*edata*/) override {}
    virtual void addLeader(const DL_LeaderData& /*data*/) override {}
    virtual void addLeaderVertex(const DL_LeaderVertexData& /*data*/) override {}
    virtual void addHatch(const DL_HatchData& /*data*/) override {}
    virtual void addImage(const DL_ImageData& /*data*/) override {}
    virtual void linkImage(const DL_ImageDefData& /*data*/) override {}
    virtual void addHatchLoop(const DL_HatchLoopData& /*data*/) override {}
    virtual void addHatchEdge(const DL_HatchEdgeData& /*data*/) override {}
    virtual void setVariableString(const char* /*key*/, const char* /*value*/, int /*code*/) override {}
    virtual void setVariableInt(const char* /*key*/, int /*value*/, int /*code*/) override {}
    virtual void setVariableDouble(const char* /*key*/, double /*value*/, int /*code*/) override {}
    virtual void setVariableVector(const char* /*key*/, double /*v1*/, double /*v2*/, double /*v3*/, int /*code*/) override {}
    virtual void addComment(const char* /*comment*/) override {}
    virtual void endSequence() override {}
    virtual void endEntity() override {}
};

// JWW Reader wrapper class
class JWWReader {
private:
    std::unique_ptr<DL_Jww> jww;
    std::unique_ptr<JSCreationInterface> creationInterface;
    
public:
    JWWReader() : jww(std::make_unique<DL_Jww>()), 
                  creationInterface(std::make_unique<JSCreationInterface>()) {}
    
    bool readFile(const std::string& filename) {
        creationInterface->clear();
        return jww->in(filename, creationInterface.get());
    }
    
    bool readData(const std::vector<uint8_t>& data) {
        // TODO: Implement reading from memory buffer
        // For now, we'll need to write to a temporary file in MEMFS
        return false;
    }
    
    const std::vector<JSLineData>& getLines() const { 
        return creationInterface->getLines(); 
    }
    
    const std::vector<JSCircleData>& getCircles() const { 
        return creationInterface->getCircles(); 
    }
    
    const std::vector<JSArcData>& getArcs() const { 
        return creationInterface->getArcs(); 
    }
};

#ifdef EMSCRIPTEN
// Embind bindings
using namespace emscripten;

EMSCRIPTEN_BINDINGS(jwwlib_module) {
    // Data structures
    value_object<JSLineData>("LineData")
        .field("x1", &JSLineData::x1)
        .field("y1", &JSLineData::y1)
        .field("x2", &JSLineData::x2)
        .field("y2", &JSLineData::y2);
    
    value_object<JSCircleData>("CircleData")
        .field("cx", &JSCircleData::cx)
        .field("cy", &JSCircleData::cy)
        .field("radius", &JSCircleData::radius);
    
    value_object<JSArcData>("ArcData")
        .field("cx", &JSArcData::cx)
        .field("cy", &JSArcData::cy)
        .field("radius", &JSArcData::radius)
        .field("angle1", &JSArcData::angle1)
        .field("angle2", &JSArcData::angle2);
    
    // Vectors
    register_vector<JSLineData>("LineDataVector");
    register_vector<JSCircleData>("CircleDataVector");
    register_vector<JSArcData>("ArcDataVector");
    register_vector<uint8_t>("Uint8Vector");
    
    // JWWReader class
    class_<JWWReader>("JWWReader")
        .constructor<>()
        .function("readFile", &JWWReader::readFile)
        .function("readData", &JWWReader::readData)
        .function("getLines", &JWWReader::getLines)
        .function("getCircles", &JWWReader::getCircles)
        .function("getArcs", &JWWReader::getArcs);
}
#endif // EMSCRIPTEN