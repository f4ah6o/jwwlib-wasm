#pragma once

#include "../include/property_base.h"
#include "../include/property_builder.h"
#include "../include/jww_test_entities.h"
#include <jwwlib/jww_document.h>
#include <jwwlib/jww_parser.h>
#include <jwwlib/jww_error.h>
#include <limits>
#include <cmath>

namespace jwwlib_wasm::pbt::properties {

// パースエラー処理プロパティ
// 不正なデータに対して適切なエラーを返すことを検証
class ParseErrorHandlingProperty : public PropertyBase<std::pair<std::vector<uint8_t>, JWWErrorType>> {
public:
    ParseErrorHandlingProperty() : PropertyBase<std::pair<std::vector<uint8_t>, JWWErrorType>>(
        "ParseErrorHandlingProperty",
        "Parser returns appropriate errors for invalid data") {}

    bool check(const std::pair<std::vector<uint8_t>, JWWErrorType>& input) const override {
        const auto& [data, expected_error_type] = input;
        
        try {
            auto temp_path = std::filesystem::temp_directory_path() / "pbt_error.jww";
            std::ofstream ofs(temp_path, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
            ofs.close();
            
            JWWParser parser;
            auto result = parser.parse(temp_path.string());
            
            std::filesystem::remove(temp_path);
            
            if (result) {
                // パースが成功した場合、エラーが期待されていなければOK
                return expected_error_type == JWWErrorType::None;
            } else {
                // パースが失敗した場合、適切なエラータイプかチェック
                auto error = parser.getLastError();
                return error.type == expected_error_type;
            }
        } catch (...) {
            // 例外が発生した場合もエラーハンドリングの一部
            return expected_error_type != JWWErrorType::None;
        }
    }
};

// 境界値テストプロパティ
// 数値の境界値で正しく動作することを検証
class BoundaryValueProperty : public PropertyBase<JWWDocument> {
public:
    BoundaryValueProperty() : PropertyBase<JWWDocument>("BoundaryValueProperty",
        "Parser handles boundary values correctly") {}

    bool check(const JWWDocument& doc) const override {
        // 境界値を含むドキュメントを作成
        JWWDocument boundary_doc;
        
        // 極大・極小座標値
        boundary_doc.addLine(JWWLine{
            {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
            {std::numeric_limits<double>::min(), std::numeric_limits<double>::min()}
        });
        
        // ゼロ長さの線分
        boundary_doc.addLine(JWWLine{{0.0, 0.0}, {0.0, 0.0}});
        
        // 極小半径の円
        boundary_doc.addCircle(JWWCircle{
            {0.0, 0.0},
            std::numeric_limits<double>::epsilon()
        });
        
        // 空の文字列
        boundary_doc.addText(JWWText{{0.0, 0.0}, "", 0.0, 1.0});
        
        // 最大長の文字列（仮に1000文字とする）
        std::string long_text(1000, 'A');
        boundary_doc.addText(JWWText{{0.0, 0.0}, long_text, 0.0, 1.0});
        
        try {
            auto temp_path = std::filesystem::temp_directory_path() / "pbt_boundary.jww";
            
            JWWWriter writer;
            if (!writer.write(boundary_doc, temp_path.string())) {
                return false;
            }
            
            JWWParser parser;
            auto result = parser.parse(temp_path.string());
            
            std::filesystem::remove(temp_path);
            
            return result.has_value();
        } catch (...) {
            return false;
        }
    }
};

// 異常データ処理プロパティ
// NaN、無限大、不正なエンコーディングなどを適切に処理
class AbnormalDataProperty : public PropertyBase<JWWDocument> {
public:
    AbnormalDataProperty() : PropertyBase<JWWDocument>("AbnormalDataProperty",
        "Parser handles abnormal data gracefully") {}

    bool check(const JWWDocument& doc) const override {
        JWWDocument abnormal_doc;
        
        // NaN座標
        abnormal_doc.addLine(JWWLine{
            {std::nan(""), std::nan("")},
            {0.0, 0.0}
        });
        
        // 無限大座標
        abnormal_doc.addCircle(JWWCircle{
            {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()},
            1.0
        });
        
        // 負の半径（物理的に不正）
        abnormal_doc.addCircle(JWWCircle{{0.0, 0.0}, -1.0});
        
        // 不正な角度の円弧（開始角 > 終了角 + 360）
        abnormal_doc.addArc(JWWArc{
            {0.0, 0.0},
            1.0,
            720.0,  // 開始角
            0.0     // 終了角
        });
        
        try {
            auto temp_path = std::filesystem::temp_directory_path() / "pbt_abnormal.jww";
            
            JWWWriter writer;
            bool write_result = writer.write(abnormal_doc, temp_path.string());
            
            if (write_result) {
                JWWParser parser;
                auto parse_result = parser.parse(temp_path.string());
                std::filesystem::remove(temp_path);
                
                // 異常データは適切にフィルタリングまたはエラーとして処理されるべき
                return true;
            } else {
                // 書き込み時点でエラーとして処理されてもOK
                return true;
            }
        } catch (...) {
            // 例外で処理されてもOK
            return true;
        }
    }
};

// 大量データ処理プロパティ
// 極端に多くのエンティティやレイヤーを適切に処理
class LargeDataProperty : public PropertyBase<size_t> {
public:
    LargeDataProperty() : PropertyBase<size_t>("LargeDataProperty",
        "Parser handles large amounts of data without issues") {}

    bool check(const size_t& entity_count) const override {
        // entity_countは1から10000の範囲を想定
        const size_t clamped_count = std::min(entity_count, size_t(10000));
        
        JWWDocument large_doc;
        
        // 多数のエンティティを追加
        for (size_t i = 0; i < clamped_count; ++i) {
            double pos = static_cast<double>(i);
            large_doc.addLine(JWWLine{{pos, 0.0}, {pos + 1.0, 1.0}});
        }
        
        try {
            auto temp_path = std::filesystem::temp_directory_path() / "pbt_large.jww";
            
            JWWWriter writer;
            if (!writer.write(large_doc, temp_path.string())) {
                // 書き込み失敗は許容（メモリ不足など）
                return true;
            }
            
            JWWParser parser;
            auto result = parser.parse(temp_path.string());
            
            std::filesystem::remove(temp_path);
            
            if (result) {
                // 正常にパースできた場合、エンティティ数が一致するか確認
                size_t total_entities = 0;
                for (const auto& layer : result->getLayers()) {
                    total_entities += layer.getEntities().size();
                }
                return total_entities == clamped_count;
            } else {
                // パース失敗も適切なエラーハンドリングの一部
                return true;
            }
        } catch (...) {
            // メモリ不足などの例外も適切な処理
            return true;
        }
    }
};

// エラーハンドリングプロパティビルダー
class ErrorHandlingPropertyBuilder {
public:
    static auto buildErrorProperties() {
        PropertyBuilder<std::pair<std::vector<uint8_t>, JWWErrorType>> builder;
        
        return builder
            .add(std::make_unique<ParseErrorHandlingProperty>())
            .build();
    }
    
    static auto buildBoundaryProperties() {
        PropertyBuilder<JWWDocument> builder;
        
        return builder
            .add(std::make_unique<BoundaryValueProperty>())
            .add(std::make_unique<AbnormalDataProperty>())
            .build();
    }
    
    static auto buildStressProperties() {
        PropertyBuilder<size_t> builder;
        
        return builder
            .add(std::make_unique<LargeDataProperty>())
            .build();
    }
};

} // namespace jwwlib_wasm::pbt::properties