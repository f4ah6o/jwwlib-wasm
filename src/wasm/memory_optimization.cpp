// Memory optimization for jwwlib-wasm
// This file contains optimized memory management implementations

#ifndef MEMORY_OPTIMIZATION_H
#define MEMORY_OPTIMIZATION_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

// Unified entity structure to reduce memory duplication
struct UnifiedEntity {
    enum Type {
        NONE = 0,
        LINE, CIRCLE, ARC, TEXT, ELLIPSE, POINT,
        POLYLINE, SOLID, MTEXT, DIMENSION, SPLINE,
        INSERT, HATCH, LEADER, IMAGE
    };
    
    Type type = NONE;
    int color = 256;  // Default BYLAYER
    
    // Common geometric data
    struct GeometryData {
        double coords[12];  // Flexible array for various coordinate needs
        double params[8];   // Additional parameters (angles, scales, etc.)
        std::string text;   // For text entities
        std::vector<double> extData;  // Extended data for complex entities
    } geometry;
    
    // Constructor
    UnifiedEntity(Type t = NONE) : type(t) {}
    
    // Type-specific accessors
    double getX1() const { return geometry.coords[0]; }
    double getY1() const { return geometry.coords[1]; }
    double getX2() const { return geometry.coords[2]; }
    double getY2() const { return geometry.coords[3]; }
    double getCX() const { return geometry.coords[0]; }
    double getCY() const { return geometry.coords[1]; }
    double getRadius() const { return geometry.params[0]; }
    double getAngle1() const { return geometry.params[1]; }
    double getAngle2() const { return geometry.params[2]; }
};

// Memory pool for efficient allocation
template<typename T>
class MemoryPool {
private:
    struct Block {
        static constexpr size_t BLOCK_SIZE = 1024;
        std::vector<T> data;
        size_t used = 0;
        
        Block() : data(BLOCK_SIZE) {}
        
        T* allocate() {
            if (used < BLOCK_SIZE) {
                return &data[used++];
            }
            return nullptr;
        }
        
        void reset() { used = 0; }
    };
    
    std::vector<std::unique_ptr<Block>> blocks;
    size_t currentBlock = 0;
    
public:
    T* allocate() {
        // Try current block
        if (currentBlock < blocks.size()) {
            T* ptr = blocks[currentBlock]->allocate();
            if (ptr) return ptr;
        }
        
        // Need new block
        blocks.push_back(std::make_unique<Block>());
        currentBlock = blocks.size() - 1;
        return blocks[currentBlock]->allocate();
    }
    
    void clear() {
        for (auto& block : blocks) {
            block->reset();
        }
        currentBlock = 0;
    }
    
    size_t getMemoryUsage() const {
        return blocks.size() * sizeof(T) * Block::BLOCK_SIZE;
    }
};

// String interning for reducing string duplication
class StringPool {
private:
    std::unordered_map<std::string, std::shared_ptr<std::string>> pool;
    
public:
    std::shared_ptr<std::string> intern(const std::string& str) {
        auto it = pool.find(str);
        if (it != pool.end()) {
            return it->second;
        }
        
        auto ptr = std::make_shared<std::string>(str);
        pool[str] = ptr;
        return ptr;
    }
    
    void clear() {
        pool.clear();
    }
    
    size_t size() const {
        return pool.size();
    }
};

// Optimized entity storage with capacity management
class OptimizedEntityStorage {
private:
    std::vector<UnifiedEntity> entities;
    MemoryPool<UnifiedEntity> pool;
    StringPool stringPool;
    
    // Statistics
    struct Stats {
        size_t totalEntities = 0;
        size_t memoryUsed = 0;
        std::unordered_map<UnifiedEntity::Type, size_t> typeCount;
    } stats;
    
public:
    // Reserve capacity based on estimated entity count
    void reserve(size_t estimatedCount) {
        entities.reserve(estimatedCount);
    }
    
    // Add entity with memory optimization
    UnifiedEntity* addEntity(UnifiedEntity::Type type) {
        UnifiedEntity* entity = pool.allocate();
        entity->type = type;
        stats.totalEntities++;
        stats.typeCount[type]++;
        return entity;
    }
    
    // Batch add for efficiency
    void addEntitiesBatch(const std::vector<UnifiedEntity>& batch) {
        entities.reserve(entities.size() + batch.size());
        entities.insert(entities.end(), batch.begin(), batch.end());
        stats.totalEntities += batch.size();
    }
    
    // Get entities by type (avoid full copy)
    std::vector<const UnifiedEntity*> getEntitiesByType(UnifiedEntity::Type type) const {
        std::vector<const UnifiedEntity*> result;
        for (const auto& entity : entities) {
            if (entity.type == type) {
                result.push_back(&entity);
            }
        }
        return result;
    }
    
    // Memory management
    void clear() {
        entities.clear();
        pool.clear();
        stringPool.clear();
        stats = Stats();
    }
    
    size_t getMemoryUsage() const {
        return entities.capacity() * sizeof(UnifiedEntity) + 
               pool.getMemoryUsage() +
               stringPool.size() * 64;  // Estimate
    }
    
    const Stats& getStats() const { return stats; }
};

// Block definition cache to prevent duplication
class BlockDefinitionCache {
private:
    struct BlockDef {
        std::string name;
        double baseX, baseY, baseZ;
        std::vector<UnifiedEntity> entities;
    };
    
    std::unordered_map<std::string, std::unique_ptr<BlockDef>> blocks;
    
public:
    // Add or update block definition
    bool addBlock(const std::string& name, double x, double y, double z) {
        auto it = blocks.find(name);
        if (it != blocks.end()) {
            // Block already exists
            return false;
        }
        
        auto block = std::make_unique<BlockDef>();
        block->name = name;
        block->baseX = x;
        block->baseY = y;
        block->baseZ = z;
        blocks[name] = std::move(block);
        return true;
    }
    
    // Get block definition
    const BlockDef* getBlock(const std::string& name) const {
        auto it = blocks.find(name);
        return (it != blocks.end()) ? it->second.get() : nullptr;
    }
    
    // Clear all blocks
    void clear() {
        blocks.clear();
    }
    
    size_t size() const {
        return blocks.size();
    }
};

// Memory-mapped file reader for large files (future enhancement)
class MemoryMappedReader {
    // TODO: Implement memory-mapped file reading for very large JWW files
    // This would avoid loading the entire file into memory
};

#endif // MEMORY_OPTIMIZATION_H