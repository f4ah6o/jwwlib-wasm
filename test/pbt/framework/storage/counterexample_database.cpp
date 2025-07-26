#include "counterexample_database.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// Compression library includes (these would need to be added to CMakeLists.txt)
// For this implementation, we'll provide a simplified version
#include <zlib.h>  // For gzip compression

namespace jwwlib {
namespace pbt {
namespace storage {

// Simple in-memory database implementation
struct CounterexampleDatabase::DatabaseImpl {
    std::vector<CounterexampleEntry> entries;
    
    void add_entry(CounterexampleEntry entry) {
        entries.push_back(std::move(entry));
    }
    
    std::vector<CounterexampleEntry> query_by_test(const std::string& test_name) const {
        std::vector<CounterexampleEntry> result;
        std::copy_if(entries.begin(), entries.end(), std::back_inserter(result),
                    [&test_name](const CounterexampleEntry& entry) {
                        return entry.test_name == test_name;
                    });
        return result;
    }
    
    std::vector<CounterexampleEntry> query_by_property(const std::string& property_name) const {
        std::vector<CounterexampleEntry> result;
        std::copy_if(entries.begin(), entries.end(), std::back_inserter(result),
                    [&property_name](const CounterexampleEntry& entry) {
                        return entry.property_name == property_name;
                    });
        return result;
    }
    
    void remove_old_entries(std::chrono::system_clock::time_point cutoff) {
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                         [cutoff](const CounterexampleEntry& entry) {
                             return entry.timestamp < cutoff;
                         }),
            entries.end()
        );
    }
    
    size_t total_size() const {
        size_t total = 0;
        for (const auto& entry : entries) {
            total += entry.compressed_size;
        }
        return total;
    }
};

// CounterexampleDatabase implementation
CounterexampleDatabase::CounterexampleDatabase(const Config& config)
    : config_(config), impl_(std::make_unique<DatabaseImpl>()) {
    
    // Register default compression strategies
    register_compression("none", std::make_shared<NoCompression>());
    register_compression("gzip", std::make_shared<GzipCompression>());
    
    // Set default serialization
    serialization_strategy_ = std::make_shared<JsonSerialization>();
    
    initialize_database();
}

CounterexampleDatabase::~CounterexampleDatabase() = default;

void CounterexampleDatabase::store(
    const std::string& test_name,
    const std::string& property_name,
    const std::string& counterexample_data,
    const std::string& error_message,
    const std::unordered_map<std::string, std::string>& metadata) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Compress the data
    auto compressed = compress_data(counterexample_data, config_.default_compression);
    
    CounterexampleEntry entry{
        .test_name = test_name,
        .property_name = property_name,
        .compressed_data = compressed,
        .error_message = error_message,
        .timestamp = std::chrono::system_clock::now(),
        .original_size = counterexample_data.size(),
        .compressed_size = compressed.size(),
        .compression_algorithm = config_.default_compression,
        .metadata = metadata
    };
    
    impl_->add_entry(std::move(entry));
    
    // Maintenance tasks
    if (config_.auto_cleanup) {
        cleanup_old_entries();
        enforce_size_limits();
    }
}

std::vector<CounterexampleEntry> CounterexampleDatabase::get_by_test(
    const std::string& test_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return impl_->query_by_test(test_name);
}

std::vector<CounterexampleEntry> CounterexampleDatabase::get_by_property(
    const std::string& property_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return impl_->query_by_property(property_name);
}

std::optional<CounterexampleEntry> CounterexampleDatabase::get_latest(
    const std::string& test_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto entries = impl_->query_by_test(test_name);
    if (entries.empty()) return std::nullopt;
    
    auto latest = std::max_element(entries.begin(), entries.end(),
                                  [](const CounterexampleEntry& a, 
                                     const CounterexampleEntry& b) {
                                      return a.timestamp < b.timestamp;
                                  });
    
    return *latest;
}

std::vector<CounterexampleEntry> CounterexampleDatabase::get_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return impl_->entries;
}

std::optional<std::string> CounterexampleDatabase::decompress(
    const CounterexampleEntry& entry) const {
    return decompress_data(entry.compressed_data, entry.compression_algorithm);
}

void CounterexampleDatabase::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    impl_->entries.clear();
}

void CounterexampleDatabase::clear_test(const std::string& test_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    impl_->entries.erase(
        std::remove_if(impl_->entries.begin(), impl_->entries.end(),
                      [&test_name](const CounterexampleEntry& entry) {
                          return entry.test_name == test_name;
                      }),
        impl_->entries.end()
    );
}

void CounterexampleDatabase::vacuum() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Sort entries by test name for better locality
    std::sort(impl_->entries.begin(), impl_->entries.end(),
             [](const CounterexampleEntry& a, const CounterexampleEntry& b) {
                 return a.test_name < b.test_name;
             });
    
    // Shrink to fit
    impl_->entries.shrink_to_fit();
}

DatabaseStats CounterexampleDatabase::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    DatabaseStats stats{};
    stats.total_entries = impl_->entries.size();
    
    if (impl_->entries.empty()) {
        return stats;
    }
    
    std::unordered_map<std::string, size_t> test_counts;
    std::unordered_map<std::string, size_t> compression_counts;
    
    for (const auto& entry : impl_->entries) {
        stats.total_original_size += entry.original_size;
        stats.total_compressed_size += entry.compressed_size;
        test_counts[entry.test_name]++;
        compression_counts[entry.compression_algorithm]++;
        
        if (stats.oldest_entry == std::chrono::system_clock::time_point{} ||
            entry.timestamp < stats.oldest_entry) {
            stats.oldest_entry = entry.timestamp;
        }
        
        if (entry.timestamp > stats.newest_entry) {
            stats.newest_entry = entry.timestamp;
        }
    }
    
    stats.average_compression_ratio = 
        static_cast<double>(stats.total_compressed_size) / stats.total_original_size;
    stats.entries_by_test = std::move(test_counts);
    stats.entries_by_compression = std::move(compression_counts);
    
    return stats;
}

void CounterexampleDatabase::export_to_file(const std::filesystem::path& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open export file");
    }
    
    // Write header
    const char* magic = "PBTC";
    file.write(magic, 4);
    
    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    uint32_t entry_count = static_cast<uint32_t>(impl_->entries.size());
    file.write(reinterpret_cast<const char*>(&entry_count), sizeof(entry_count));
    
    // Write entries
    for (const auto& entry : impl_->entries) {
        // Write test name
        uint32_t name_len = static_cast<uint32_t>(entry.test_name.size());
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        file.write(entry.test_name.data(), name_len);
        
        // Write property name
        uint32_t prop_len = static_cast<uint32_t>(entry.property_name.size());
        file.write(reinterpret_cast<const char*>(&prop_len), sizeof(prop_len));
        file.write(entry.property_name.data(), prop_len);
        
        // Write compressed data
        uint32_t data_len = static_cast<uint32_t>(entry.compressed_data.size());
        file.write(reinterpret_cast<const char*>(&data_len), sizeof(data_len));
        file.write(reinterpret_cast<const char*>(entry.compressed_data.data()), data_len);
        
        // Write metadata
        uint32_t error_len = static_cast<uint32_t>(entry.error_message.size());
        file.write(reinterpret_cast<const char*>(&error_len), sizeof(error_len));
        file.write(entry.error_message.data(), error_len);
        
        // Write timestamp
        auto time_since_epoch = entry.timestamp.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        file.write(reinterpret_cast<const char*>(&millis), sizeof(millis));
        
        // Write sizes
        file.write(reinterpret_cast<const char*>(&entry.original_size), sizeof(entry.original_size));
        file.write(reinterpret_cast<const char*>(&entry.compressed_size), sizeof(entry.compressed_size));
        
        // Write compression algorithm
        uint32_t algo_len = static_cast<uint32_t>(entry.compression_algorithm.size());
        file.write(reinterpret_cast<const char*>(&algo_len), sizeof(algo_len));
        file.write(entry.compression_algorithm.data(), algo_len);
    }
}

void CounterexampleDatabase::import_from_file(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open import file");
    }
    
    // Read and verify header
    char magic[4];
    file.read(magic, 4);
    if (std::strncmp(magic, "PBTC", 4) != 0) {
        throw std::runtime_error("Invalid file format");
    }
    
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) {
        throw std::runtime_error("Unsupported file version");
    }
    
    uint32_t entry_count;
    file.read(reinterpret_cast<char*>(&entry_count), sizeof(entry_count));
    
    // Read entries
    for (uint32_t i = 0; i < entry_count; ++i) {
        CounterexampleEntry entry;
        
        // Read test name
        uint32_t name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        entry.test_name.resize(name_len);
        file.read(entry.test_name.data(), name_len);
        
        // Read property name
        uint32_t prop_len;
        file.read(reinterpret_cast<char*>(&prop_len), sizeof(prop_len));
        entry.property_name.resize(prop_len);
        file.read(entry.property_name.data(), prop_len);
        
        // Read compressed data
        uint32_t data_len;
        file.read(reinterpret_cast<char*>(&data_len), sizeof(data_len));
        entry.compressed_data.resize(data_len);
        file.read(reinterpret_cast<char*>(entry.compressed_data.data()), data_len);
        
        // Read metadata
        uint32_t error_len;
        file.read(reinterpret_cast<char*>(&error_len), sizeof(error_len));
        entry.error_message.resize(error_len);
        file.read(entry.error_message.data(), error_len);
        
        // Read timestamp
        int64_t millis;
        file.read(reinterpret_cast<char*>(&millis), sizeof(millis));
        entry.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(millis));
        
        // Read sizes
        file.read(reinterpret_cast<char*>(&entry.original_size), sizeof(entry.original_size));
        file.read(reinterpret_cast<char*>(&entry.compressed_size), sizeof(entry.compressed_size));
        
        // Read compression algorithm
        uint32_t algo_len;
        file.read(reinterpret_cast<char*>(&algo_len), sizeof(algo_len));
        entry.compression_algorithm.resize(algo_len);
        file.read(entry.compression_algorithm.data(), algo_len);
        
        impl_->add_entry(std::move(entry));
    }
}

void CounterexampleDatabase::register_compression(
    const std::string& name,
    std::shared_ptr<CompressionStrategy> strategy) {
    compression_strategies_[name] = strategy;
}

void CounterexampleDatabase::set_default_compression(const std::string& name) {
    if (compression_strategies_.find(name) == compression_strategies_.end()) {
        throw std::runtime_error("Unknown compression strategy: " + name);
    }
    config_.default_compression = name;
}

void CounterexampleDatabase::initialize_database() {
    // Load existing database from disk if it exists
    if (std::filesystem::exists(config_.database_path)) {
        try {
            import_from_file(config_.database_path);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load existing database: " << e.what() << std::endl;
        }
    }
}

void CounterexampleDatabase::cleanup_old_entries() {
    auto cutoff = std::chrono::system_clock::now() - config_.retention_period;
    impl_->remove_old_entries(cutoff);
}

void CounterexampleDatabase::enforce_size_limits() {
    // Enforce per-test entry limit
    std::unordered_map<std::string, size_t> test_counts;
    for (const auto& entry : impl_->entries) {
        test_counts[entry.test_name]++;
    }
    
    for (const auto& [test_name, count] : test_counts) {
        if (count > config_.max_entries_per_test) {
            // Keep only the newest entries
            auto test_entries = impl_->query_by_test(test_name);
            std::sort(test_entries.begin(), test_entries.end(),
                     [](const CounterexampleEntry& a, const CounterexampleEntry& b) {
                         return a.timestamp > b.timestamp;
                     });
            
            // Remove old entries
            for (size_t i = config_.max_entries_per_test; i < test_entries.size(); ++i) {
                auto it = std::find_if(impl_->entries.begin(), impl_->entries.end(),
                                      [&test_entries, i](const CounterexampleEntry& e) {
                                          return e.test_name == test_entries[i].test_name &&
                                                 e.timestamp == test_entries[i].timestamp;
                                      });
                if (it != impl_->entries.end()) {
                    impl_->entries.erase(it);
                }
            }
        }
    }
    
    // Enforce total size limit
    size_t max_size_bytes = config_.max_database_size_mb * 1024 * 1024;
    while (impl_->total_size() > max_size_bytes && !impl_->entries.empty()) {
        // Remove oldest entry
        auto oldest = std::min_element(impl_->entries.begin(), impl_->entries.end(),
                                      [](const CounterexampleEntry& a,
                                         const CounterexampleEntry& b) {
                                          return a.timestamp < b.timestamp;
                                      });
        impl_->entries.erase(oldest);
    }
}

std::vector<uint8_t> CounterexampleDatabase::compress_data(
    const std::string& data,
    const std::string& algorithm) const {
    
    auto it = compression_strategies_.find(algorithm);
    if (it == compression_strategies_.end()) {
        throw std::runtime_error("Unknown compression algorithm: " + algorithm);
    }
    
    return it->second->compress(data);
}

std::optional<std::string> CounterexampleDatabase::decompress_data(
    const std::vector<uint8_t>& data,
    const std::string& algorithm) const {
    
    auto it = compression_strategies_.find(algorithm);
    if (it == compression_strategies_.end()) {
        return std::nullopt;
    }
    
    return it->second->decompress(data);
}

// Compression strategy implementations

// NoCompression
std::vector<uint8_t> NoCompression::compress(const std::string& data) {
    return std::vector<uint8_t>(data.begin(), data.end());
}

std::optional<std::string> NoCompression::decompress(const std::vector<uint8_t>& compressed) {
    return std::string(compressed.begin(), compressed.end());
}

// GzipCompression
GzipCompression::GzipCompression(const Config& config) : config_(config) {}

std::vector<uint8_t> GzipCompression::compress(const std::string& data) {
    z_stream zs{};
    
    if (deflateInit2(&zs, config_.compression_level, Z_DEFLATED,
                     15 + 16,  // windowBits + 16 for gzip encoding
                     8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("Failed to initialize gzip compression");
    }
    
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    zs.avail_in = static_cast<uInt>(data.size());
    
    std::vector<uint8_t> compressed;
    compressed.resize(deflateBound(&zs, data.size()));
    
    zs.next_out = reinterpret_cast<Bytef*>(compressed.data());
    zs.avail_out = static_cast<uInt>(compressed.size());
    
    int ret = deflate(&zs, Z_FINISH);
    deflateEnd(&zs);
    
    if (ret != Z_STREAM_END) {
        throw std::runtime_error("Gzip compression failed");
    }
    
    compressed.resize(zs.total_out);
    return compressed;
}

std::optional<std::string> GzipCompression::decompress(const std::vector<uint8_t>& compressed) {
    z_stream zs{};
    
    if (inflateInit2(&zs, 15 + 16) != Z_OK) {  // 15 + 16 for gzip decoding
        return std::nullopt;
    }
    
    zs.next_in = const_cast<Bytef*>(compressed.data());
    zs.avail_in = static_cast<uInt>(compressed.size());
    
    std::string decompressed;
    decompressed.resize(compressed.size() * 4);  // Initial guess
    
    size_t total_out = 0;
    
    while (true) {
        if (total_out == decompressed.size()) {
            decompressed.resize(decompressed.size() * 2);
        }
        
        zs.next_out = reinterpret_cast<Bytef*>(&decompressed[total_out]);
        zs.avail_out = static_cast<uInt>(decompressed.size() - total_out);
        
        int ret = inflate(&zs, Z_NO_FLUSH);
        
        if (ret == Z_STREAM_END) {
            break;
        } else if (ret != Z_OK) {
            inflateEnd(&zs);
            return std::nullopt;
        }
        
        total_out = zs.total_out;
    }
    
    inflateEnd(&zs);
    decompressed.resize(zs.total_out);
    
    return decompressed;
}

// Placeholder implementations for other compression strategies
ZstdCompression::ZstdCompression(const Config& config) : config_(config) {}

std::vector<uint8_t> ZstdCompression::compress(const std::string& data) {
    // In a real implementation, this would use the zstd library
    // For now, fall back to gzip
    GzipCompression gzip;
    return gzip.compress(data);
}

std::optional<std::string> ZstdCompression::decompress(const std::vector<uint8_t>& compressed) {
    GzipCompression gzip;
    return gzip.decompress(compressed);
}

Lz4Compression::Lz4Compression(const Config& config) : config_(config) {}

std::vector<uint8_t> Lz4Compression::compress(const std::string& data) {
    // In a real implementation, this would use the lz4 library
    // For now, fall back to gzip
    GzipCompression gzip;
    return gzip.compress(data);
}

std::optional<std::string> Lz4Compression::decompress(const std::vector<uint8_t>& compressed) {
    GzipCompression gzip;
    return gzip.decompress(compressed);
}

// Serialization implementations
std::string JsonSerialization::serialize(const std::any& value) {
    // Simplified JSON serialization
    // In a real implementation, this would use a JSON library
    return "{}";
}

std::any JsonSerialization::deserialize(const std::string& data, 
                                       const std::type_info& type) {
    // Simplified JSON deserialization
    return std::any{};
}

std::string BinarySerialization::serialize(const std::any& value) {
    // Simplified binary serialization
    return "";
}

std::any BinarySerialization::deserialize(const std::string& data, 
                                         const std::type_info& type) {
    // Simplified binary deserialization
    return std::any{};
}

// CounterexampleReplayer implementation
CounterexampleReplayer::CounterexampleReplayer(
    std::shared_ptr<CounterexampleDatabase> db)
    : database_(db) {}

void CounterexampleReplayer::remove_fixed(
    const std::string& test_name,
    const std::vector<CounterexampleEntry>& fixed_entries) {
    
    for (const auto& entry : fixed_entries) {
        // In a real implementation, this would remove specific entries
        // For now, we'll just clear the test
        database_->clear_test(test_name);
        break;
    }
}

}  // namespace storage
}  // namespace pbt
}  // namespace jwwlib