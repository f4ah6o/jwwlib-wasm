#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace jwwlib {
namespace pbt {
namespace storage {

// Forward declarations
class CompressionStrategy;
class SerializationStrategy;

struct CounterexampleEntry {
    std::string test_name;
    std::string property_name;
    std::vector<uint8_t> compressed_data;
    std::string error_message;
    std::chrono::system_clock::time_point timestamp;
    size_t original_size;
    size_t compressed_size;
    std::string compression_algorithm;
    
    // Metadata
    std::unordered_map<std::string, std::string> metadata;
};

struct DatabaseStats {
    size_t total_entries;
    size_t total_original_size;
    size_t total_compressed_size;
    double average_compression_ratio;
    std::chrono::system_clock::time_point oldest_entry;
    std::chrono::system_clock::time_point newest_entry;
    std::unordered_map<std::string, size_t> entries_by_test;
    std::unordered_map<std::string, size_t> entries_by_compression;
};

class CounterexampleDatabase {
public:
    struct Config {
        std::filesystem::path database_path{"pbt_counterexamples.db"};
        size_t max_database_size_mb = 100;
        size_t max_entries_per_test = 10;
        bool auto_cleanup = true;
        std::chrono::hours retention_period{24 * 30};  // 30 days
        bool enable_compression = true;
        std::string default_compression = "zstd";
    };

    explicit CounterexampleDatabase(const Config& config = Config{});
    ~CounterexampleDatabase();

    // Store a counterexample
    void store(const std::string& test_name,
               const std::string& property_name,
               const std::string& counterexample_data,
               const std::string& error_message,
               const std::unordered_map<std::string, std::string>& metadata = {});

    // Store with custom serialization
    template <typename T>
    void store_typed(const std::string& test_name,
                     const std::string& property_name,
                     const T& counterexample,
                     const std::string& error_message,
                     const std::unordered_map<std::string, std::string>& metadata = {}) {
        std::string serialized = serialize(counterexample);
        store(test_name, property_name, serialized, error_message, metadata);
    }

    // Retrieve counterexamples
    std::vector<CounterexampleEntry> get_by_test(const std::string& test_name) const;
    std::vector<CounterexampleEntry> get_by_property(const std::string& property_name) const;
    std::optional<CounterexampleEntry> get_latest(const std::string& test_name) const;
    std::vector<CounterexampleEntry> get_all() const;

    // Decompress and retrieve
    std::optional<std::string> decompress(const CounterexampleEntry& entry) const;
    
    template <typename T>
    std::optional<T> decompress_typed(const CounterexampleEntry& entry) const {
        auto decompressed = decompress(entry);
        if (!decompressed) return std::nullopt;
        
        try {
            return deserialize<T>(*decompressed);
        } catch (...) {
            return std::nullopt;
        }
    }

    // Database management
    void clear();
    void clear_test(const std::string& test_name);
    void vacuum();  // Clean up and optimize storage
    DatabaseStats get_stats() const;
    
    // Export/Import
    void export_to_file(const std::filesystem::path& path) const;
    void import_from_file(const std::filesystem::path& path);
    
    // Compression strategies
    void register_compression(const std::string& name, 
                            std::shared_ptr<CompressionStrategy> strategy);
    void set_default_compression(const std::string& name);

private:
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<CompressionStrategy>> 
        compression_strategies_;
    std::shared_ptr<SerializationStrategy> serialization_strategy_;
    
    struct DatabaseImpl;
    std::unique_ptr<DatabaseImpl> impl_;
    
    void initialize_database();
    void cleanup_old_entries();
    void enforce_size_limits();
    std::vector<uint8_t> compress_data(const std::string& data, 
                                      const std::string& algorithm) const;
    std::optional<std::string> decompress_data(const std::vector<uint8_t>& data,
                                              const std::string& algorithm) const;
    
    template <typename T>
    std::string serialize(const T& value) const;
    
    template <typename T>
    T deserialize(const std::string& data) const;
};

// Compression strategies

class CompressionStrategy {
public:
    virtual ~CompressionStrategy() = default;
    
    virtual std::vector<uint8_t> compress(const std::string& data) = 0;
    virtual std::optional<std::string> decompress(const std::vector<uint8_t>& compressed) = 0;
    virtual std::string name() const = 0;
    virtual double expected_ratio() const = 0;  // Expected compression ratio
};

// ZSTD compression (best for general data)
class ZstdCompression : public CompressionStrategy {
public:
    struct Config {
        int compression_level = 3;  // 1-22, higher = better compression
        bool enable_dictionary = false;
        size_t dictionary_size = 100 * 1024;  // 100KB
    };
    
    explicit ZstdCompression(const Config& config = Config{});
    
    std::vector<uint8_t> compress(const std::string& data) override;
    std::optional<std::string> decompress(const std::vector<uint8_t>& compressed) override;
    std::string name() const override { return "zstd"; }
    double expected_ratio() const override { return 0.3; }
    
private:
    Config config_;
    std::vector<uint8_t> dictionary_;
    
    void train_dictionary(const std::vector<std::string>& samples);
};

// LZ4 compression (fast with decent ratio)
class Lz4Compression : public CompressionStrategy {
public:
    struct Config {
        bool high_compression = false;
        int acceleration = 1;  // Higher = faster but worse ratio
    };
    
    explicit Lz4Compression(const Config& config = Config{});
    
    std::vector<uint8_t> compress(const std::string& data) override;
    std::optional<std::string> decompress(const std::vector<uint8_t>& compressed) override;
    std::string name() const override { return "lz4"; }
    double expected_ratio() const override { return 0.5; }
    
private:
    Config config_;
};

// Gzip compression (standard, widely supported)
class GzipCompression : public CompressionStrategy {
public:
    struct Config {
        int compression_level = 6;  // 1-9
    };
    
    explicit GzipCompression(const Config& config = Config{});
    
    std::vector<uint8_t> compress(const std::string& data) override;
    std::optional<std::string> decompress(const std::vector<uint8_t>& compressed) override;
    std::string name() const override { return "gzip"; }
    double expected_ratio() const override { return 0.35; }
    
private:
    Config config_;
};

// No compression (for comparison or small data)
class NoCompression : public CompressionStrategy {
public:
    std::vector<uint8_t> compress(const std::string& data) override;
    std::optional<std::string> decompress(const std::vector<uint8_t>& compressed) override;
    std::string name() const override { return "none"; }
    double expected_ratio() const override { return 1.0; }
};

// Serialization strategies

class SerializationStrategy {
public:
    virtual ~SerializationStrategy() = default;
    
    virtual std::string serialize(const std::any& value) = 0;
    virtual std::any deserialize(const std::string& data, 
                                const std::type_info& type) = 0;
};

// JSON serialization (human-readable, debuggable)
class JsonSerialization : public SerializationStrategy {
public:
    std::string serialize(const std::any& value) override;
    std::any deserialize(const std::string& data, 
                        const std::type_info& type) override;
};

// Binary serialization (efficient, compact)
class BinarySerialization : public SerializationStrategy {
public:
    std::string serialize(const std::any& value) override;
    std::any deserialize(const std::string& data, 
                        const std::type_info& type) override;
};

// Utility class for managing counterexample replay

class CounterexampleReplayer {
public:
    explicit CounterexampleReplayer(std::shared_ptr<CounterexampleDatabase> db);
    
    // Replay a specific counterexample
    template <typename T, typename PropertyType>
    bool replay(const CounterexampleEntry& entry, 
                std::shared_ptr<PropertyType> property) {
        auto value = database_->decompress_typed<T>(entry);
        if (!value) return false;
        
        try {
            property->check_single(*value);
            return false;  // Property passed, counterexample no longer valid
        } catch (...) {
            return true;   // Property still fails
        }
    }
    
    // Replay all counterexamples for a test
    template <typename PropertyType>
    std::vector<CounterexampleEntry> replay_all(
        const std::string& test_name,
        std::shared_ptr<PropertyType> property) {
        
        auto entries = database_->get_by_test(test_name);
        std::vector<CounterexampleEntry> still_failing;
        
        for (const auto& entry : entries) {
            if (replay<typename PropertyType::value_type>(entry, property)) {
                still_failing.push_back(entry);
            }
        }
        
        return still_failing;
    }
    
    // Clean up fixed counterexamples
    void remove_fixed(const std::string& test_name,
                     const std::vector<CounterexampleEntry>& fixed_entries);

private:
    std::shared_ptr<CounterexampleDatabase> database_;
};

}  // namespace storage
}  // namespace pbt
}  // namespace jwwlib