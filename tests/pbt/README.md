# Property-Based Testing for jwwlib-wasm

Property-Based Testing (PBT) フレームワークは、jwwlib-wasmの品質保証を強化するための自動テスト生成システムです。

## 概要

PBTは、コードの不変条件（プロパティ）を定義し、ランダムに生成されたテストデータでこれらのプロパティを検証します。従来の例ベーステストと異なり、PBTは予期しないエッジケースを自動的に発見できます。

## 主な機能

### 1. プロパティ定義フレームワーク
* 再利用可能なプロパティ基底クラス
* プロパティの合成と組み合わせ
* 型安全なプロパティ定義

### 2. JWWエンティティジェネレータ
* 線分、円、円弧、テキストの自動生成
* 日本語文字列（Shift-JIS）の生成
* レイヤー構造とドキュメント全体の生成

### 3. テスト済みプロパティ
* **パーサープロパティ**: ラウンドトリップ、安全性、メモリリーク
* **エラーハンドリング**: 境界値、異常データ、大量データ
* **WASMバインディング**: 構造体マッピング、エンコーディング、数値精度

## クイックスタート

### ビルド

```bash
# プロジェクトのビルド
cd jwwlib-wasm
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON -DENABLE_PBT=ON
make pbt_tests
```

### テスト実行

```bash
# 全てのPBTテストを実行
ctest -R pbt_

# 個別のテストを実行
./tests/pbt/pbt_test_parser_properties

# スクリプトを使用した実行
../tests/pbt/scripts/run_pbt_tests.sh
```

### 失敗の再現

```bash
# 反例ファイルから失敗を再現
../tests/pbt/scripts/reproduce_failure.sh build/counterexample.txt

# デバッガで再現
../tests/pbt/scripts/reproduce_failure.sh --debug build/counterexample.txt
```

## プロパティの作成方法

### 1. 新しいプロパティクラスの定義

```cpp
#include "property_base.h"

class MyProperty : public PropertyBase<MyType> {
public:
    MyProperty() : PropertyBase<MyType>("MyProperty", "Description") {}
    
    bool check(const MyType& input) const override {
        // プロパティの検証ロジック
        return /* 条件 */;
    }
};
```

### 2. ジェネレータの作成

```cpp
namespace rc {
template<>
struct Arbitrary<MyType> {
    static Gen<MyType> arbitrary() {
        return gen::build<MyType>(
            gen::set(&MyType::field1, gen::arbitrary<int>()),
            gen::set(&MyType::field2, gen::arbitrary<std::string>())
        );
    }
};
}
```

### 3. テストの作成

```cpp
TEST(MyPropertyTest, TestName) {
    auto property = std::make_unique<MyProperty>();
    
    auto result = rc::check(
        "Property description",
        [&property]() {
            auto input = *rc::gen::arbitrary<MyType>();
            RC_ASSERT(property->check(input));
        }
    );
    
    EXPECT_TRUE(result);
}
```

## CI/CD統合

GitHub Actionsワークフローが自動的に以下を実行します：

1. **通常のPBTテスト**: PRごとに実行
2. **メモリチェック**: AddressSanitizerとValgrind
3. **カバレッジ測定**: lcovによるカバレッジレポート
4. **ストレステスト**: 手動トリガーで長時間実行

## ベストプラクティス

### 1. プロパティ設計
* 単一の概念に焦点を当てる
* 実装の詳細ではなく仕様を記述
* 反例が理解しやすいようにする

### 2. ジェネレータ設計
* 有効なデータと無効なデータの両方を生成
* エッジケースを含める
* 縮小（shrinking）をサポート

### 3. テスト実行
* CI環境では適切なタイムアウトを設定
* 反例は必ず保存して再現可能にする
* カバレッジを監視して盲点を発見

## トラブルシューティング

### テストがタイムアウトする
```bash
# タイムアウトを延長
RAPIDCHECK_MAX_SUCCESS=50 ./pbt_tests
```

### メモリ不足
```bash
# データサイズを制限
RAPIDCHECK_MAX_SIZE=10 ./pbt_tests
```

### 反例の最小化
```cpp
// RC_CLASSIFYを使用して失敗を分類
RC_CLASSIFY(input.size() < 10, "small");
RC_CLASSIFY(input.size() < 100, "medium");
RC_CLASSIFY(input.size() >= 100, "large");
```

## 参考資料

* [RapidCheck Documentation](https://github.com/emil-e/rapidcheck)
* [Property-Based Testing with C++](https://www.youtube.com/watch?v=shngiiBfD80)
* [QuickCheck Tutorial](http://www.cse.chalmers.se/~rjmh/QuickCheck/manual.html)