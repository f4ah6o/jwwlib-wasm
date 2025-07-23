// Batch processing performance tests for jwwlib-wasm
// Tests batch processing optimization and performance

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <chrono>
#include <iostream>
#include "../../src/wasm/wasm_bindings.cpp"
#include "../../src/wasm/batch_processing.h"

// Test fixture for batch processing tests
class BatchProcessingTest : public ::testing::Test {
protected:
    std::unique_ptr<JSCreationInterface> creationInterface;
    
    void SetUp() override {
        creationInterface = std::make_unique<JSCreationInterface>();
    }
    
    void TearDown() override {
        creationInterface.reset();
    }
    
    // Helper to measure execution time
    template<typename Func>
    double measureTime(Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / 1000.0; // Return milliseconds
    }
};

// Test batch line processing
TEST_F(BatchProcessingTest, BatchLineProcessingPerformance) {
    const int NUM_LINES = 10000;
    
    // Test individual line addition
    double individualTime = measureTime([&]() {
        for (int i = 0; i < NUM_LINES; ++i) {
            DL_LineData lineData;
            lineData.x1 = i;
            lineData.y1 = i;
            lineData.x2 = i + 1;
            lineData.y2 = i + 1;
            creationInterface->addLine(lineData);
        }
    });
    
    creationInterface->clear();
    
    // Test batch line addition
    double batchTime = measureTime([&]() {
        std::vector<std::tuple<double, double, double, double, int>> lineData;
        lineData.reserve(NUM_LINES);
        
        for (int i = 0; i < NUM_LINES; ++i) {
            lineData.emplace_back(i, i, i + 1, i + 1, 256);
        }
        
        creationInterface->processBatchedLines(lineData);
    });
    
    std::cout << "Individual line addition: " << individualTime << " ms\n";
    std::cout << "Batch line addition: " << batchTime << " ms\n";
    std::cout << "Speedup: " << individualTime / batchTime << "x\n";
    
    // Batch should be faster (or at least not significantly slower)
    EXPECT_LE(batchTime, individualTime * 1.5);
}

// Test batch processor class
TEST_F(BatchProcessingTest, BatchProcessorFunctionality) {
    BatchProcessor<int> processor(100); // Batch size of 100
    
    std::vector<int> data;
    for (int i = 0; i < 1000; ++i) {
        data.push_back(i);
    }
    
    int sum = 0;
    processor.processBatch(data, [&sum](int value) {
        sum += value;
    });
    
    // Verify all items were processed
    int expectedSum = (999 * 1000) / 2;
    EXPECT_EQ(sum, expectedSum);
}

// Test batch transformation
TEST_F(BatchProcessingTest, BatchTransformation) {
    BatchProcessor<int> processor(50);
    
    std::vector<int> input;
    for (int i = 0; i < 200; ++i) {
        input.push_back(i);
    }
    
    auto output = processor.transformBatch<int, int>(input, [](int value) {
        return value * 2;
    });
    
    ASSERT_EQ(output.size(), input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_EQ(output[i], input[i] * 2);
    }
}

// Test batched entity converter
TEST_F(BatchProcessingTest, BatchedEntityConverter) {
    BatchedEntityConverter converter;
    
    // Create test data
    struct SimpleEntity {
        double x, y;
    };
    
    std::vector<SimpleEntity> entities;
    for (int i = 0; i < 1000; ++i) {
        entities.push_back({static_cast<double>(i), static_cast<double>(i * 2)});
    }
    
    // Convert entities
    auto converted = converter.convertEntities<SimpleEntity, std::pair<double, double>>(
        entities,
        [](const SimpleEntity& e) { return std::make_pair(e.x, e.y); }
    );
    
    ASSERT_EQ(converted.size(), entities.size());
    for (size_t i = 0; i < entities.size(); ++i) {
        EXPECT_EQ(converted[i].first, entities[i].x);
        EXPECT_EQ(converted[i].second, entities[i].y);
    }
}

// Test batched index builder
TEST_F(BatchProcessingTest, BatchedIndexBuilder) {
    BatchedIndexBuilder<std::string, int> indexBuilder(100);
    
    std::vector<std::pair<std::string, int>> items;
    for (int i = 0; i < 1000; ++i) {
        items.emplace_back("key" + std::to_string(i % 100), i);
    }
    
    double buildTime = measureTime([&]() {
        indexBuilder.buildIndex(items);
    });
    
    std::cout << "Index build time for 1000 items: " << buildTime << " ms\n";
    
    // Test retrieval
    const auto* values = indexBuilder.getItems("key50");
    ASSERT_NE(values, nullptr);
    EXPECT_EQ(values->size(), 10); // Should have 10 items with key50
}

// Test progress callback
TEST_F(BatchProcessingTest, ProgressCallback) {
    BatchedEntityConverter converter;
    
    std::vector<int> entities(10000);
    size_t lastProgress = 0;
    int callbackCount = 0;
    
    converter.processWithProgress<int>(
        entities,
        [](int& value) { value = 42; },
        [&](size_t current, size_t total) {
            EXPECT_GE(current, lastProgress);
            EXPECT_LE(current, total);
            lastProgress = current;
            callbackCount++;
        }
    );
    
    EXPECT_GT(callbackCount, 0);
    EXPECT_EQ(lastProgress, entities.size());
}

// Test memory pre-allocation performance
TEST_F(BatchProcessingTest, MemoryPreallocationPerformance) {
    const int NUM_ENTITIES = 50000;
    
    // Without pre-allocation
    auto interface1 = std::make_unique<JSCreationInterface>();
    double withoutPrealloc = measureTime([&]() {
        for (int i = 0; i < NUM_ENTITIES; ++i) {
            DL_LineData lineData;
            lineData.x1 = i;
            interface1->addLine(lineData);
        }
    });
    
    // With pre-allocation
    auto interface2 = std::make_unique<JSCreationInterface>(NUM_ENTITIES * 100);
    double withPrealloc = measureTime([&]() {
        for (int i = 0; i < NUM_ENTITIES; ++i) {
            DL_LineData lineData;
            lineData.x1 = i;
            interface2->addLine(lineData);
        }
    });
    
    std::cout << "Without pre-allocation: " << withoutPrealloc << " ms\n";
    std::cout << "With pre-allocation: " << withPrealloc << " ms\n";
    std::cout << "Improvement: " << (withoutPrealloc - withPrealloc) / withoutPrealloc * 100 << "%\n";
    
    // Pre-allocation should be faster
    EXPECT_LT(withPrealloc, withoutPrealloc);
}

// Test entity statistics batch counting
TEST_F(BatchProcessingTest, EntityStatisticsBatchCounting) {
    // Add various entities
    for (int i = 0; i < 1000; ++i) {
        DL_LineData lineData;
        creationInterface->addLine(lineData);
    }
    
    for (int i = 0; i < 500; ++i) {
        DL_CircleData circleData;
        creationInterface->addCircle(circleData);
    }
    
    for (int i = 0; i < 100; ++i) {
        DL_BlockData blockData;
        blockData.name = "Block" + std::to_string(i);
        creationInterface->addBlock(blockData);
    }
    
    double statsTime = measureTime([&]() {
        auto stats = creationInterface->getEntityStats();
        EXPECT_EQ(stats["lines"], 1000);
        EXPECT_EQ(stats["circles"], 500);
        EXPECT_EQ(stats["blocks"], 100);
    });
    
    std::cout << "Entity statistics calculation time: " << statsTime << " ms\n";
    EXPECT_LT(statsTime, 10.0); // Should be very fast
}

// Test batch operations class
TEST_F(BatchProcessingTest, BatchedJSOperations) {
    // Test batch entity counting
    std::vector<std::pair<std::string, size_t>> typeCounts = {
        {"lines", 1000},
        {"circles", 500},
        {"lines", 2000}, // Duplicate key to test aggregation
        {"arcs", 300}
    };
    
    auto result = BatchedJSOperations::countEntitiesByType(typeCounts);
    
    EXPECT_EQ(result["lines"], 3000);
    EXPECT_EQ(result["circles"], 500);
    EXPECT_EQ(result["arcs"], 300);
    
    // Test memory estimation
    std::vector<std::pair<size_t, size_t>> capacitySizes = {
        {1000, sizeof(JSLineData)},
        {500, sizeof(JSCircleData)},
        {300, sizeof(JSArcData)}
    };
    
    size_t estimatedMemory = BatchedJSOperations::estimateMemoryBatch(capacitySizes);
    size_t expectedMemory = 1000 * sizeof(JSLineData) + 
                           500 * sizeof(JSCircleData) + 
                           300 * sizeof(JSArcData);
    EXPECT_EQ(estimatedMemory, expectedMemory);
}

// Test large file handling
TEST_F(BatchProcessingTest, LargeFileHandling) {
    const size_t FILE_SIZE = 100 * 1024 * 1024; // 100MB
    auto largeInterface = std::make_unique<JSCreationInterface>(FILE_SIZE);
    
    // Simulate large file processing
    double processTime = measureTime([&]() {
        // Add blocks
        for (int i = 0; i < 1000; ++i) {
            DL_BlockData blockData;
            blockData.name = "LargeBlock" + std::to_string(i);
            largeInterface->addBlock(blockData);
        }
        
        // Build indexes
        largeInterface->buildIndexes();
        
        // Add inserts referencing blocks
        for (int i = 0; i < 10000; ++i) {
            DL_InsertData insertData;
            insertData.name = "LargeBlock" + std::to_string(i % 1000);
            largeInterface->addInsert(insertData);
        }
    });
    
    std::cout << "Large file processing time: " << processTime << " ms\n";
    
    auto stats = largeInterface->getEntityStats();
    EXPECT_EQ(stats["blocks"], 1000);
    EXPECT_EQ(stats["inserts"], 10000);
}

// Main function
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}