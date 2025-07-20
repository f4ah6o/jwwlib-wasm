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
#include <cmath>

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

struct JSTextData {
    double x, y;             // Text position
    double height;           // Text height
    double angle;            // Rotation angle in radians
    std::string text;        // Text content
};

struct JSEllipseData {
    double cx, cy;           // Center position
    double majorAxis;        // Major axis length
    double ratio;            // Ratio of minor to major axis
    double angle;            // Rotation angle in radians
};

// Unified entity structure
struct JSEntity {
    std::string type;
    double x1, y1, x2, y2;      // For lines
    double cx, cy, radius;       // For circles and arcs
    double angle1, angle2;       // For arcs
    double x, y;                 // For text position
    double height;               // For text height
    double angle;                // For text rotation and ellipse rotation
    std::string text;            // For text content
    double majorAxis;            // For ellipse major axis
    double ratio;                // For ellipse axis ratio
};

// Header information
struct JSHeader {
    std::string version;
    int entityCount;
};

// JavaScript-friendly creation interface
class JSCreationInterface : public DL_CreationInterface {
private:
    std::vector<JSLineData> lines;
    std::vector<JSCircleData> circles;
    std::vector<JSArcData> arcs;
    std::vector<JSTextData> texts;
    std::vector<JSEllipseData> ellipses;
    
public:
    // Getters for JavaScript
    const std::vector<JSLineData>& getLines() const { return lines; }
    const std::vector<JSCircleData>& getCircles() const { return circles; }
    const std::vector<JSArcData>& getArcs() const { return arcs; }
    const std::vector<JSTextData>& getTexts() const { return texts; }
    const std::vector<JSEllipseData>& getEllipses() const { return ellipses; }
    
    // Clear all data
    void clear() {
        lines.clear();
        circles.clear();
        arcs.clear();
        texts.clear();
        ellipses.clear();
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
    
    virtual void addEllipse(const DL_EllipseData& data) override {
        // Calculate major axis length and angle from the major axis endpoint
        double dx = data.mx - data.cx;
        double dy = data.my - data.cy;
        double majorAxis = sqrt(dx * dx + dy * dy);
        double angle = atan2(dy, dx);
        ellipses.push_back({data.cx, data.cy, majorAxis, data.ratio, angle});
    }
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
    virtual void addText(const DL_TextData& data) override {
        texts.push_back({data.ipx, data.ipy, data.height, data.angle * M_PI / 180.0, data.text});
    }
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
    JWWReader() : creationInterface(std::make_unique<JSCreationInterface>()) {}
    
    // Constructor with data pointer and size
    JWWReader(uintptr_t dataPtr, size_t size) : creationInterface(std::make_unique<JSCreationInterface>()) {
        readFile(dataPtr, size);
    }
    
    bool readFile(uintptr_t dataPtr, size_t size) {
        creationInterface->clear();
        
        // Read data from memory
        const uint8_t* data = reinterpret_cast<const uint8_t*>(dataPtr);
        std::vector<uint8_t> buffer(data, data + size);
        
        // Write buffer to temporary file in MEMFS
        std::string tempFile = "/tmp/jww_temp.jww";
        FILE* fp = fopen(tempFile.c_str(), "wb");
        if (!fp) {
            return false;
        }
        fwrite(data, 1, size, fp);
        fclose(fp);
        
        // Use DL_Jww to read the file
        jww = std::make_unique<DL_Jww>();
        bool result = jww->in(tempFile, creationInterface.get());
        
        // Clean up temporary file
        remove(tempFile.c_str());
        
        return result;
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
    
    const std::vector<JSTextData>& getTexts() const { 
        return creationInterface->getTexts(); 
    }
    
    const std::vector<JSEllipseData>& getEllipses() const { 
        return creationInterface->getEllipses(); 
    }
    
    // Get all entities in a unified format
    std::vector<JSEntity> getEntities() const {
        std::vector<JSEntity> entities;
        
        // Add lines
        for (const auto& line : creationInterface->getLines()) {
            JSEntity entity;
            entity.type = "LINE";
            entity.x1 = line.x1;
            entity.y1 = line.y1;
            entity.x2 = line.x2;
            entity.y2 = line.y2;
            entity.cx = entity.cy = entity.radius = 0.0;
            entity.angle1 = entity.angle2 = 0.0;
            entity.x = entity.y = entity.height = entity.angle = 0.0;
            entity.majorAxis = entity.ratio = 0.0;
            entities.push_back(entity);
        }
        
        // Add circles
        for (const auto& circle : creationInterface->getCircles()) {
            JSEntity entity;
            entity.type = "CIRCLE";
            entity.cx = circle.cx;
            entity.cy = circle.cy;
            entity.radius = circle.radius;
            entity.x1 = entity.y1 = entity.x2 = entity.y2 = 0.0;
            entity.angle1 = entity.angle2 = 0.0;
            entity.x = entity.y = entity.height = entity.angle = 0.0;
            entity.majorAxis = entity.ratio = 0.0;
            entities.push_back(entity);
        }
        
        // Add arcs
        for (const auto& arc : creationInterface->getArcs()) {
            JSEntity entity;
            entity.type = "ARC";
            entity.cx = arc.cx;
            entity.cy = arc.cy;
            entity.radius = arc.radius;
            entity.angle1 = arc.angle1;
            entity.angle2 = arc.angle2;
            entity.x1 = entity.y1 = entity.x2 = entity.y2 = 0.0;
            entity.x = entity.y = entity.height = 0.0;
            entity.angle = 0.0;
            entities.push_back(entity);
        }
        
        // Add texts
        for (const auto& textData : creationInterface->getTexts()) {
            JSEntity entity;
            entity.type = "TEXT";
            entity.x = textData.x;
            entity.y = textData.y;
            entity.height = textData.height;
            entity.angle = textData.angle;
            entity.text = textData.text;
            entity.x1 = entity.y1 = entity.x2 = entity.y2 = 0.0;
            entity.cx = entity.cy = entity.radius = 0.0;
            entity.angle1 = entity.angle2 = 0.0;
            entity.majorAxis = entity.ratio = 0.0;
            entities.push_back(entity);
        }
        
        // Add ellipses
        for (const auto& ellipse : creationInterface->getEllipses()) {
            JSEntity entity;
            entity.type = "ELLIPSE";
            entity.cx = ellipse.cx;
            entity.cy = ellipse.cy;
            entity.majorAxis = ellipse.majorAxis;
            entity.ratio = ellipse.ratio;
            entity.angle = ellipse.angle;
            entity.x1 = entity.y1 = entity.x2 = entity.y2 = 0.0;
            entity.x = entity.y = entity.height = 0.0;
            entity.radius = entity.angle1 = entity.angle2 = 0.0;
            entities.push_back(entity);
        }
        
        return entities;
    }
    
    // Get header information
    JSHeader getHeader() const {
        JSHeader header;
        header.version = "JWW";
        header.entityCount = creationInterface->getLines().size() + 
                           creationInterface->getCircles().size() + 
                           creationInterface->getArcs().size() +
                           creationInterface->getTexts().size() +
                           creationInterface->getEllipses().size();
        return header;
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
    
    value_object<JSTextData>("TextData")
        .field("x", &JSTextData::x)
        .field("y", &JSTextData::y)
        .field("height", &JSTextData::height)
        .field("angle", &JSTextData::angle)
        .field("text", &JSTextData::text);
    
    value_object<JSEllipseData>("EllipseData")
        .field("cx", &JSEllipseData::cx)
        .field("cy", &JSEllipseData::cy)
        .field("majorAxis", &JSEllipseData::majorAxis)
        .field("ratio", &JSEllipseData::ratio)
        .field("angle", &JSEllipseData::angle);
    
    // Unified entity structure
    value_object<JSEntity>("Entity")
        .field("type", &JSEntity::type)
        .field("x1", &JSEntity::x1)
        .field("y1", &JSEntity::y1)
        .field("x2", &JSEntity::x2)
        .field("y2", &JSEntity::y2)
        .field("cx", &JSEntity::cx)
        .field("cy", &JSEntity::cy)
        .field("radius", &JSEntity::radius)
        .field("angle1", &JSEntity::angle1)
        .field("angle2", &JSEntity::angle2)
        .field("x", &JSEntity::x)
        .field("y", &JSEntity::y)
        .field("height", &JSEntity::height)
        .field("angle", &JSEntity::angle)
        .field("text", &JSEntity::text)
        .field("majorAxis", &JSEntity::majorAxis)
        .field("ratio", &JSEntity::ratio);
    
    // Header structure
    value_object<JSHeader>("Header")
        .field("version", &JSHeader::version)
        .field("entityCount", &JSHeader::entityCount);
    
    // Vectors
    register_vector<JSLineData>("LineDataVector");
    register_vector<JSCircleData>("CircleDataVector");
    register_vector<JSArcData>("ArcDataVector");
    register_vector<JSTextData>("TextDataVector");
    register_vector<JSEllipseData>("EllipseDataVector");
    register_vector<JSEntity>("EntityVector");
    register_vector<uint8_t>("Uint8Vector");
    
    // JWWReader class
    class_<JWWReader>("JWWReader")
        .constructor<>()
        .constructor<uintptr_t, size_t>()
        .function("readFile", &JWWReader::readFile)
        .function("getLines", &JWWReader::getLines)
        .function("getCircles", &JWWReader::getCircles)
        .function("getArcs", &JWWReader::getArcs)
        .function("getTexts", &JWWReader::getTexts)
        .function("getEllipses", &JWWReader::getEllipses)
        .function("getEntities", &JWWReader::getEntities)
        .function("getHeader", &JWWReader::getHeader);
}
#endif // EMSCRIPTEN