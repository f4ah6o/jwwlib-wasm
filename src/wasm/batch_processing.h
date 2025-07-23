// Batch processing optimization for jwwlib-wasm
// This file contains batch processing implementations for improved performance

#ifndef BATCH_PROCESSING_H
#define BATCH_PROCESSING_H

#include <vector>
#include <functional>
#include <algorithm>
#include <thread>
#include <future>
#include <map>
#include <unordered_map>

// Batch processor for entity operations
template<typename T>
class BatchProcessor {
private:
    size_t batchSize;
    bool enableParallel;
    
public:
    BatchProcessor(size_t batch = 1000, bool parallel = false) 
        : batchSize(batch), enableParallel(parallel) {}
    
    // Process entities in batches
    template<typename Func>
    void processBatch(std::vector<T>& entities, Func processor) {
        size_t totalSize = entities.size();
        
        if (totalSize <= batchSize || !enableParallel) {
            // Process sequentially
            for (auto& entity : entities) {
                processor(entity);
            }
            return;
        }
        
        // Process in parallel batches
        size_t numBatches = (totalSize + batchSize - 1) / batchSize;
        std::vector<std::future<void>> futures;
        
        for (size_t i = 0; i < numBatches; ++i) {
            size_t start = i * batchSize;
            size_t end = std::min(start + batchSize, totalSize);
            
            futures.push_back(std::async(std::launch::async, [&, start, end]() {
                for (size_t j = start; j < end; ++j) {
                    processor(entities[j]);
                }
            }));
        }
        
        // Wait for all batches to complete
        for (auto& future : futures) {
            future.wait();
        }
    }
    
    // Batch transformation with result collection
    template<typename InputType, typename OutputType, typename TransformFunc>
    std::vector<OutputType> transformBatch(
        const std::vector<InputType>& input, 
        TransformFunc transformer
    ) {
        std::vector<OutputType> output;
        output.reserve(input.size());
        
        size_t totalSize = input.size();
        
        for (size_t i = 0; i < totalSize; i += batchSize) {
            size_t end = std::min(i + batchSize, totalSize);
            
            // Process batch
            for (size_t j = i; j < end; ++j) {
                output.push_back(transformer(input[j]));
            }
            
            // Optional: yield to other operations
            if (i + batchSize < totalSize) {
                std::this_thread::yield();
            }
        }
        
        return output;
    }
};

// Optimized entity converter with batching
class BatchedEntityConverter {
private:
    BatchProcessor<void*> processor;
    
public:
    BatchedEntityConverter() : processor(500, false) {} // WebAssembly doesn't support threads
    
    // Convert entities in batches to minimize memory allocation
    template<typename FromType, typename ToType>
    std::vector<ToType> convertEntities(
        const std::vector<FromType>& source,
        std::function<ToType(const FromType&)> converter
    ) {
        return processor.transformBatch<FromType, ToType>(source, converter);
    }
    
    // Process large entity arrays with progress callback
    template<typename T>
    void processWithProgress(
        std::vector<T>& entities,
        std::function<void(T&)> processor,
        std::function<void(size_t, size_t)> progressCallback = nullptr
    ) {
        size_t total = entities.size();
        size_t processed = 0;
        const size_t batchSize = 100;
        
        for (size_t i = 0; i < total; i += batchSize) {
            size_t end = std::min(i + batchSize, total);
            
            for (size_t j = i; j < end; ++j) {
                processor(entities[j]);
                processed++;
            }
            
            if (progressCallback && (processed % 1000 == 0 || processed == total)) {
                progressCallback(processed, total);
            }
        }
    }
};

// Index builder with batch optimization
template<typename KeyType, typename ValueType>
class BatchedIndexBuilder {
private:
    std::unordered_map<KeyType, std::vector<ValueType>> index;
    size_t batchSize;
    
public:
    BatchedIndexBuilder(size_t batch = 1000) : batchSize(batch) {}
    
    // Build index in batches
    void buildIndex(
        const std::vector<std::pair<KeyType, ValueType>>& items
    ) {
        // Pre-allocate buckets
        index.reserve(items.size() / 10); // Estimate
        
        // Process in batches
        for (size_t i = 0; i < items.size(); i += batchSize) {
            size_t end = std::min(i + batchSize, items.size());
            
            for (size_t j = i; j < end; ++j) {
                const auto& item = items[j];
                index[item.first].push_back(item.second);
            }
        }
    }
    
    // Get items by key
    const std::vector<ValueType>* getItems(const KeyType& key) const {
        auto it = index.find(key);
        return (it != index.end()) ? &it->second : nullptr;
    }
    
    // Clear index
    void clear() {
        index.clear();
    }
};

// Progressive loader for large files
class ProgressiveEntityLoader {
private:
    size_t chunkSize;
    std::function<void(size_t, size_t)> progressCallback;
    
public:
    ProgressiveEntityLoader(size_t chunk = 10000) : chunkSize(chunk) {}
    
    void setProgressCallback(std::function<void(size_t, size_t)> callback) {
        progressCallback = callback;
    }
    
    // Load entities progressively
    template<typename EntityType>
    bool loadEntities(
        const uint8_t* data,
        size_t dataSize,
        std::vector<EntityType>& entities,
        std::function<bool(const uint8_t*, size_t, EntityType&)> parser
    ) {
        size_t offset = 0;
        size_t entitiesLoaded = 0;
        
        while (offset < dataSize) {
            EntityType entity;
            size_t consumed = 0;
            
            if (parser(data + offset, dataSize - offset, entity)) {
                entities.push_back(std::move(entity));
                entitiesLoaded++;
                
                // Report progress
                if (progressCallback && entitiesLoaded % chunkSize == 0) {
                    progressCallback(offset, dataSize);
                }
            } else {
                break; // Parsing error
            }
            
            offset += consumed;
        }
        
        // Final progress report
        if (progressCallback) {
            progressCallback(dataSize, dataSize);
        }
        
        return offset == dataSize;
    }
};

// Batch operations for JSCreationInterface
class BatchedJSOperations {
public:
    // Batch add lines
    template<typename LineContainer>
    static void addLinesBatch(
        LineContainer& container,
        const std::vector<std::tuple<double, double, double, double, int>>& lineData
    ) {
        container.reserve(container.size() + lineData.size());
        
        for (const auto& data : lineData) {
            container.push_back({
                std::get<0>(data), // x1
                std::get<1>(data), // y1
                std::get<2>(data), // x2
                std::get<3>(data), // y2
                std::get<4>(data)  // color
            });
        }
    }
    
    // Batch entity counting
    static std::map<std::string, size_t> countEntitiesByType(
        const std::vector<std::pair<std::string, size_t>>& typeCounts
    ) {
        std::map<std::string, size_t> result;
        
        for (const auto& pair : typeCounts) {
            result[pair.first] += pair.second;
        }
        
        return result;
    }
    
    // Batch memory estimation
    static size_t estimateMemoryBatch(
        const std::vector<std::pair<size_t, size_t>>& capacitySizePairs
    ) {
        size_t total = 0;
        for (const auto& pair : capacitySizePairs) {
            total += pair.first * pair.second;
        }
        return total;
    }
};

#endif // BATCH_PROCESSING_H