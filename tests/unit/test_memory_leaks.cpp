// Memory leak tests for jwwlib-wasm
// Tests to ensure proper memory management and no leaks

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <thread>
#include "../../src/wasm/wasm_bindings.cpp"

// Test fixture for memory leak tests
class MemoryLeakTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Memory leak detection will be handled by tools like Valgrind or ASan
    }
    
    void TearDown() override {
        // Cleanup
    }
};

// Test repeated creation and destruction
TEST_F(MemoryLeakTest, RepeatedCreationDestruction) {
    for (int i = 0; i < 100; ++i) {
        auto creationInterface = std::make_unique<JSCreationInterface>();
        
        // Add some entities
        DL_LineData lineData;
        lineData.x1 = 0;
        lineData.y1 = 0;
        lineData.x2 = 100;
        lineData.y2 = 100;
        creationInterface->addLine(lineData);
        
        DL_BlockData blockData;
        blockData.name = "TestBlock" + std::to_string(i);
        creationInterface->addBlock(blockData);
        
        // Interface will be destroyed at end of scope
    }
    
    // If we reach here without memory issues, test passes
    SUCCEED();
}

// Test large entity arrays
TEST_F(MemoryLeakTest, LargeEntityArrays) {
    auto creationInterface = std::make_unique<JSCreationInterface>(10000000); // 10MB file hint
    
    // Add 10000 lines
    for (int i = 0; i < 10000; ++i) {
        DL_LineData lineData;
        lineData.x1 = i;
        lineData.y1 = i;
        lineData.x2 = i + 1;
        lineData.y2 = i + 1;
        creationInterface->addLine(lineData);
    }
    
    // Add 1000 polylines with 100 vertices each
    for (int i = 0; i < 1000; ++i) {
        DL_PolylineData polylineData;
        polylineData.number = 100;
        polylineData.flags = 0;
        creationInterface->addPolyline(polylineData);
        
        for (int j = 0; j < 100; ++j) {
            DL_VertexData vertex;
            vertex.x = j;
            vertex.y = j;
            vertex.z = 0;
            vertex.bulge = 0;
            creationInterface->addVertex(vertex);
        }
    }
    
    // Clear all data
    creationInterface->clear();
    
    // Memory should be properly freed
    SUCCEED();
}

// Test string memory management
TEST_F(MemoryLeakTest, StringMemoryManagement) {
    auto creationInterface = std::make_unique<JSCreationInterface>();
    
    // Add many text entities with long strings
    for (int i = 0; i < 1000; ++i) {
        DL_TextData textData;
        textData.x = i;
        textData.y = i;
        textData.height = 10;
        textData.angle = 0;
        textData.text = std::string(1000, 'A' + (i % 26)); // 1000 char strings
        
        creationInterface->addText(textData);
    }
    
    // Add blocks with long names
    for (int i = 0; i < 100; ++i) {
        DL_BlockData blockData;
        blockData.name = "VeryLongBlockName_" + std::string(100, 'X') + std::to_string(i);
        creationInterface->addBlock(blockData);
    }
    
    SUCCEED();
}

// Test error handling memory
TEST_F(MemoryLeakTest, ErrorHandlingMemory) {
    auto creationInterface = std::make_unique<JSCreationInterface>();
    
    // Generate many errors
    for (int i = 0; i < 1000; ++i) {
        // Invalid block references
        DL_InsertData insertData;
        insertData.name = "NonExistentBlock" + std::to_string(i);
        creationInterface->addInsert(insertData);
    }
    
    // Check errors were collected
    const auto& errors = creationInterface->getParseErrors();
    EXPECT_EQ(errors.size(), 1000);
    
    // Clear should free error memory
    creationInterface->clear();
    SUCCEED();
}

// Test concurrent access (if applicable)
TEST_F(MemoryLeakTest, ConcurrentAccess) {
    auto creationInterface = std::make_unique<JSCreationInterface>();
    
    // Note: DL_Jww is not thread-safe, so we test sequential batch processing
    std::vector<std::tuple<double, double, double, double, int>> lineData;
    
    // Prepare batch data
    for (int i = 0; i < 10000; ++i) {
        lineData.emplace_back(i, i, i + 1, i + 1, 256);
    }
    
    // Process in batches
    creationInterface->processBatchedLines(lineData);
    
    const auto& lines = creationInterface->getLines();
    EXPECT_EQ(lines.size(), 10000);
    
    SUCCEED();
}

// Test memory usage reporting
TEST_F(MemoryLeakTest, MemoryUsageReporting) {
    auto creationInterface = std::make_unique<JSCreationInterface>();
    
    size_t initialMemory = creationInterface->getEstimatedMemoryUsage();
    EXPECT_GT(initialMemory, 0);
    
    // Add entities and check memory growth
    for (int i = 0; i < 1000; ++i) {
        DL_CircleData circleData;
        circleData.cx = i;
        circleData.cy = i;
        circleData.radius = 10;
        creationInterface->addCircle(circleData);
    }
    
    size_t afterMemory = creationInterface->getEstimatedMemoryUsage();
    EXPECT_GT(afterMemory, initialMemory);
    
    // Clear and check memory reduction
    creationInterface->clear();
    
    // Note: capacity may still be reserved, but data should be cleared
    const auto& circles = creationInterface->getCircles();
    EXPECT_EQ(circles.size(), 0);
    
    SUCCEED();
}

// Test batch index builder memory
TEST_F(MemoryLeakTest, BatchIndexBuilderMemory) {
    auto creationInterface = std::make_unique<JSCreationInterface>();
    
    // Add many blocks to test index building
    for (int i = 0; i < 10000; ++i) {
        DL_BlockData blockData;
        blockData.name = "Block_" + std::to_string(i);
        blockData.bpx = i;
        blockData.bpy = i;
        blockData.bpz = 0;
        creationInterface->addBlock(blockData);
    }
    
    // Build indexes multiple times
    for (int i = 0; i < 10; ++i) {
        creationInterface->buildIndexes();
    }
    
    // Memory should not grow excessively
    SUCCEED();
}

// Test hatch entity memory with complex boundaries
TEST_F(MemoryLeakTest, ComplexHatchMemory) {
    auto creationInterface = std::make_unique<JSCreationInterface>();
    
    // Create hatches with many loops and edges
    for (int i = 0; i < 100; ++i) {
        DL_HatchData hatchData;
        hatchData.solid = false;
        hatchData.pattern = "ANSI31";
        creationInterface->addHatch(hatchData);
        
        // Add multiple loops
        for (int j = 0; j < 10; ++j) {
            DL_HatchLoopData loopData;
            loopData.numEdges = 100;
            creationInterface->addHatchLoop(loopData);
            
            // Add many edges
            for (int k = 0; k < 100; ++k) {
                DL_HatchEdgeData edgeData;
                edgeData.type = 1; // Line
                edgeData.x1 = k;
                edgeData.y1 = k;
                edgeData.x2 = k + 1;
                edgeData.y2 = k + 1;
                creationInterface->addHatchEdge(edgeData);
            }
        }
    }
    
    const auto& hatches = creationInterface->getHatches();
    EXPECT_EQ(hatches.size(), 100);
    
    // Clear should free all nested structures
    creationInterface->clear();
    SUCCEED();
}

// Main function
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}