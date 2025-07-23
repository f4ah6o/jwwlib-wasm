// WASM bindings for jwwlib
// This file will contain Embind bindings for JavaScript interface

#ifdef EMSCRIPTEN
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

#include "dl_jww.h"
#include "dl_creationinterface.h"
#include "batch_processing.h"
#include <vector>
#include <memory>
#include <cmath>
#include <map>
#include <sstream>

// Error types for parsing
enum class ParseErrorType {
    NONE = 0,
    INVALID_BLOCK_REFERENCE,
    INVALID_IMAGE_REFERENCE,
    INVALID_HATCH_BOUNDARY,
    INVALID_LEADER_PATH,
    INVALID_DIMENSION_DATA,
    MEMORY_ALLOCATION_FAILED,
    UNKNOWN_ENTITY_TYPE
};

// Error information structure
struct JSParseError {
    ParseErrorType type;
    std::string message;
    std::string entityType;
    int lineNumber;
    
    // Default constructor for Embind
    JSParseError() : type(ParseErrorType::NONE), lineNumber(0) {}
    
    JSParseError(ParseErrorType t, const std::string& msg, const std::string& entity = "", int line = 0)
        : type(t), message(msg), entityType(entity), lineNumber(line) {}
};

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

// Spline control point
struct JSControlPointData {
    double x, y, z;
    double weight;  // For NURBS (usually 1.0)
};

// Spline entity data
struct JSSplineData {
    int degree;                                      // Spline degree (usually 3 for cubic)
    std::vector<double> knotValues;                  // Knot vector
    std::vector<JSControlPointData> controlPoints;   // Control points
    bool closed;                                     // Is spline closed
    int color;
};

// Block definition data
struct JSBlockData {
    std::string name;                                // Block name
    double baseX, baseY, baseZ;                      // Base point
    std::string description;                         // Block description
};

// Block reference (insert) data
struct JSInsertData {
    std::string blockName;                           // Referenced block name
    double ipx, ipy, ipz;                            // Insertion point
    double sx, sy, sz;                               // Scale factors
    double angle;                                    // Rotation angle (radians)
    int cols, rows;                                  // Array dimensions
    double colSpacing, rowSpacing;                   // Array spacing
    int color;
};

// Hatch boundary edge data
struct JSHatchEdgeData {
    int type;                                        // Edge type (1=line, 2=arc, 3=ellipse, 4=spline)
    // Line data
    double x1, y1, x2, y2;
    // Arc data
    double cx, cy, radius;
    double angle1, angle2;
    // For complex edges, store vertex data
    std::vector<JSVertexData> vertices;
};

// Hatch loop data
struct JSHatchLoopData {
    int type;                                        // Loop type (0=default, 1=external, 2=polyline, etc.)
    std::vector<JSHatchEdgeData> edges;             // Loop edges
    bool isCCW;                                      // Counter-clockwise flag
};

// Hatch entity data
struct JSHatchData {
    int patternType;                                 // 0=user-defined, 1=predefined, 2=custom
    std::string patternName;                         // Pattern name
    bool solid;                                      // Solid fill flag
    double angle;                                    // Pattern angle
    double scale;                                    // Pattern scale
    std::vector<JSHatchLoopData> loops;             // Boundary loops
    int color;
};

// Leader entity data
struct JSLeaderData {
    int arrowHeadFlag;                               // Arrow head flag
    int pathType;                                    // Path type (0=straight, 1=spline)
    int annotationType;                              // Annotation type
    double dimScaleOverall;                          // Overall dimension scale
    double arrowHeadSize;                            // Arrow head size
    std::vector<JSVertexData> vertices;              // Leader vertices
    std::string annotationReference;                 // Associated annotation handle
    int color;
};

// Image definition data
struct JSImageDefData {
    std::string fileName;                            // Image file path
    double sizeX, sizeY;                             // Image size in pixels
    std::string imageData;                           // Base64 encoded image data
};

// Image entity data
struct JSImageData {
    std::string imageDefHandle;                      // Reference to image definition
    double ipx, ipy, ipz;                            // Insertion point
    double ux, uy, uz;                               // U-vector (width direction)
    double vx, vy, vz;                               // V-vector (height direction)
    double width, height;                            // Display size
    int displayProperties;                           // Display props (clipping, transparency, etc.)
    double brightness;                               // Brightness (0-100)
    double contrast;                                 // Contrast (0-100)
    double fade;                                     // Fade (0-100)
    bool clippingState;                              // Clipping enabled
    std::vector<JSVertexData> clippingVertices;     // Clipping boundary
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
    // Entity storage with pre-allocated capacity
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
    std::vector<JSSplineData> splines;
    JSSplineData* currentSpline = nullptr;
    std::vector<JSBlockData> blocks;
    std::vector<JSInsertData> inserts;
    std::vector<JSHatchData> hatches;
    JSHatchData* currentHatch = nullptr;
    JSHatchLoopData* currentHatchLoop = nullptr;
    std::vector<JSLeaderData> leaders;
    JSLeaderData* currentLeader = nullptr;
    std::vector<JSImageData> images;
    std::vector<JSImageDefData> imageDefs;
    std::map<std::string, int> blockNameToIndex;
    std::map<std::string, int> imageDefHandleToIndex;
    std::vector<JSParseError> parseErrors;
    int currentColor = 256;  // Default to BYLAYER
    
    // Memory optimization
    static constexpr size_t INITIAL_CAPACITY = 1000;
    static constexpr size_t GROWTH_FACTOR = 2;
    
    // Batch processing support
    BatchedEntityConverter batchConverter;
    BatchedIndexBuilder<std::string, int> blockIndexBuilder;
    BatchedIndexBuilder<std::string, int> imageIndexBuilder;
    ProgressiveEntityLoader progressiveLoader;
    
public:
    // Public method to reserve capacity
    void reserveCapacity(size_t capacity) {
        if (capacity > lines.capacity()) {
            lines.reserve(capacity);
        }
    }
    
    // Public method to add parse error
    void addParseError(const JSParseError& error) {
        parseErrors.push_back(error);
    }
    // Constructor with memory pre-allocation
    JSCreationInterface() : progressiveLoader(10000) {
        // Pre-allocate memory for common entity types
        lines.reserve(INITIAL_CAPACITY);
        circles.reserve(INITIAL_CAPACITY / 4);
        arcs.reserve(INITIAL_CAPACITY / 4);
        texts.reserve(INITIAL_CAPACITY / 10);
        polylines.reserve(INITIAL_CAPACITY / 10);
        blocks.reserve(100);
        inserts.reserve(INITIAL_CAPACITY / 5);
        
        // Initialize index builders with appropriate batch sizes
        blockIndexBuilder = BatchedIndexBuilder<std::string, int>(100);
        imageIndexBuilder = BatchedIndexBuilder<std::string, int>(50);
    }
    
    // Constructor with file size hint for better pre-allocation
    JSCreationInterface(size_t fileSize) : progressiveLoader(10000) {
        // Estimate entity counts based on file size
        size_t estimatedEntities = fileSize / 100; // Rough estimate
        
        // Pre-allocate with batched sizing
        size_t batchSize = std::max<size_t>(1000, estimatedEntities / 10);
        lines.reserve(batchSize * 4);
        circles.reserve(batchSize);
        arcs.reserve(batchSize);
        texts.reserve(batchSize / 2);
        polylines.reserve(batchSize / 2);
        blocks.reserve(std::min<size_t>(1000, batchSize / 10));
        inserts.reserve(batchSize * 2);
        hatches.reserve(batchSize / 4);
        leaders.reserve(batchSize / 10);
        
        // Initialize index builders
        blockIndexBuilder = BatchedIndexBuilder<std::string, int>(100);
        imageIndexBuilder = BatchedIndexBuilder<std::string, int>(50);
    }
    
    // Memory usage estimation
    size_t getEstimatedMemoryUsage() const {
        size_t total = 0;
        total += lines.capacity() * sizeof(JSLineData);
        total += circles.capacity() * sizeof(JSCircleData);
        total += arcs.capacity() * sizeof(JSArcData);
        total += texts.capacity() * sizeof(JSTextData);
        total += ellipses.capacity() * sizeof(JSEllipseData);
        total += points.capacity() * sizeof(JSPointData);
        total += polylines.capacity() * sizeof(JSPolylineData);
        total += solids.capacity() * sizeof(JSSolidData);
        total += mtexts.capacity() * sizeof(JSMTextData);
        total += dimensions.capacity() * sizeof(JSDimensionData);
        total += splines.capacity() * sizeof(JSSplineData);
        total += blocks.capacity() * sizeof(JSBlockData);
        total += inserts.capacity() * sizeof(JSInsertData);
        total += hatches.capacity() * sizeof(JSHatchData);
        total += leaders.capacity() * sizeof(JSLeaderData);
        total += images.capacity() * sizeof(JSImageData);
        total += imageDefs.capacity() * sizeof(JSImageDefData);
        return total;
    }
    
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
    const std::vector<JSSplineData>& getSplines() const { return splines; }
    const std::vector<JSBlockData>& getBlocks() const { return blocks; }
    const std::vector<JSInsertData>& getInserts() const { return inserts; }
    const std::vector<JSHatchData>& getHatches() const { return hatches; }
    const std::vector<JSLeaderData>& getLeaders() const { return leaders; }
    const std::vector<JSImageData>& getImages() const { return images; }
    const std::vector<JSImageDefData>& getImageDefs() const { return imageDefs; }
    const std::vector<JSParseError>& getParseErrors() const { return parseErrors; }
    
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
        splines.clear();
        currentSpline = nullptr;
        blocks.clear();
        inserts.clear();
        hatches.clear();
        currentHatch = nullptr;
        currentHatchLoop = nullptr;
        leaders.clear();
        currentLeader = nullptr;
        images.clear();
        imageDefs.clear();
        blockNameToIndex.clear();
        imageDefHandleToIndex.clear();
        parseErrors.clear();
        blockIndexBuilder.clear();
        imageIndexBuilder.clear();
    }
    
    // Batch processing methods
    void processBatchedLines(const std::vector<std::tuple<double, double, double, double, int>>& lineData) {
        BatchedJSOperations::addLinesBatch(lines, lineData);
    }
    
    // Set progress callback for large file processing
    void setProgressCallback(std::function<void(size_t, size_t)> callback) {
        progressiveLoader.setProgressCallback(callback);
    }
    
    // Build indexes in batch mode
    void buildIndexes() {
        // Build block name index
        std::vector<std::pair<std::string, int>> blockPairs;
        for (size_t i = 0; i < blocks.size(); ++i) {
            blockPairs.emplace_back(blocks[i].name, i);
        }
        blockIndexBuilder.buildIndex(blockPairs);
        
        // Build image definition index
        std::vector<std::pair<std::string, int>> imagePairs;
        for (size_t i = 0; i < imageDefs.size(); ++i) {
            imagePairs.emplace_back(imageDefs[i].fileName, i);
        }
        imageIndexBuilder.buildIndex(imagePairs);
    }
    
    // Get entity statistics with batch counting
    std::map<std::string, size_t> getEntityStats() const {
        std::vector<std::pair<std::string, size_t>> typeCounts = {
            {"lines", lines.size()},
            {"circles", circles.size()},
            {"arcs", arcs.size()},
            {"texts", texts.size()},
            {"ellipses", ellipses.size()},
            {"points", points.size()},
            {"polylines", polylines.size()},
            {"solids", solids.size()},
            {"mtexts", mtexts.size()},
            {"dimensions", dimensions.size()},
            {"splines", splines.size()},
            {"blocks", blocks.size()},
            {"inserts", inserts.size()},
            {"hatches", hatches.size()},
            {"leaders", leaders.size()},
            {"images", images.size()},
            {"imageDefs", imageDefs.size()}
        };
        
        return BatchedJSOperations::countEntitiesByType(typeCounts);
    }
    
    // DL_CreationInterface implementation
    void setAttributes(const DL_Attributes& attrib) {
        currentColor = attrib.getColor();
        // Call parent implementation
        DL_CreationInterface::setAttributes(attrib);
    }
    virtual void addLayer(const DL_LayerData& /*data*/) override {}
    virtual void addBlock(const DL_BlockData& data) override {
        // Check for duplicate block definition
        auto it = blockNameToIndex.find(data.name);
        if (it != blockNameToIndex.end()) {
            // Block already exists, update if needed or skip
            parseErrors.push_back(JSParseError(
                ParseErrorType::NONE,
                "Duplicate block definition: " + data.name,
                "BLOCK"
            ));
            return;
        }
        
        JSBlockData block;
        block.name = data.name;
        block.baseX = data.bpx;
        block.baseY = data.bpy;
        block.baseZ = data.bpz;
        block.description = "";  // JWW may not have descriptions
        
        // Store block and update index map
        blockNameToIndex[block.name] = blocks.size();
        blocks.push_back(std::move(block));  // Use move semantics
    }
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
    virtual void addSpline(const DL_SplineData& data) override {
        JSSplineData spline;
        spline.degree = data.degree;
        spline.closed = (data.flags & 0x01) != 0;  // Check closed flag
        spline.color = currentColor;
        // Reserve space for knots and control points
        spline.knotValues.reserve(data.nKnots);
        spline.controlPoints.reserve(data.nControl);
        splines.push_back(spline);
        currentSpline = &splines.back();
    }
    virtual void addControlPoint(const DL_ControlPointData& data) override {
        if (currentSpline) {
            currentSpline->controlPoints.push_back({data.x, data.y, data.z, 1.0});
        }
    }
    virtual void addKnot(const DL_KnotData& data) override {
        if (currentSpline) {
            currentSpline->knotValues.push_back(data.k);
        }
    }
    virtual void addInsert(const DL_InsertData& data) override {
        JSInsertData insert;
        insert.blockName = data.name;
        insert.ipx = data.ipx;
        insert.ipy = data.ipy;
        insert.ipz = data.ipz;
        insert.sx = data.sx;
        insert.sy = data.sy;
        insert.sz = data.sz;
        insert.angle = data.angle * M_PI / 180.0;  // Convert to radians
        insert.cols = data.cols;
        insert.rows = data.rows;
        insert.colSpacing = data.colSp;
        insert.rowSpacing = data.rowSp;
        insert.color = currentColor;
        
        // Validate block reference
        if (blockNameToIndex.find(insert.blockName) == blockNameToIndex.end()) {
            parseErrors.push_back(JSParseError(
                ParseErrorType::INVALID_BLOCK_REFERENCE,
                "Block '" + insert.blockName + "' not found",
                "INSERT"
            ));
        }
        
        inserts.push_back(insert);
    }
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
    virtual void addDimRadial(const DL_DimensionData& data, const DL_DimRadialData& edata) override {
        dimensions.push_back({
            data.dpx, data.dpy, data.dpz,
            data.mpx, data.mpy, data.mpz,
            2, // type = radial
            data.attachmentPoint,
            data.text,
            data.angle,
            edata.dpx, edata.dpy, edata.dpz,  // Center point
            edata.dpx + edata.leader, edata.dpy, edata.dpz,  // Point on circle
            0.0,
            currentColor
        });
    }
    virtual void addDimDiametric(const DL_DimensionData& data, const DL_DimDiametricData& edata) override {
        dimensions.push_back({
            data.dpx, data.dpy, data.dpz,
            data.mpx, data.mpy, data.mpz,
            3, // type = diametric
            data.attachmentPoint,
            data.text,
            data.angle,
            edata.dpx, edata.dpy, edata.dpz,  // Center point
            edata.dpx + edata.leader, edata.dpy, edata.dpz,  // Point on diameter
            0.0,
            currentColor
        });
    }
    virtual void addDimAngular(const DL_DimensionData& data, const DL_DimAngularData& edata) override {
        dimensions.push_back({
            data.dpx, data.dpy, data.dpz,
            data.mpx, data.mpy, data.mpz,
            4, // type = angular
            data.attachmentPoint,
            data.text,
            data.angle,
            edata.dpx1, edata.dpy1, edata.dpz1,  // Line 1 start
            edata.dpx2, edata.dpy2, edata.dpz2,  // Line 1 end
            0.0,  // Store additional data in separate fields if needed
            currentColor
        });
    }
    virtual void addDimAngular3P(const DL_DimensionData& data, const DL_DimAngular3PData& edata) override {
        dimensions.push_back({
            data.dpx, data.dpy, data.dpz,
            data.mpx, data.mpy, data.mpz,
            5, // type = angular3p
            data.attachmentPoint,
            data.text,
            data.angle,
            edata.dpx1, edata.dpy1, edata.dpz1,  // First point
            edata.dpx2, edata.dpy2, edata.dpz2,  // Second point
            0.0,  // Third point stored separately if needed
            currentColor
        });
    }
    virtual void addDimOrdinate(const DL_DimensionData& data, const DL_DimOrdinateData& edata) override {
        dimensions.push_back({
            data.dpx, data.dpy, data.dpz,
            data.mpx, data.mpy, data.mpz,
            6, // type = ordinate
            data.attachmentPoint,
            data.text,
            data.angle,
            edata.dpx1, edata.dpy1, edata.dpz1,  // Feature location
            edata.dpx2, edata.dpy2, edata.dpz2,  // Leader endpoint
            0.0,
            currentColor
        });
    }
    virtual void addLeader(const DL_LeaderData& data) override {
        JSLeaderData leader;
        leader.arrowHeadFlag = data.arrowHeadFlag;
        leader.pathType = data.leaderPathType;
        leader.annotationType = 0;  // Default annotation type
        leader.dimScaleOverall = 1.0;  // Default scale
        leader.arrowHeadSize = 0.18;  // Default arrow size
        leader.color = currentColor;
        
        leaders.push_back(leader);
        currentLeader = &leaders.back();
    }
    virtual void addLeaderVertex(const DL_LeaderVertexData& data) override {
        if (currentLeader) {
            currentLeader->vertices.push_back({data.x, data.y, data.z, 0.0});
        }
    }
    virtual void addHatch(const DL_HatchData& data) override {
        JSHatchData hatch;
        hatch.patternType = data.solid ? 2 : 1;  // Map pattern type
        hatch.patternName = data.solid ? "SOLID" : data.pattern;
        hatch.solid = data.solid;
        hatch.angle = data.angle * M_PI / 180.0;  // Convert to radians
        hatch.scale = data.scale;
        hatch.color = currentColor;
        
        hatches.push_back(hatch);
        currentHatch = &hatches.back();
    }
    virtual void addImage(const DL_ImageData& data) override {
        JSImageData image;
        image.imageDefHandle = data.ref;
        image.ipx = data.ipx;
        image.ipy = data.ipy;
        image.ipz = data.ipz;
        image.ux = data.ux;
        image.uy = data.uy;
        image.uz = data.uz;
        image.vx = data.vx;
        image.vy = data.vy;
        image.vz = data.vz;
        image.width = data.width;
        image.height = data.height;
        image.brightness = data.brightness;
        image.contrast = data.contrast;
        image.fade = data.fade;
        
        images.push_back(image);
    }
    virtual void linkImage(const DL_ImageDefData& data) override {
        JSImageDefData imageDef;
        imageDef.fileName = data.file;
        imageDef.sizeX = 0;  // Will be set from actual image
        imageDef.sizeY = 0;
        imageDef.imageData = "";  // TODO: Base64 encode if needed
        
        imageDefHandleToIndex[data.ref] = imageDefs.size();
        imageDefs.push_back(imageDef);
    }
    virtual void addHatchLoop(const DL_HatchLoopData& data) override {
        if (currentHatch) {
            JSHatchLoopData loop;
            loop.type = data.numEdges > 0 ? 1 : 0;  // Simple type mapping
            loop.isCCW = true;  // Default to CCW
            
            currentHatch->loops.push_back(loop);
            currentHatchLoop = &currentHatch->loops.back();
        }
    }
    virtual void addHatchEdge(const DL_HatchEdgeData& data) override {
        if (currentHatchLoop) {
            JSHatchEdgeData edge;
            
            if (data.type == 1) {  // Line
                edge.type = 1;
                edge.x1 = data.x1;
                edge.y1 = data.y1;
                edge.x2 = data.x2;
                edge.y2 = data.y2;
            } else if (data.type == 2) {  // Arc
                edge.type = 2;
                edge.cx = data.cx;
                edge.cy = data.cy;
                edge.radius = data.radius;
                edge.angle1 = data.angle1;
                edge.angle2 = data.angle2;
            } else if (data.type == 3) {  // Ellipse
                edge.type = 3;
                edge.cx = data.cx;
                edge.cy = data.cy;
                edge.radius = data.radius;  // Major axis
                edge.angle1 = data.angle1;  // Start param
                edge.angle2 = data.angle2;  // End param
            } else if (data.type == 4) {  // Spline
                edge.type = 4;
                // Store control points if available
            } else {
                parseErrors.push_back(JSParseError(
                    ParseErrorType::INVALID_HATCH_BOUNDARY,
                    "Unknown hatch edge type: " + std::to_string(data.type),
                    "HATCH"
                ));
                return;
            }
            
            currentHatchLoop->edges.push_back(edge);
        }
    }
    virtual void setVariableString(const char* /*key*/, const char* /*value*/, int /*code*/) override {}
    virtual void setVariableInt(const char* /*key*/, int /*value*/, int /*code*/) override {}
    virtual void setVariableDouble(const char* /*key*/, double /*value*/, int /*code*/) override {}
    virtual void setVariableVector(const char* /*key*/, double /*v1*/, double /*v2*/, double /*v3*/, int /*code*/) override {}
    virtual void addComment(const char* /*comment*/) override {}
    virtual void endSequence() override {
        currentPolyline = nullptr;
        currentSpline = nullptr;
        currentHatch = nullptr;
        currentHatchLoop = nullptr;
        currentLeader = nullptr;
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
    JWWReader(uintptr_t dataPtr, size_t size) : creationInterface(std::make_unique<JSCreationInterface>(size)) {
        readFile(dataPtr, size);
    }
    
    // Constructor with progress callback support
    JWWReader(uintptr_t dataPtr, size_t size, emscripten::val progressCallback) 
        : creationInterface(std::make_unique<JSCreationInterface>(size)) {
        if (!progressCallback.isNull() && !progressCallback.isUndefined()) {
            creationInterface->setProgressCallback([progressCallback](size_t current, size_t total) {
                progressCallback(current, total);
            });
        }
        readFile(dataPtr, size);
    }
    
    bool readFile(uintptr_t dataPtr, size_t size) {
        creationInterface->clear();
        
        // Estimate entity count based on file size (rough heuristic)
        size_t estimatedEntities = size / 100;  // Average ~100 bytes per entity
        if (estimatedEntities > 1000) {  // INITIAL_CAPACITY
            // Pre-allocate additional capacity for large files
            creationInterface->reserveCapacity(estimatedEntities);
        }
        
        // Read data from memory
        const uint8_t* data = reinterpret_cast<const uint8_t*>(dataPtr);
        
        // Write buffer to temporary file in MEMFS
        // Note: This is required by DL_Jww API, but MEMFS keeps it in memory
        std::string tempFile = "/tmp/jww_temp.jww";
        FILE* fp = fopen(tempFile.c_str(), "wb");
        if (!fp) {
            creationInterface->addParseError(JSParseError(
                ParseErrorType::MEMORY_ALLOCATION_FAILED,
                "Failed to create temporary file",
                "FILE"
            ));
            return false;
        }
        
        // Direct write without intermediate buffer
        size_t written = fwrite(data, 1, size, fp);
        fclose(fp);
        
        if (written != size) {
            remove(tempFile.c_str());
            creationInterface->addParseError(JSParseError(
                ParseErrorType::MEMORY_ALLOCATION_FAILED,
                "Failed to write temporary file",
                "FILE"
            ));
            return false;
        }
        
        // Use DL_Jww to read the file
        jww = std::make_unique<DL_Jww>();
        bool result = jww->in(tempFile, creationInterface.get());
        
        // Clean up temporary file immediately
        remove(tempFile.c_str());
        
        // Build indexes after successful parsing
        if (result) {
            creationInterface->buildIndexes();
        }
        
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
    
    const std::vector<JSSplineData>& getSplines() const {
        return creationInterface->getSplines();
    }
    
    const std::vector<JSBlockData>& getBlocks() const {
        return creationInterface->getBlocks();
    }
    
    const std::vector<JSInsertData>& getInserts() const {
        return creationInterface->getInserts();
    }
    
    const std::vector<JSHatchData>& getHatches() const {
        return creationInterface->getHatches();
    }
    
    const std::vector<JSLeaderData>& getLeaders() const {
        return creationInterface->getLeaders();
    }
    
    const std::vector<JSImageData>& getImages() const {
        return creationInterface->getImages();
    }
    
    const std::vector<JSImageDefData>& getImageDefs() const {
        return creationInterface->getImageDefs();
    }
    
    const std::vector<JSParseError>& getParsingErrors() const {
        return creationInterface->getParseErrors();
    }
    
    // Memory usage information
    size_t getMemoryUsage() const {
        return creationInterface->getEstimatedMemoryUsage();
    }
    
    // Get entity count by type for statistics
    std::map<std::string, int> getEntityStats() const {
        // Use batch counting for better performance
        auto stats = creationInterface->getEntityStats();
        std::map<std::string, int> result;
        for (const auto& pair : stats) {
            result[pair.first] = pair.second;
        }
        return result;
    }
    
    // Batch processing methods
    void processBatchedLines(const std::vector<std::tuple<double, double, double, double, int>>& lineData) {
        creationInterface->processBatchedLines(lineData);
    }
    
    // Set progress callback for batch processing
    void setProgressCallback(emscripten::val callback) {
        if (!callback.isNull() && !callback.isUndefined()) {
            creationInterface->setProgressCallback([callback](size_t current, size_t total) {
                callback(current, total);
            });
        }
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
        
        // Add splines
        for (const auto& spline : creationInterface->getSplines()) {
            JSEntity entity;
            entity.type = "SPLINE";
            entity.color = spline.color;
            entity.closed = spline.closed;
            // Note: Spline data is stored in a separate structure
            // This is just a placeholder for the entity list
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
                           creationInterface->getDimensions().size() +
                           creationInterface->getSplines().size() +
                           creationInterface->getInserts().size() +
                           creationInterface->getHatches().size() +
                           creationInterface->getLeaders().size() +
                           creationInterface->getImages().size();
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
    
    value_object<JSControlPointData>("ControlPointData")
        .field("x", &JSControlPointData::x)
        .field("y", &JSControlPointData::y)
        .field("z", &JSControlPointData::z)
        .field("weight", &JSControlPointData::weight);
    
    value_object<JSSplineData>("SplineData")
        .field("degree", &JSSplineData::degree)
        .field("knotValues", &JSSplineData::knotValues)
        .field("controlPoints", &JSSplineData::controlPoints)
        .field("closed", &JSSplineData::closed)
        .field("color", &JSSplineData::color);
    
    // Block and Insert data
    value_object<JSBlockData>("BlockData")
        .field("name", &JSBlockData::name)
        .field("baseX", &JSBlockData::baseX)
        .field("baseY", &JSBlockData::baseY)
        .field("baseZ", &JSBlockData::baseZ)
        .field("description", &JSBlockData::description);
    
    value_object<JSInsertData>("InsertData")
        .field("blockName", &JSInsertData::blockName)
        .field("ipx", &JSInsertData::ipx)
        .field("ipy", &JSInsertData::ipy)
        .field("ipz", &JSInsertData::ipz)
        .field("sx", &JSInsertData::sx)
        .field("sy", &JSInsertData::sy)
        .field("sz", &JSInsertData::sz)
        .field("angle", &JSInsertData::angle)
        .field("cols", &JSInsertData::cols)
        .field("rows", &JSInsertData::rows)
        .field("colSpacing", &JSInsertData::colSpacing)
        .field("rowSpacing", &JSInsertData::rowSpacing)
        .field("color", &JSInsertData::color);
    
    // Hatch data
    value_object<JSHatchEdgeData>("HatchEdgeData")
        .field("type", &JSHatchEdgeData::type)
        .field("x1", &JSHatchEdgeData::x1)
        .field("y1", &JSHatchEdgeData::y1)
        .field("x2", &JSHatchEdgeData::x2)
        .field("y2", &JSHatchEdgeData::y2)
        .field("cx", &JSHatchEdgeData::cx)
        .field("cy", &JSHatchEdgeData::cy)
        .field("radius", &JSHatchEdgeData::radius)
        .field("angle1", &JSHatchEdgeData::angle1)
        .field("angle2", &JSHatchEdgeData::angle2)
        .field("vertices", &JSHatchEdgeData::vertices);
    
    value_object<JSHatchLoopData>("HatchLoopData")
        .field("type", &JSHatchLoopData::type)
        .field("edges", &JSHatchLoopData::edges)
        .field("isCCW", &JSHatchLoopData::isCCW);
    
    value_object<JSHatchData>("HatchData")
        .field("patternType", &JSHatchData::patternType)
        .field("patternName", &JSHatchData::patternName)
        .field("solid", &JSHatchData::solid)
        .field("angle", &JSHatchData::angle)
        .field("scale", &JSHatchData::scale)
        .field("loops", &JSHatchData::loops)
        .field("color", &JSHatchData::color);
    
    // Leader data
    value_object<JSLeaderData>("LeaderData")
        .field("arrowHeadFlag", &JSLeaderData::arrowHeadFlag)
        .field("pathType", &JSLeaderData::pathType)
        .field("annotationType", &JSLeaderData::annotationType)
        .field("dimScaleOverall", &JSLeaderData::dimScaleOverall)
        .field("arrowHeadSize", &JSLeaderData::arrowHeadSize)
        .field("vertices", &JSLeaderData::vertices)
        .field("annotationReference", &JSLeaderData::annotationReference)
        .field("color", &JSLeaderData::color);
    
    // Image data
    value_object<JSImageDefData>("ImageDefData")
        .field("fileName", &JSImageDefData::fileName)
        .field("sizeX", &JSImageDefData::sizeX)
        .field("sizeY", &JSImageDefData::sizeY)
        .field("imageData", &JSImageDefData::imageData);
    
    value_object<JSImageData>("ImageData")
        .field("imageDefHandle", &JSImageData::imageDefHandle)
        .field("ipx", &JSImageData::ipx)
        .field("ipy", &JSImageData::ipy)
        .field("ipz", &JSImageData::ipz)
        .field("ux", &JSImageData::ux)
        .field("uy", &JSImageData::uy)
        .field("uz", &JSImageData::uz)
        .field("vx", &JSImageData::vx)
        .field("vy", &JSImageData::vy)
        .field("vz", &JSImageData::vz)
        .field("width", &JSImageData::width)
        .field("height", &JSImageData::height)
        .field("displayProperties", &JSImageData::displayProperties)
        .field("brightness", &JSImageData::brightness)
        .field("contrast", &JSImageData::contrast)
        .field("fade", &JSImageData::fade)
        .field("clippingState", &JSImageData::clippingState)
        .field("clippingVertices", &JSImageData::clippingVertices);
    
    // Parse error data
    enum_<ParseErrorType>("ParseErrorType")
        .value("NONE", ParseErrorType::NONE)
        .value("INVALID_BLOCK_REFERENCE", ParseErrorType::INVALID_BLOCK_REFERENCE)
        .value("INVALID_IMAGE_REFERENCE", ParseErrorType::INVALID_IMAGE_REFERENCE)
        .value("INVALID_HATCH_BOUNDARY", ParseErrorType::INVALID_HATCH_BOUNDARY)
        .value("INVALID_LEADER_PATH", ParseErrorType::INVALID_LEADER_PATH)
        .value("INVALID_DIMENSION_DATA", ParseErrorType::INVALID_DIMENSION_DATA)
        .value("MEMORY_ALLOCATION_FAILED", ParseErrorType::MEMORY_ALLOCATION_FAILED)
        .value("UNKNOWN_ENTITY_TYPE", ParseErrorType::UNKNOWN_ENTITY_TYPE);
    
    value_object<JSParseError>("ParseError")
        .field("type", &JSParseError::type)
        .field("message", &JSParseError::message)
        .field("entityType", &JSParseError::entityType)
        .field("lineNumber", &JSParseError::lineNumber);
    
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
    register_vector<JSControlPointData>("ControlPointDataVector");
    register_vector<JSSplineData>("SplineDataVector");
    register_vector<JSEntity>("EntityVector");
    register_vector<uint8_t>("Uint8Vector");
    register_vector<JSBlockData>("BlockDataVector");
    register_vector<JSInsertData>("InsertDataVector");
    register_vector<JSHatchEdgeData>("HatchEdgeDataVector");
    register_vector<JSHatchLoopData>("HatchLoopDataVector");
    register_vector<JSHatchData>("HatchDataVector");
    register_vector<JSLeaderData>("LeaderDataVector");
    register_vector<JSImageData>("ImageDataVector");
    register_vector<JSImageDefData>("ImageDefDataVector");
    register_vector<JSParseError>("ParseErrorVector");
    register_vector<double>("DoubleVector");
    register_map<std::string, int>("StringIntMap");
    
    // JWWReader class
    class_<JWWReader>("JWWReader")
        .constructor<>()
        .constructor<uintptr_t, size_t>()
        .constructor<uintptr_t, size_t, emscripten::val>()
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
        .function("getSplines", &JWWReader::getSplines)
        .function("getBlocks", &JWWReader::getBlocks)
        .function("getInserts", &JWWReader::getInserts)
        .function("getHatches", &JWWReader::getHatches)
        .function("getLeaders", &JWWReader::getLeaders)
        .function("getImages", &JWWReader::getImages)
        .function("getImageDefs", &JWWReader::getImageDefs)
        .function("getParsingErrors", &JWWReader::getParsingErrors)
        .function("getEntities", &JWWReader::getEntities)
        .function("getHeader", &JWWReader::getHeader)
        .function("getMemoryUsage", &JWWReader::getMemoryUsage)
        .function("getEntityStats", &JWWReader::getEntityStats)
        .function("processBatchedLines", &JWWReader::processBatchedLines)
        .function("setProgressCallback", &JWWReader::setProgressCallback);
}
#endif // EMSCRIPTEN