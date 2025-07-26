#include <rapidcheck.h>
#include <gtest/gtest.h>
#include "../properties/error_handling_properties.h"
#include "../generators/document_generator.h"
#include "../include/test_execution_config.h"

using namespace jwwlib_wasm::pbt;
using namespace jwwlib_wasm::pbt::properties;
using namespace jwwlib_wasm::pbt::generators;

class ErrorHandlingPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = TestExecutionConfig::getDefault();
        config_.max_test_count = 50;  // エラーハンドリングテストは時間がかかるため少なめ
        config_.timeout_ms = 10000;
    }

    TestExecutionConfig config_;
};

TEST_F(ErrorHandlingPropertyTest, ParseErrorHandlingPropertyTest) {
    auto property = std::make_unique<ParseErrorHandlingProperty>();
    
    auto result = rc::check(
        "Parser returns appropriate errors for invalid data",
        [&property]() {
            // エラータイプをランダムに選択
            auto error_type = *rc::gen::element(
                JWWErrorType::InvalidFormat,
                JWWErrorType::UnsupportedVersion,
                JWWErrorType::CorruptedData,
                JWWErrorType::None
            );
            
            // エラータイプに応じたデータを生成
            std::vector<uint8_t> data;
            if (error_type == JWWErrorType::InvalidFormat) {
                // 完全にランダムなデータ
                data = *rc::gen::container<std::vector<uint8_t>>(
                    rc::gen::inRange<uint8_t>(0, 255),
                    rc::gen::inRange<size_t>(100, 1000)
                );
            } else if (error_type == JWWErrorType::UnsupportedVersion) {
                // JWWヘッダーっぽいが不正なバージョン
                std::string header = "JW_CAD VERSION 99.99\n";
                data.assign(header.begin(), header.end());
            } else if (error_type == JWWErrorType::CorruptedData) {
                // 途中で切れたデータ
                std::string partial = "JW_CAD VERSION 7.11\nLAYER 0\n";
                data.assign(partial.begin(), partial.end());
            } else {
                // 正常なデータ（簡易版）
                std::string valid = "JW_CAD VERSION 7.11\nLAYER 0\nEOF\n";
                data.assign(valid.begin(), valid.end());
            }
            
            RC_ASSERT(property->check({data, error_type}));
        }
    );
    
    EXPECT_TRUE(result);
}

TEST_F(ErrorHandlingPropertyTest, BoundaryValuePropertyTest) {
    auto property = std::make_unique<BoundaryValueProperty>();
    
    // 境界値テストは決定的なので、直接テスト
    JWWDocument dummy_doc;  // checkメソッドは引数を使用しない
    EXPECT_TRUE(property->check(dummy_doc));
}

TEST_F(ErrorHandlingPropertyTest, AbnormalDataPropertyTest) {
    auto property = std::make_unique<AbnormalDataProperty>();
    
    // 異常データテストも決定的
    JWWDocument dummy_doc;
    EXPECT_TRUE(property->check(dummy_doc));
}

TEST_F(ErrorHandlingPropertyTest, LargeDataPropertyTest) {
    auto property = std::make_unique<LargeDataProperty>();
    
    auto result = rc::check(
        "Parser handles large amounts of data appropriately",
        [&property]() {
            // エンティティ数をランダムに生成
            auto entity_count = *rc::gen::inRange<size_t>(1, 1000);
            
            RC_ASSERT(property->check(entity_count));
        }
    );
    
    EXPECT_TRUE(result);
}

TEST_F(ErrorHandlingPropertyTest, StressTestWithMixedData) {
    auto boundary_properties = ErrorHandlingPropertyBuilder::buildBoundaryProperties();
    auto stress_properties = ErrorHandlingPropertyBuilder::buildStressProperties();
    
    // 境界値プロパティのテスト
    {
        JWWDocument doc;
        for (const auto& property : boundary_properties) {
            EXPECT_TRUE(property->check(doc));
        }
    }
    
    // ストレステスト
    {
        auto result = rc::check(
            "All stress properties hold",
            [&stress_properties]() {
                auto size = *rc::gen::inRange<size_t>(1, 500);
                
                for (const auto& property : stress_properties) {
                    RC_ASSERT(property->check(size));
                }
            }
        );
        
        EXPECT_TRUE(result);
    }
}

TEST_F(ErrorHandlingPropertyTest, CorruptedFileRecoveryTest) {
    // 破損したファイルからの回復テスト
    auto property = std::make_unique<ParserSafetyProperty>();
    
    auto result = rc::check(
        "Parser recovers gracefully from corrupted files",
        [&property]() {
            // 正常なJWWファイルの一部を生成
            std::string header = "JW_CAD VERSION 7.11\n";
            std::vector<uint8_t> data(header.begin(), header.end());
            
            // ランダムな位置にゴミデータを挿入
            auto corruption_pos = *rc::gen::inRange<size_t>(0, data.size());
            auto garbage = *rc::gen::container<std::vector<uint8_t>>(
                rc::gen::inRange<uint8_t>(0, 255),
                rc::gen::inRange<size_t>(1, 100)
            );
            
            data.insert(data.begin() + corruption_pos, garbage.begin(), garbage.end());
            
            // パーサーがクラッシュしないことを確認
            RC_ASSERT(property->check(data));
        }
    );
    
    EXPECT_TRUE(result);
}

TEST_F(ErrorHandlingPropertyTest, EncodingErrorTest) {
    // エンコーディングエラーのテスト
    auto property = std::make_unique<ParserSafetyProperty>();
    
    // 不正なShift-JISシーケンスを含むデータ
    std::vector<uint8_t> invalid_sjis = {
        0x4A, 0x57, 0x5F, 0x43, 0x41, 0x44, 0x20,  // "JW_CAD "
        0x82,  // Shift-JISの開始バイトだが、続きがない
        0x0A   // 改行
    };
    
    EXPECT_TRUE(property->check(invalid_sjis));
}

TEST_F(ErrorHandlingPropertyTest, MemoryExhaustionTest) {
    auto property = std::make_unique<LargeDataProperty>();
    
    // メモリ枯渇をシミュレート（非常に大きな数）
    size_t huge_count = 1000000;  // プロパティ内で10000に制限される
    
    EXPECT_TRUE(property->check(huge_count));
}