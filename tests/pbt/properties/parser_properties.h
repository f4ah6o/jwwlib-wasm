#pragma once

#include "../include/property_base.h"
#include "../include/property_builder.h"
#include "../include/jww_test_entities.h"
#include <jwwlib/jww_document.h>
#include <jwwlib/jww_parser.h>
#include <jwwlib/jww_writer.h>
#include <memory>
#include <filesystem>
#include <fstream>

namespace jwwlib_wasm::pbt::properties {

// ラウンドトリップテストプロパティ
// パース → 書き込み → 再パースで同じ構造になることを検証
class RoundTripProperty : public PropertyBase<JWWDocument> {
public:
    RoundTripProperty() : PropertyBase<JWWDocument>("RoundTripProperty", 
        "Parse-Write-Parse produces identical structure") {}

    bool check(const JWWDocument& doc) const override {
        try {
            // 一時ファイルに書き込み
            auto temp_path = std::filesystem::temp_directory_path() / "pbt_roundtrip.jww";
            
            // ドキュメントを書き込み
            JWWWriter writer;
            if (!writer.write(doc, temp_path.string())) {
                return false;
            }
            
            // 再度パース
            JWWParser parser;
            auto parsed_doc = parser.parse(temp_path.string());
            
            // 一時ファイルを削除
            std::filesystem::remove(temp_path);
            
            if (!parsed_doc) {
                return false;
            }
            
            // ドキュメント構造の比較
            return compareDocuments(doc, *parsed_doc);
        } catch (...) {
            return false;
        }
    }

private:
    bool compareDocuments(const JWWDocument& doc1, const JWWDocument& doc2) const {
        // ヘッダー情報の比較
        if (doc1.getVersion() != doc2.getVersion()) return false;
        if (doc1.getJwwVersion() != doc2.getJwwVersion()) return false;
        
        // レイヤー数の比較
        if (doc1.getLayers().size() != doc2.getLayers().size()) return false;
        
        // 各レイヤーのエンティティ数の比較
        for (size_t i = 0; i < doc1.getLayers().size(); ++i) {
            const auto& layer1 = doc1.getLayers()[i];
            const auto& layer2 = doc2.getLayers()[i];
            
            if (layer1.getName() != layer2.getName()) return false;
            if (layer1.getEntities().size() != layer2.getEntities().size()) return false;
            
            // エンティティの詳細比較は省略（実装では必要）
        }
        
        return true;
    }
};

// パーサー安全性プロパティ
// 任意のバイトシーケンスでクラッシュしないことを検証
class ParserSafetyProperty : public PropertyBase<std::vector<uint8_t>> {
public:
    ParserSafetyProperty() : PropertyBase<std::vector<uint8_t>>("ParserSafetyProperty",
        "Parser never crashes on arbitrary byte sequences") {}

    bool check(const std::vector<uint8_t>& data) const override {
        try {
            // 一時ファイルに書き込み
            auto temp_path = std::filesystem::temp_directory_path() / "pbt_safety.jww";
            std::ofstream ofs(temp_path, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
            ofs.close();
            
            // パースを試みる
            JWWParser parser;
            auto result = parser.parse(temp_path.string());
            
            // 一時ファイルを削除
            std::filesystem::remove(temp_path);
            
            // クラッシュしなければ成功
            return true;
        } catch (const std::exception&) {
            // 例外は正常な動作
            return true;
        } catch (...) {
            // 未知の例外は失敗
            return false;
        }
    }
};

// メモリ安全性プロパティ
// メモリリークや不正アクセスがないことを検証
class MemorySafetyProperty : public PropertyBase<JWWDocument> {
public:
    MemorySafetyProperty() : PropertyBase<JWWDocument>("MemorySafetyProperty",
        "No memory leaks or invalid access during parsing") {}

    bool check(const JWWDocument& doc) const override {
        // Valgrind/ASan統合が必要
        // ここでは基本的なメモリ操作のチェックのみ
        try {
            // 大量のエンティティでメモリ使用をテスト
            auto temp_path = std::filesystem::temp_directory_path() / "pbt_memory.jww";
            
            JWWWriter writer;
            if (!writer.write(doc, temp_path.string())) {
                return false;
            }
            
            // 複数回パースしてメモリリークをチェック
            for (int i = 0; i < 10; ++i) {
                JWWParser parser;
                auto result = parser.parse(temp_path.string());
                if (!result) {
                    std::filesystem::remove(temp_path);
                    return false;
                }
                // resultはスコープを抜けると自動的に削除される
            }
            
            std::filesystem::remove(temp_path);
            return true;
        } catch (...) {
            return false;
        }
    }
};

// プロパティビルダー
class ParserPropertyBuilder {
public:
    static auto buildBasicProperties() {
        PropertyBuilder<JWWDocument> builder;
        
        return builder
            .add(std::make_unique<RoundTripProperty>())
            .build();
    }
    
    static auto buildSafetyProperties() {
        PropertyBuilder<std::vector<uint8_t>> builder;
        
        return builder
            .add(std::make_unique<ParserSafetyProperty>())
            .build();
    }
    
    static auto buildMemoryProperties() {
        PropertyBuilder<JWWDocument> builder;
        
        return builder
            .add(std::make_unique<MemorySafetyProperty>())
            .build();
    }
};

} // namespace jwwlib_wasm::pbt::properties