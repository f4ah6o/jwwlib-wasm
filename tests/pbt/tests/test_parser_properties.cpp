#include <rapidcheck.h>
#include <gtest/gtest.h>
#include "../properties/parser_properties.h"
#include "../generators/document_generator.h"
#include "../include/test_execution_config.h"

using namespace jwwlib_wasm::pbt;
using namespace jwwlib_wasm::pbt::properties;
using namespace jwwlib_wasm::pbt::generators;

class ParserPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // テスト設定の初期化
        config_ = TestExecutionConfig::getDefault();
        config_.max_test_count = 100;  // CI環境用に調整
        config_.max_size = 50;
        config_.timeout_ms = 5000;
    }

    TestExecutionConfig config_;
};

TEST_F(ParserPropertyTest, RoundTripPropertyTest) {
    auto property = std::make_unique<RoundTripProperty>();
    
    auto result = rc::check(
        "JWW documents survive round-trip (parse-write-parse)",
        [&property]() {
            // ドキュメントを生成
            auto doc = *rc::gen::apply(
                [](const auto& lines, const auto& circles, const auto& arcs, const auto& texts) {
                    return DocumentGenerator::generateWithEntities(lines, circles, arcs, texts);
                },
                rc::gen::container<std::vector<JWWLine>>(rc::gen::arbitrary<JWWLine>()),
                rc::gen::container<std::vector<JWWCircle>>(rc::gen::arbitrary<JWWCircle>()),
                rc::gen::container<std::vector<JWWArc>>(rc::gen::arbitrary<JWWArc>()),
                rc::gen::container<std::vector<JWWText>>(rc::gen::arbitrary<JWWText>())
            );
            
            // プロパティをチェック
            RC_ASSERT(property->check(doc));
        }
    );
    
    EXPECT_TRUE(result);
}

TEST_F(ParserPropertyTest, ParserSafetyPropertyTest) {
    auto property = std::make_unique<ParserSafetyProperty>();
    
    auto result = rc::check(
        "Parser never crashes on arbitrary byte sequences",
        [&property]() {
            // ランダムなバイトシーケンスを生成
            auto data = *rc::gen::container<std::vector<uint8_t>>(
                rc::gen::inRange<uint8_t>(0, 255)
            );
            
            // 最小サイズを保証
            if (data.size() < 100) {
                data.resize(100);
            }
            
            // プロパティをチェック（クラッシュしないことを確認）
            RC_ASSERT(property->check(data));
        }
    );
    
    EXPECT_TRUE(result);
}

TEST_F(ParserPropertyTest, MemorySafetyPropertyTest) {
    auto property = std::make_unique<MemorySafetyProperty>();
    
    auto result = rc::check(
        "No memory leaks during repeated parsing",
        [&property]() {
            // 小さめのドキュメントを生成（メモリテスト用）
            auto doc = *rc::gen::apply(
                [](const auto& lines) {
                    JWWDocument doc;
                    for (const auto& line : lines) {
                        doc.addLine(line);
                    }
                    return doc;
                },
                rc::gen::container<std::vector<JWWLine>>(
                    rc::gen::arbitrary<JWWLine>(),
                    rc::gen::inRange<size_t>(1, 10)
                )
            );
            
            // プロパティをチェック
            RC_ASSERT(property->check(doc));
        }
    );
    
    EXPECT_TRUE(result);
}

TEST_F(ParserPropertyTest, CombinedPropertiesTest) {
    auto properties = ParserPropertyBuilder::buildBasicProperties();
    
    auto result = rc::check(
        "All basic parser properties hold",
        [&properties]() {
            auto doc = *DocumentGenerator::arbitrary();
            
            // すべてのプロパティをチェック
            for (const auto& property : properties) {
                RC_ASSERT(property->check(doc));
            }
        }
    );
    
    EXPECT_TRUE(result);
}

TEST_F(ParserPropertyTest, EdgeCaseDocumentTest) {
    auto property = std::make_unique<RoundTripProperty>();
    
    // エッジケース：空のドキュメント
    {
        JWWDocument empty_doc;
        EXPECT_TRUE(property->check(empty_doc));
    }
    
    // エッジケース：単一エンティティ
    {
        JWWDocument single_entity_doc;
        single_entity_doc.addLine(JWWLine{{0.0, 0.0}, {1.0, 1.0}});
        EXPECT_TRUE(property->check(single_entity_doc));
    }
    
    // エッジケース：多数のレイヤー
    {
        JWWDocument multi_layer_doc;
        for (int i = 0; i < 16; ++i) {
            multi_layer_doc.setCurrentLayer(i);
            multi_layer_doc.addLine(JWWLine{{0.0, 0.0}, {1.0, 1.0}});
        }
        EXPECT_TRUE(property->check(multi_layer_doc));
    }
}

// 反例最小化のテスト
TEST_F(ParserPropertyTest, CounterexampleMinimizationTest) {
    auto property = std::make_unique<RoundTripProperty>();
    
    // わざと失敗するプロパティを作成して反例最小化をテスト
    class FailingProperty : public PropertyBase<JWWDocument> {
    public:
        FailingProperty() : PropertyBase<JWWDocument>("FailingProperty", "Always fails") {}
        
        bool check(const JWWDocument& doc) const override {
            // 10個以上のエンティティがあると失敗
            size_t total_entities = 0;
            for (const auto& layer : doc.getLayers()) {
                total_entities += layer.getEntities().size();
            }
            return total_entities < 10;
        }
    };
    
    auto failing_property = std::make_unique<FailingProperty>();
    
    // 反例最小化のテスト
    try {
        rc::check(
            "Counterexample minimization works",
            [&failing_property]() {
                auto doc = *DocumentGenerator::arbitrary();
                RC_ASSERT(failing_property->check(doc));
            },
            rc::settings{
                rc::maxSuccess = 10,
                rc::maxSize = 100
            }
        );
        
        // このテストは失敗することが期待される
        EXPECT_TRUE(false) << "Expected test to fail for counterexample minimization";
    } catch (const std::exception& e) {
        // 反例が見つかることを確認
        std::string error_msg = e.what();
        EXPECT_TRUE(error_msg.find("Falsifiable") != std::string::npos);
    }
}