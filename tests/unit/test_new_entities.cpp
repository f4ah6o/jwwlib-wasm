// Unit tests for new entity types in jwwlib-wasm
// Tests block, insert, hatch, dimension, leader, and image entities

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "../../src/wasm/wasm_bindings.cpp"

// Test fixture for entity parsing tests
class NewEntityTest : public ::testing::Test {
protected:
    std::unique_ptr<JSCreationInterface> creationInterface;
    
    void SetUp() override {
        creationInterface = std::make_unique<JSCreationInterface>();
    }
    
    void TearDown() override {
        creationInterface.reset();
    }
};

// Block entity tests
TEST_F(NewEntityTest, AddBlockBasic) {
    DL_BlockData blockData;
    blockData.name = "TestBlock";
    blockData.bpx = 100.0;
    blockData.bpy = 200.0;
    blockData.bpz = 0.0;
    
    creationInterface->addBlock(blockData);
    
    const auto& blocks = creationInterface->getBlocks();
    ASSERT_EQ(blocks.size(), 1);
    EXPECT_EQ(blocks[0].name, "TestBlock");
    EXPECT_DOUBLE_EQ(blocks[0].baseX, 100.0);
    EXPECT_DOUBLE_EQ(blocks[0].baseY, 200.0);
    EXPECT_DOUBLE_EQ(blocks[0].baseZ, 0.0);
}

TEST_F(NewEntityTest, AddBlockDuplicateHandling) {
    DL_BlockData blockData;
    blockData.name = "DuplicateBlock";
    blockData.bpx = 0.0;
    blockData.bpy = 0.0;
    blockData.bpz = 0.0;
    
    creationInterface->addBlock(blockData);
    creationInterface->addBlock(blockData); // Add duplicate
    
    const auto& blocks = creationInterface->getBlocks();
    ASSERT_EQ(blocks.size(), 1); // Should only have one block
    
    const auto& errors = creationInterface->getParseErrors();
    ASSERT_EQ(errors.size(), 1);
    EXPECT_EQ(errors[0].type, ParseErrorType::NONE);
    EXPECT_NE(errors[0].message.find("Duplicate block"), std::string::npos);
}

// Insert entity tests
TEST_F(NewEntityTest, AddInsertBasic) {
    // First add a block
    DL_BlockData blockData;
    blockData.name = "RefBlock";
    creationInterface->addBlock(blockData);
    
    // Then add an insert
    DL_InsertData insertData;
    insertData.name = "RefBlock";
    insertData.ipx = 50.0;
    insertData.ipy = 100.0;
    insertData.ipz = 0.0;
    insertData.sx = 1.0;
    insertData.sy = 1.0;
    insertData.sz = 1.0;
    insertData.angle = 0.0;
    insertData.cols = 1;
    insertData.rows = 1;
    insertData.colSp = 0.0;
    insertData.rowSp = 0.0;
    
    creationInterface->addInsert(insertData);
    
    const auto& inserts = creationInterface->getInserts();
    ASSERT_EQ(inserts.size(), 1);
    EXPECT_EQ(inserts[0].blockName, "RefBlock");
    EXPECT_DOUBLE_EQ(inserts[0].ipx, 50.0);
    EXPECT_DOUBLE_EQ(inserts[0].ipy, 100.0);
    EXPECT_DOUBLE_EQ(inserts[0].sx, 1.0);
    EXPECT_DOUBLE_EQ(inserts[0].sy, 1.0);
}

TEST_F(NewEntityTest, AddInsertInvalidReference) {
    DL_InsertData insertData;
    insertData.name = "NonExistentBlock";
    insertData.ipx = 0.0;
    insertData.ipy = 0.0;
    insertData.ipz = 0.0;
    
    creationInterface->addInsert(insertData);
    
    const auto& inserts = creationInterface->getInserts();
    ASSERT_EQ(inserts.size(), 1); // Insert is still added
    
    const auto& errors = creationInterface->getParseErrors();
    ASSERT_EQ(errors.size(), 1);
    EXPECT_EQ(errors[0].type, ParseErrorType::INVALID_BLOCK_REFERENCE);
}

// Hatch entity tests
TEST_F(NewEntityTest, AddHatchSolid) {
    DL_HatchData hatchData;
    hatchData.solid = true;
    hatchData.pattern = "";
    hatchData.angle = 0.0;
    hatchData.scale = 1.0;
    
    creationInterface->addHatch(hatchData);
    
    const auto& hatches = creationInterface->getHatches();
    ASSERT_EQ(hatches.size(), 1);
    EXPECT_TRUE(hatches[0].solid);
    EXPECT_EQ(hatches[0].patternType, 2); // Solid type
    EXPECT_EQ(hatches[0].patternName, "SOLID");
}

TEST_F(NewEntityTest, AddHatchWithPattern) {
    DL_HatchData hatchData;
    hatchData.solid = false;
    hatchData.pattern = "ANSI31";
    hatchData.angle = 45.0;
    hatchData.scale = 2.0;
    
    creationInterface->addHatch(hatchData);
    
    const auto& hatches = creationInterface->getHatches();
    ASSERT_EQ(hatches.size(), 1);
    EXPECT_FALSE(hatches[0].solid);
    EXPECT_EQ(hatches[0].patternType, 1);
    EXPECT_EQ(hatches[0].patternName, "ANSI31");
    EXPECT_DOUBLE_EQ(hatches[0].angle, 45.0 * M_PI / 180.0);
    EXPECT_DOUBLE_EQ(hatches[0].scale, 2.0);
}

TEST_F(NewEntityTest, AddHatchLoopAndEdges) {
    DL_HatchData hatchData;
    hatchData.solid = true;
    creationInterface->addHatch(hatchData);
    
    DL_HatchLoopData loopData;
    loopData.numEdges = 4;
    creationInterface->addHatchLoop(loopData);
    
    // Add line edges
    DL_HatchEdgeData edgeData;
    edgeData.type = 1; // Line
    edgeData.x1 = 0.0;
    edgeData.y1 = 0.0;
    edgeData.x2 = 100.0;
    edgeData.y2 = 0.0;
    creationInterface->addHatchEdge(edgeData);
    
    const auto& hatches = creationInterface->getHatches();
    ASSERT_EQ(hatches.size(), 1);
    ASSERT_EQ(hatches[0].loops.size(), 1);
    ASSERT_EQ(hatches[0].loops[0].edges.size(), 1);
    EXPECT_EQ(hatches[0].loops[0].edges[0].type, 1);
    EXPECT_DOUBLE_EQ(hatches[0].loops[0].edges[0].x1, 0.0);
    EXPECT_DOUBLE_EQ(hatches[0].loops[0].edges[0].x2, 100.0);
}

// Leader entity tests
TEST_F(NewEntityTest, AddLeaderBasic) {
    DL_LeaderData leaderData;
    leaderData.arrowHeadFlag = 1;
    leaderData.leaderPathType = 0; // Straight
    
    creationInterface->addLeader(leaderData);
    
    // Add vertices
    DL_LeaderVertexData vertex1{0.0, 0.0, 0.0};
    DL_LeaderVertexData vertex2{100.0, 100.0, 0.0};
    creationInterface->addLeaderVertex(vertex1);
    creationInterface->addLeaderVertex(vertex2);
    
    const auto& leaders = creationInterface->getLeaders();
    ASSERT_EQ(leaders.size(), 1);
    EXPECT_EQ(leaders[0].arrowHeadFlag, 1);
    EXPECT_EQ(leaders[0].pathType, 0);
    ASSERT_EQ(leaders[0].vertices.size(), 2);
    EXPECT_DOUBLE_EQ(leaders[0].vertices[0].x, 0.0);
    EXPECT_DOUBLE_EQ(leaders[0].vertices[1].x, 100.0);
}

// Dimension entity tests
TEST_F(NewEntityTest, AddDimensionRadial) {
    DL_DimensionData dimData;
    dimData.dpx = 50.0;
    dimData.dpy = 50.0;
    dimData.dpz = 0.0;
    dimData.mpx = 75.0;
    dimData.mpy = 75.0;
    dimData.mpz = 0.0;
    dimData.attachmentPoint = 5;
    dimData.text = "R50";
    dimData.angle = 0.0;
    
    DL_DimRadialData radialData;
    radialData.leader = 25.0;
    
    creationInterface->addDimRadial(dimData, radialData);
    
    const auto& dimensions = creationInterface->getDimensions();
    ASSERT_EQ(dimensions.size(), 1);
    EXPECT_EQ(dimensions[0].type, 2); // Radial type
    EXPECT_EQ(dimensions[0].text, "R50");
    EXPECT_DOUBLE_EQ(dimensions[0].dpx, 50.0);
}

TEST_F(NewEntityTest, AddDimensionAngular) {
    DL_DimensionData dimData;
    dimData.text = "45°";
    
    DL_DimAngularData angularData;
    angularData.dpx1 = 0.0;
    angularData.dpy1 = 0.0;
    angularData.dpz1 = 0.0;
    angularData.dpx2 = 100.0;
    angularData.dpy2 = 0.0;
    angularData.dpz2 = 0.0;
    // Additional fields for second line
    
    creationInterface->addDimAngular(dimData, angularData);
    
    const auto& dimensions = creationInterface->getDimensions();
    ASSERT_EQ(dimensions.size(), 1);
    EXPECT_EQ(dimensions[0].type, 4); // Angular type
    EXPECT_EQ(dimensions[0].text, "45°");
}

// Image entity tests
TEST_F(NewEntityTest, AddImageDefinition) {
    DL_ImageDefData imageDefData;
    imageDefData.ref = "IMG001";
    imageDefData.file = "test_image.png";
    
    creationInterface->linkImage(imageDefData);
    
    const auto& imageDefs = creationInterface->getImageDefs();
    ASSERT_EQ(imageDefs.size(), 1);
    EXPECT_EQ(imageDefs[0].fileName, "test_image.png");
}

TEST_F(NewEntityTest, AddImageReference) {
    // First add image definition
    DL_ImageDefData imageDefData;
    imageDefData.ref = "IMG001";
    imageDefData.file = "test_image.png";
    creationInterface->linkImage(imageDefData);
    
    // Then add image reference
    DL_ImageData imageData;
    imageData.ref = "IMG001";
    imageData.ipx = 100.0;
    imageData.ipy = 200.0;
    imageData.ipz = 0.0;
    imageData.ux = 100.0;
    imageData.uy = 0.0;
    imageData.uz = 0.0;
    imageData.vx = 0.0;
    imageData.vy = 100.0;
    imageData.vz = 0.0;
    imageData.width = 100.0;
    imageData.height = 100.0;
    imageData.brightness = 50;
    imageData.contrast = 50;
    imageData.fade = 0;
    
    creationInterface->addImage(imageData);
    
    const auto& images = creationInterface->getImages();
    ASSERT_EQ(images.size(), 1);
    EXPECT_EQ(images[0].imageDefHandle, "IMG001");
    EXPECT_DOUBLE_EQ(images[0].ipx, 100.0);
    EXPECT_DOUBLE_EQ(images[0].width, 100.0);
}

// Memory management tests
TEST_F(NewEntityTest, MemoryPreallocation) {
    size_t initialMemory = creationInterface->getEstimatedMemoryUsage();
    EXPECT_GT(initialMemory, 0);
    
    // Add many entities
    for (int i = 0; i < 1000; ++i) {
        DL_LineData lineData;
        lineData.x1 = i;
        lineData.y1 = i;
        lineData.x2 = i + 1;
        lineData.y2 = i + 1;
        creationInterface->addLine(lineData);
    }
    
    size_t afterMemory = creationInterface->getEstimatedMemoryUsage();
    EXPECT_GT(afterMemory, initialMemory);
}

TEST_F(NewEntityTest, EntityStatistics) {
    // Add various entities
    DL_LineData lineData;
    creationInterface->addLine(lineData);
    
    DL_CircleData circleData;
    creationInterface->addCircle(circleData);
    
    DL_BlockData blockData;
    blockData.name = "TestBlock";
    creationInterface->addBlock(blockData);
    
    auto stats = creationInterface->getEntityStats();
    EXPECT_EQ(stats["lines"], 1);
    EXPECT_EQ(stats["circles"], 1);
    EXPECT_EQ(stats["blocks"], 1);
}

// Batch processing tests
TEST_F(NewEntityTest, BatchIndexBuilding) {
    // Add multiple blocks
    for (int i = 0; i < 100; ++i) {
        DL_BlockData blockData;
        blockData.name = "Block" + std::to_string(i);
        creationInterface->addBlock(blockData);
    }
    
    // Build indexes
    creationInterface->buildIndexes();
    
    // Verify all blocks are indexed
    const auto& blocks = creationInterface->getBlocks();
    EXPECT_EQ(blocks.size(), 100);
}

TEST_F(NewEntityTest, BatchLineProcessing) {
    std::vector<std::tuple<double, double, double, double, int>> lineData;
    for (int i = 0; i < 1000; ++i) {
        lineData.emplace_back(i, i, i + 1, i + 1, 256);
    }
    
    creationInterface->processBatchedLines(lineData);
    
    const auto& lines = creationInterface->getLines();
    EXPECT_EQ(lines.size(), 1000);
}

// Error handling tests
TEST_F(NewEntityTest, ParseErrorCollection) {
    // Trigger various errors
    DL_InsertData invalidInsert;
    invalidInsert.name = "NonExistent";
    creationInterface->addInsert(invalidInsert);
    
    DL_BlockData block1;
    block1.name = "Duplicate";
    creationInterface->addBlock(block1);
    creationInterface->addBlock(block1); // Duplicate
    
    const auto& errors = creationInterface->getParseErrors();
    EXPECT_GE(errors.size(), 2);
}

// Main function for running tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}