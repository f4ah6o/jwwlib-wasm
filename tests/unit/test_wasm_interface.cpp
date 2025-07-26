#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "../../src/wasm/wasm_bindings.cpp" // Include the implementation directly for testing

// Test fixture for JWWDocumentWASM
class JWWDocumentWASMTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test JSEntityData structure
TEST(JSEntityDataTest, DefaultConstructor) {
    JSEntityData entity;
    EXPECT_EQ(entity.type, "");
    EXPECT_TRUE(entity.coordinates.empty());
    EXPECT_TRUE(entity.properties.empty());
    EXPECT_EQ(entity.color, 256);
    EXPECT_EQ(entity.layer, 0);
}

TEST(JSEntityDataTest, DataAssignment) {
    JSEntityData entity;
    entity.type = "LINE";
    entity.coordinates = {0.0, 0.0, 100.0, 100.0};
    entity.properties["width"] = 2.0;
    entity.color = 1;
    entity.layer = 2;

    EXPECT_EQ(entity.type, "LINE");
    EXPECT_EQ(entity.coordinates.size(), 4);
    EXPECT_DOUBLE_EQ(entity.coordinates[0], 0.0);
    EXPECT_DOUBLE_EQ(entity.coordinates[3], 100.0);
    EXPECT_EQ(entity.properties.size(), 1);
    EXPECT_DOUBLE_EQ(entity.properties["width"], 2.0);
    EXPECT_EQ(entity.color, 1);
    EXPECT_EQ(entity.layer, 2);
}

// Test JSLayerData structure
TEST(JSLayerDataTest, DefaultConstructor) {
    JSLayerData layer;
    EXPECT_EQ(layer.index, 0);
    EXPECT_EQ(layer.name, "");
    EXPECT_EQ(layer.color, 7);
    EXPECT_TRUE(layer.visible);
    EXPECT_EQ(layer.entityCount, 0);
}

TEST(JSLayerDataTest, DataAssignment) {
    JSLayerData layer;
    layer.index = 1;
    layer.name = "Layer1";
    layer.color = 3;
    layer.visible = false;
    layer.entityCount = 10;

    EXPECT_EQ(layer.index, 1);
    EXPECT_EQ(layer.name, "Layer1");
    EXPECT_EQ(layer.color, 3);
    EXPECT_FALSE(layer.visible);
    EXPECT_EQ(layer.entityCount, 10);
}

// Test JWWDocumentWASM class
TEST_F(JWWDocumentWASMTest, Constructor) {
    JWWDocumentWASM doc;
    EXPECT_FALSE(doc.hasError());
    EXPECT_EQ(doc.getEntityCount(), 0);
    EXPECT_EQ(doc.getLayerCount(), 0);
}

TEST_F(JWWDocumentWASMTest, ErrorHandling) {
    JWWDocumentWASM doc;
    
    // Test with null pointer
    bool result = doc.loadFromMemory(0, 0);
    EXPECT_FALSE(result);
    EXPECT_TRUE(doc.hasError());
    EXPECT_FALSE(doc.getLastError().empty());
}

TEST_F(JWWDocumentWASMTest, GetEntities) {
    JWWDocumentWASM doc;
    
    // Initially should be empty
    auto entities = doc.getEntities();
    EXPECT_EQ(entities.size(), 0);
}

TEST_F(JWWDocumentWASMTest, GetLayers) {
    JWWDocumentWASM doc;
    
    // Initially should have default layer after loading
    auto layers = doc.getLayers();
    EXPECT_EQ(layers.size(), 0); // No file loaded yet
}

TEST_F(JWWDocumentWASMTest, MemoryUsage) {
    JWWDocumentWASM doc;
    
    // Should report some memory usage
    size_t usage = doc.getMemoryUsage();
    EXPECT_GT(usage, 0);
}

// Test entity conversion in JWWDocumentWASM
TEST_F(JWWDocumentWASMTest, ConvertEntitiesToNewFormat) {
    // This test would require mocking the JSCreationInterface
    // For now, we just test that the method doesn't crash
    JWWDocumentWASM doc;
    
    // Create dummy data
    std::vector<uint8_t> dummyData(100, 0);
    bool result = doc.loadFromMemory(reinterpret_cast<uintptr_t>(dummyData.data()), dummyData.size());
    
    // Should fail with dummy data but not crash
    EXPECT_FALSE(result);
}

// Test JSCreationInterface
TEST(JSCreationInterfaceTest, Constructor) {
    JSCreationInterface interface;
    
    EXPECT_EQ(interface.getLines().size(), 0);
    EXPECT_EQ(interface.getCircles().size(), 0);
    EXPECT_EQ(interface.getArcs().size(), 0);
    EXPECT_EQ(interface.getTexts().size(), 0);
}

TEST(JSCreationInterfaceTest, AddLine) {
    JSCreationInterface interface;
    
    // Simulate adding a line
    DL_LineData lineData(0, 0, 100, 100);
    interface.addLine(lineData);
    
    EXPECT_EQ(interface.getLines().size(), 1);
    EXPECT_DOUBLE_EQ(interface.getLines()[0].x1, 0);
    EXPECT_DOUBLE_EQ(interface.getLines()[0].y1, 0);
    EXPECT_DOUBLE_EQ(interface.getLines()[0].x2, 100);
    EXPECT_DOUBLE_EQ(interface.getLines()[0].y2, 100);
}

TEST(JSCreationInterfaceTest, AddCircle) {
    JSCreationInterface interface;
    
    // Simulate adding a circle
    DL_CircleData circleData(50, 50, 25);
    interface.addCircle(circleData);
    
    EXPECT_EQ(interface.getCircles().size(), 1);
    EXPECT_DOUBLE_EQ(interface.getCircles()[0].cx, 50);
    EXPECT_DOUBLE_EQ(interface.getCircles()[0].cy, 50);
    EXPECT_DOUBLE_EQ(interface.getCircles()[0].radius, 25);
}

TEST(JSCreationInterfaceTest, MemoryEstimation) {
    JSCreationInterface interface;
    
    // Add some entities
    for (int i = 0; i < 100; ++i) {
        DL_LineData lineData(0, 0, i, i);
        interface.addLine(lineData);
    }
    
    size_t memUsage = interface.getEstimatedMemoryUsage();
    EXPECT_GT(memUsage, 0);
}

TEST(JSCreationInterfaceTest, Clear) {
    JSCreationInterface interface;
    
    // Add some data
    DL_LineData lineData(0, 0, 100, 100);
    interface.addLine(lineData);
    DL_CircleData circleData(50, 50, 25);
    interface.addCircle(circleData);
    
    EXPECT_GT(interface.getLines().size(), 0);
    EXPECT_GT(interface.getCircles().size(), 0);
    
    // Clear all data
    interface.clear();
    
    EXPECT_EQ(interface.getLines().size(), 0);
    EXPECT_EQ(interface.getCircles().size(), 0);
}

// Main function for running tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}