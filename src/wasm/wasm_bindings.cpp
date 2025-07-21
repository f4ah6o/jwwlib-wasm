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
    int color;
};

struct JSCircleData {
    double cx, cy, radius;
    int color;
};

struct JSArcData {
    double cx, cy, radius;
    double angle1, angle2;
    int color;
};

struct JSTextData {
    double x, y;             // Text position
    double height;           // Text height
    double angle;            // Rotation angle in radians
    std::string text;        // Text content (UTF-8)
    std::vector<uint8_t> textBytes;  // Original text bytes (Shift-JIS)
    int color;
};

struct JSEllipseData {
    double cx, cy;           // Center position
    double majorAxis;        // Major axis length
    double ratio;            // Ratio of minor to major axis
    double angle;            // Rotation angle in radians
    int color;
};

struct JSPointData {
    double x, y, z;          // Point position
    int color;
};

struct JSVertexData {
    double x, y, z;          // Vertex position
    double bulge;            // Bulge for arc segments (0 for lines)
};

struct JSPolylineData {
    std::vector<JSVertexData> vertices;  // Vertex list
    bool closed;                         // Is polyline closed
    int color;
};

struct JSSolidData {
    double x[4], y[4], z[4];             // Four vertices (3 or 4 used)
    int color;
    
    // Getter methods for Embind
    double getX(int index) const { return (index >= 0 && index < 4) ? x[index] : 0.0; }
    double getY(int index) const { return (index >= 0 && index < 4) ? y[index] : 0.0; }
    double getZ(int index) const { return (index >= 0 && index < 4) ? z[index] : 0.0; }
};

struct JSMTextData {
    double x, y, z;              // 挿入点
    double height;               // テキスト高さ
    double width;                // テキストボックス幅
    int attachmentPoint;         // アタッチメントポイント（1-9）
    int drawingDirection;        // 描画方向
    int lineSpacingStyle;        // 行間スタイル
    double lineSpacingFactor;    // 行間係数
    std::string text;            // テキスト内容 (UTF-8)
    std::vector<uint8_t> textBytes;  // Original text bytes (Shift-JIS)
    std::string style;           // スタイル名
    double angle;                // 回転角度
    int color;
};

struct JSDimensionData {
    // 共通フィールド
    double dpx, dpy, dpz;        // 定義点
    double mpx, mpy, mpz;        // テキスト中点
    int type;                    // 寸法タイプ (0=linear, 1=aligned, 2=radial, etc.)
    int attachmentPoint;         // テキスト配置
    std::string text;            // 寸法テキスト
    double angle;                // テキスト角度
    
    // Linear寸法用
    double dpx1, dpy1, dpz1;     // 定義点1
    double dpx2, dpy2, dpz2;     // 定義点2
    double dimLineAngle;         // 寸法線角度
    int color;
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
    double z;                    // For point z coordinate
    std::vector<JSVertexData> vertices; // For polyline vertices
    bool closed;                 // For polyline closed flag
    double solidX[4], solidY[4], solidZ[4]; // For solid vertices
    int color;                   // Entity color
    
    // Getter methods for solid vertices
    double getSolidX(int index) const { return (index >= 0 && index < 4) ? solidX[index] : 0.0; }
    double getSolidY(int index) const { return (index >= 0 && index < 4) ? solidY[index] : 0.0; }
    double getSolidZ(int index) const { return (index >= 0 && index < 4) ? solidZ[index] : 0.0; }
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
    std::vector<JSPointData> points;
    std::vector<JSPolylineData> polylines;
    JSPolylineData* currentPolyline = nullptr;
    std::vector<JSSolidData> solids;
    std::vector<JSMTextData> mtexts;
    std::vector<JSDimensionData> dimensions;
    int currentColor = 256;  // Default to BYLAYER
    
public:
    // Getters for JavaScript
    const std::vector<JSLineData>& getLines() const { return lines; }
    const std::vector<JSCircleData>& getCircles() const { return circles; }
    const std::vector<JSArcData>& getArcs() const { return arcs; }
    const std::vector<JSTextData>& getTexts() const { return texts; }
    const std::vector<JSEllipseData>& getEllipses() const { return ellipses; }
    const std::vector<JSPointData>& getPoints() const { return points; }
    const std::vector<JSPolylineData>& getPolylines() const { return polylines; }
    const std::vector<JSSolidData>& getSolids() const { return solids; }
    const std::vector<JSMTextData>& getMTexts() const { return mtexts; }
    const std::vector<JSDimensionData>& getDimensions() const { return dimensions; }
    
    // Clear all data
    void clear() {
        lines.clear();
        circles.clear();
        arcs.clear();
        texts.clear();
        ellipses.clear();
        points.clear();
        polylines.clear();
        currentPolyline = nullptr;
        solids.clear();
        mtexts.clear();
        dimensions.clear();
    }
    
    // DL_CreationInterface implementation
    void setAttributes(const DL_Attributes& attrib) {
        currentColor = attrib.getColor();
        // Call parent implementation
        DL_CreationInterface::setAttributes(attrib);
    }
    virtual void addLayer(const DL_LayerData& /*data*/) override {}
    virtual void addBlock(const DL_BlockData& /*data*/) override {}
    virtual void endBlock() override {}
    virtual void addPoint(const DL_PointData& data) override {
        points.push_back({data.x, data.y, data.z, currentColor});
    }
    
    virtual void addLine(const DL_LineData& data) override {
        lines.push_back({data.x1, data.y1, data.x2, data.y2, currentColor});
    }
    
    virtual void addArc(const DL_ArcData& data) override {
        arcs.push_back({data.cx, data.cy, data.radius, data.angle1, data.angle2, currentColor});
    }
    
    virtual void addCircle(const DL_CircleData& data) override {
        circles.push_back({data.cx, data.cy, data.radius, currentColor});
    }
    
    virtual void addEllipse(const DL_EllipseData& data) override {
        // Calculate major axis length and angle from the major axis endpoint
        double dx = data.mx - data.cx;
        double dy = data.my - data.cy;
        double majorAxis = sqrt(dx * dx + dy * dy);
        double angle = atan2(dy, dx);
        ellipses.push_back({data.cx, data.cy, majorAxis, data.ratio, angle, currentColor});
    }
    virtual void addPolyline(const DL_PolylineData& data) override {
        JSPolylineData polyline;
        polyline.closed = (data.flags & 0x01) != 0;  // Check if closed
        polyline.color = currentColor;
        polylines.push_back(polyline);
        currentPolyline = &polylines.back();
    }
    virtual void addVertex(const DL_VertexData& data) override {
        if (currentPolyline) {
            currentPolyline->vertices.push_back({data.x, data.y, data.z, data.bulge});
        }
    }
    virtual void addSpline(const DL_SplineData& /*data*/) override {}
    virtual void addControlPoint(const DL_ControlPointData& /*data*/) override {}
    virtual void addKnot(const DL_KnotData& /*data*/) override {}
    virtual void addInsert(const DL_InsertData& /*data*/) override {}
    virtual void addTrace(const DL_TraceData& /*data*/) override {}
    virtual void add3dFace(const DL_3dFaceData& /*data*/) override {}
    virtual void addSolid(const DL_SolidData& data) override {
        JSSolidData solid;
        for (int i = 0; i < 4; i++) {
            solid.x[i] = data.x[i];
            solid.y[i] = data.y[i];
            solid.z[i] = data.z[i];
        }
        solid.color = currentColor;
        solids.push_back(solid);
    }
    virtual void addMText(const DL_MTextData& data) override {
        // Store both the text string and original bytes
        std::vector<uint8_t> bytes(data.text.begin(), data.text.end());
        mtexts.push_back({
            data.ipx, data.ipy, data.ipz,
            data.height, data.width,
            data.attachmentPoint,
            data.drawingDirection,
            data.lineSpacingStyle,
            data.lineSpacingFactor,
            data.text, bytes, data.style,
            data.angle,
            currentColor
        });
    }
    virtual void addMTextChunk(const char* text) override {
        if (!mtexts.empty()) {
            mtexts.back().text += text;
            // Also append to bytes
            std::string str(text);
            mtexts.back().textBytes.insert(mtexts.back().textBytes.end(), str.begin(), str.end());
        }
    }
    virtual void addText(const DL_TextData& data) override {
        // Store both the text string and original bytes
        std::vector<uint8_t> bytes(data.text.begin(), data.text.end());
        texts.push_back({data.ipx, data.ipy, data.height, data.angle * M_PI / 180.0, data.text, bytes, currentColor});
    }
    virtual void addDimAlign(const DL_DimensionData& data, const DL_DimAlignedData& edata) override {
        dimensions.push_back({
            data.dpx, data.dpy, data.dpz,
            data.mpx, data.mpy, data.mpz,
            1, // type = aligned
            data.attachmentPoint,
            data.text,
            data.angle,
            edata.epx1, edata.epy1, edata.epz1,
            edata.epx2, edata.epy2, edata.epz2,
            0.0, // dimLineAngle not used for aligned
            currentColor
        });
    }
    virtual void addDimLinear(const DL_DimensionData& data, const DL_DimLinearData& edata) override {
        dimensions.push_back({
            data.dpx, data.dpy, data.dpz,
            data.mpx, data.mpy, data.mpz,
            0, // type = linear
            data.attachmentPoint,
            data.text,
            data.angle,
            edata.dpx1, edata.dpy1, edata.dpz1,
            edata.dpx2, edata.dpy2, edata.dpz2,
            edata.angle,
            currentColor
        });
    }
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
    virtual void endSequence() override {
        currentPolyline = nullptr;
    }
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
    
    const std::vector<JSPointData>& getPoints() const { 
        return creationInterface->getPoints(); 
    }
    
    const std::vector<JSPolylineData>& getPolylines() const { 
        return creationInterface->getPolylines(); 
    }
    
    const std::vector<JSSolidData>& getSolids() const { 
        return creationInterface->getSolids(); 
    }
    
    const std::vector<JSMTextData>& getMTexts() const { 
        return creationInterface->getMTexts(); 
    }
    
    const std::vector<JSDimensionData>& getDimensions() const { 
        return creationInterface->getDimensions(); 
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
            entity.color = line.color;
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
            entity.color = circle.color;
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
            entity.color = arc.color;
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
            entity.color = textData.color;
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
            entity.z = 0.0;
            entity.color = ellipse.color;
            entities.push_back(entity);
        }
        
        // Add points
        for (const auto& point : creationInterface->getPoints()) {
            JSEntity entity;
            entity.type = "POINT";
            entity.x = point.x;
            entity.y = point.y;
            entity.z = point.z;
            entity.x1 = entity.y1 = entity.x2 = entity.y2 = 0.0;
            entity.cx = entity.cy = entity.radius = 0.0;
            entity.angle1 = entity.angle2 = 0.0;
            entity.height = entity.angle = 0.0;
            entity.majorAxis = entity.ratio = 0.0;
            entity.color = point.color;
            entities.push_back(entity);
        }
        
        // Add solids
        for (const auto& solid : creationInterface->getSolids()) {
            JSEntity entity;
            entity.type = "SOLID";
            for (int i = 0; i < 4; i++) {
                entity.solidX[i] = solid.x[i];
                entity.solidY[i] = solid.y[i];
                entity.solidZ[i] = solid.z[i];
            }
            entity.x1 = entity.y1 = entity.x2 = entity.y2 = 0.0;
            entity.cx = entity.cy = entity.radius = 0.0;
            entity.angle1 = entity.angle2 = 0.0;
            entity.x = entity.y = entity.height = entity.angle = 0.0;
            entity.majorAxis = entity.ratio = 0.0;
            entity.z = 0.0;
            entity.closed = false;
            entity.color = solid.color;
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
                           creationInterface->getEllipses().size() +
                           creationInterface->getPoints().size() +
                           creationInterface->getPolylines().size() +
                           creationInterface->getSolids().size() +
                           creationInterface->getMTexts().size() +
                           creationInterface->getDimensions().size();
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
        .field("y2", &JSLineData::y2)
        .field("color", &JSLineData::color);
    
    value_object<JSCircleData>("CircleData")
        .field("cx", &JSCircleData::cx)
        .field("cy", &JSCircleData::cy)
        .field("radius", &JSCircleData::radius)
        .field("color", &JSCircleData::color);
    
    value_object<JSArcData>("ArcData")
        .field("cx", &JSArcData::cx)
        .field("cy", &JSArcData::cy)
        .field("radius", &JSArcData::radius)
        .field("angle1", &JSArcData::angle1)
        .field("angle2", &JSArcData::angle2)
        .field("color", &JSArcData::color);
    
    value_object<JSTextData>("TextData")
        .field("x", &JSTextData::x)
        .field("y", &JSTextData::y)
        .field("height", &JSTextData::height)
        .field("angle", &JSTextData::angle)
        .field("text", &JSTextData::text)
        .field("textBytes", &JSTextData::textBytes)
        .field("color", &JSTextData::color);
    
    value_object<JSEllipseData>("EllipseData")
        .field("cx", &JSEllipseData::cx)
        .field("cy", &JSEllipseData::cy)
        .field("majorAxis", &JSEllipseData::majorAxis)
        .field("ratio", &JSEllipseData::ratio)
        .field("angle", &JSEllipseData::angle)
        .field("color", &JSEllipseData::color);
    
    value_object<JSPointData>("PointData")
        .field("x", &JSPointData::x)
        .field("y", &JSPointData::y)
        .field("z", &JSPointData::z)
        .field("color", &JSPointData::color);
    
    value_object<JSVertexData>("VertexData")
        .field("x", &JSVertexData::x)
        .field("y", &JSVertexData::y)
        .field("z", &JSVertexData::z)
        .field("bulge", &JSVertexData::bulge);
    
    value_object<JSPolylineData>("PolylineData")
        .field("vertices", &JSPolylineData::vertices)
        .field("closed", &JSPolylineData::closed)
        .field("color", &JSPolylineData::color);
    
    class_<JSSolidData>("SolidData")
        .function("getX", &JSSolidData::getX)
        .function("getY", &JSSolidData::getY)
        .function("getZ", &JSSolidData::getZ)
        .property("color", &JSSolidData::color);
    
    value_object<JSMTextData>("MTextData")
        .field("x", &JSMTextData::x)
        .field("y", &JSMTextData::y)
        .field("z", &JSMTextData::z)
        .field("height", &JSMTextData::height)
        .field("width", &JSMTextData::width)
        .field("attachmentPoint", &JSMTextData::attachmentPoint)
        .field("drawingDirection", &JSMTextData::drawingDirection)
        .field("lineSpacingStyle", &JSMTextData::lineSpacingStyle)
        .field("lineSpacingFactor", &JSMTextData::lineSpacingFactor)
        .field("text", &JSMTextData::text)
        .field("textBytes", &JSMTextData::textBytes)
        .field("style", &JSMTextData::style)
        .field("angle", &JSMTextData::angle)
        .field("color", &JSMTextData::color);
    
    value_object<JSDimensionData>("DimensionData")
        .field("dpx", &JSDimensionData::dpx)
        .field("dpy", &JSDimensionData::dpy)
        .field("dpz", &JSDimensionData::dpz)
        .field("mpx", &JSDimensionData::mpx)
        .field("mpy", &JSDimensionData::mpy)
        .field("mpz", &JSDimensionData::mpz)
        .field("type", &JSDimensionData::type)
        .field("attachmentPoint", &JSDimensionData::attachmentPoint)
        .field("text", &JSDimensionData::text)
        .field("angle", &JSDimensionData::angle)
        .field("dpx1", &JSDimensionData::dpx1)
        .field("dpy1", &JSDimensionData::dpy1)
        .field("dpz1", &JSDimensionData::dpz1)
        .field("dpx2", &JSDimensionData::dpx2)
        .field("dpy2", &JSDimensionData::dpy2)
        .field("dpz2", &JSDimensionData::dpz2)
        .field("dimLineAngle", &JSDimensionData::dimLineAngle)
        .field("color", &JSDimensionData::color);
    
    // Unified entity structure
    class_<JSEntity>("Entity")
        .property("type", &JSEntity::type)
        .property("x1", &JSEntity::x1)
        .property("y1", &JSEntity::y1)
        .property("x2", &JSEntity::x2)
        .property("y2", &JSEntity::y2)
        .property("cx", &JSEntity::cx)
        .property("cy", &JSEntity::cy)
        .property("radius", &JSEntity::radius)
        .property("angle1", &JSEntity::angle1)
        .property("angle2", &JSEntity::angle2)
        .property("x", &JSEntity::x)
        .property("y", &JSEntity::y)
        .property("height", &JSEntity::height)
        .property("angle", &JSEntity::angle)
        .property("text", &JSEntity::text)
        .property("majorAxis", &JSEntity::majorAxis)
        .property("ratio", &JSEntity::ratio)
        .property("z", &JSEntity::z)
        .property("vertices", &JSEntity::vertices)
        .property("closed", &JSEntity::closed)
        .property("color", &JSEntity::color)
        .function("getSolidX", &JSEntity::getSolidX)
        .function("getSolidY", &JSEntity::getSolidY)
        .function("getSolidZ", &JSEntity::getSolidZ);
    
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
    register_vector<JSPointData>("PointDataVector");
    register_vector<JSVertexData>("VertexDataVector");
    register_vector<JSPolylineData>("PolylineDataVector");
    register_vector<JSSolidData>("SolidDataVector");
    register_vector<JSMTextData>("MTextDataVector");
    register_vector<JSDimensionData>("DimensionDataVector");
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
        .function("getPoints", &JWWReader::getPoints)
        .function("getPolylines", &JWWReader::getPolylines)
        .function("getSolids", &JWWReader::getSolids)
        .function("getMTexts", &JWWReader::getMTexts)
        .function("getDimensions", &JWWReader::getDimensions)
        .function("getEntities", &JWWReader::getEntities)
        .function("getHeader", &JWWReader::getHeader);
}
#endif // EMSCRIPTEN