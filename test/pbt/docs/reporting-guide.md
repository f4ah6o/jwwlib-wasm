# PBT レポート機能ガイド

## 概要

jwwlib-wasmのProperty-Based Testing (PBT) フレームワークには、包括的なレポート生成機能が含まれています。この機能により、テスト結果の可視化、メトリクスの追跡、エッジケースカバレッジの分析が可能になります。

## 主要コンポーネント

### 1. HTML レポートジェネレータ

HTMLレポートジェネレータは、テスト実行結果を視覚的にわかりやすい形式で出力します。

```cpp
#include "reporting/html_report_generator.hpp"

// レポート設定
ReportConfig config;
config.output_directory = "./reports";
config.report_title = "JWW PBT実行レポート";
config.include_charts = true;
config.include_coverage = true;

// レポートジェネレータの作成
HtmlReportGenerator generator(config);

// テストスイートの追加
TestSuite suite;
suite.name = "パーサープロパティテスト";
suite.description = "JWWファイルパーサーの健全性チェック";

// テスト結果の追加
TestResult result;
result.property_name = "prop_roundtrip";
result.passed = true;
result.test_cases_run = 1000;
result.test_cases_failed = 0;
result.execution_time = std::chrono::milliseconds(1500);
suite.results.push_back(result);

generator.addTestSuite(suite);
generator.generateReport();
```

### 2. データビジュアライザ

データ分布の可視化とエッジケース分析機能を提供します。

```cpp
#include "reporting/data_visualizer.hpp"

// データ分布の分析
std::vector<double> test_data = generateTestData();
auto stats = DataVisualizer::calculateStats(test_data);

// ヒストグラムの生成
auto histogram = DataVisualizer::createHistogram(test_data, 20);
auto svg = DataVisualizer::generateDistributionSvg(
    histogram, 800, 600, "入力値分布"
);

// エッジケース分析
std::vector<std::function<bool(const JWWLine&)>> detectors = {
    [](const JWWLine& line) { return line.length() == 0; },
    [](const JWWLine& line) { return line.isVertical(); },
    [](const JWWLine& line) { return line.isHorizontal(); }
};

std::vector<std::string> edge_case_names = {
    "長さゼロの線分",
    "垂直線",
    "水平線"
};

auto coverage = EdgeCaseAnalyzer::analyzeEdgeCases(
    generated_lines, detectors, edge_case_names
);
```

### 3. メトリクスコレクタ

テスト実行中のメトリクスを自動的に収集します。

```cpp
#include "reporting/test_metrics.hpp"

auto& collector = MetricsCollector::getInstance();

// プロパティテストの開始
collector.startProperty("prop_parser_safety");

// テストケースごとのメトリクス記録
MetricsTimer gen_timer("prop_parser_safety", "generation");
auto input = generateJWWDocument();
auto gen_time = gen_timer.elapsed();

MetricsTimer exec_timer("prop_parser_safety", "execution");
bool passed = testProperty(input);
auto exec_time = exec_timer.elapsed();

collector.recordTestCase(
    "prop_parser_safety",
    input.size(),
    gen_time,
    exec_time,
    passed
);

// エッジケースの記録
if (input.isEmpty()) {
    collector.recordEdgeCaseHit("empty_document");
}

// プロパティテストの終了
collector.endProperty("prop_parser_safety", passed);
```

## 生成されるレポート

### HTMLレポート

HTMLレポートには以下の情報が含まれます：

1. **サマリーセクション**
   - 総テスト数
   - 成功/失敗数
   - 全体成功率
   - 総実行時間

2. **テストスイート詳細**
   - 各プロパティの結果
   - 実行時間
   - 失敗時の反例

3. **統計グラフ**
   - 成功率チャート
   - 実行時間の推移
   - テストケース分布
   - カバレッジ情報

4. **環境情報**
   - 実行環境の詳細
   - タイムスタンプ

### メトリクスレポート

メトリクスレポートは以下の形式で出力可能です：

- **HTML形式**: ブラウザで閲覧可能な詳細レポート
- **JSON形式**: プログラムでの処理に適した構造化データ
- **XML形式**: 他のツールとの統合用

```cpp
// JSON形式でエクスポート
collector.exportMetrics("metrics.json");

// レポート生成オプション
MetricsReporter::ReportOptions options;
options.include_property_breakdown = true;
options.include_memory_analysis = true;
options.include_performance_trends = true;
options.format = "json";

auto report = MetricsReporter::generateReport(collector, options);
```

## ベストプラクティス

### 1. 継続的なメトリクス収集

```cpp
class PropertyTest {
    void runTest() {
        auto& collector = MetricsCollector::getInstance();
        collector.startProperty(property_name_);
        
        try {
            // テスト実行
            executeProperty();
            collector.endProperty(property_name_, true);
        } catch (const std::exception& e) {
            collector.endProperty(property_name_, false);
            throw;
        }
    }
};
```

### 2. エッジケースの定義

```cpp
struct JWWEntityEdgeCases {
    static std::vector<std::function<bool(const JWWEntity&)>> getDetectors() {
        return {
            [](const auto& e) { return e.isAtOrigin(); },
            [](const auto& e) { return e.hasMaximumSize(); },
            [](const auto& e) { return e.hasSpecialCharacters(); }
        };
    }
};
```

### 3. カスタムレポートセクション

```cpp
class CustomReportSection {
    std::string generateSection() const {
        std::ostringstream html;
        html << "<div class='custom-section'>";
        html << "<h3>カスタム分析</h3>";
        // カスタム内容
        html << "</div>";
        return html.str();
    }
};
```

## CI/CD統合

### GitHub Actions での使用例

```yaml
- name: Run PBT with reporting
  run: |
    mkdir -p reports
    ./build/pbt_tests --gtest_output=xml:reports/test_results.xml
    
- name: Generate HTML report
  run: |
    ./scripts/generate_pbt_report.sh reports/
    
- name: Upload reports
  uses: actions/upload-artifact@v3
  with:
    name: pbt-reports
    path: reports/
```

## トラブルシューティング

### レポート生成エラー

1. **ディレクトリ権限エラー**
   ```cpp
   // 出力ディレクトリの存在確認と作成
   if (!std::filesystem::exists(output_dir)) {
       std::filesystem::create_directories(output_dir);
   }
   ```

2. **メモリ不足**
   ```cpp
   // 大量データの場合は分割処理
   const size_t BATCH_SIZE = 1000;
   for (size_t i = 0; i < data.size(); i += BATCH_SIZE) {
       processBatch(data, i, std::min(i + BATCH_SIZE, data.size()));
   }
   ```

3. **SVG生成エラー**
   ```cpp
   // データの正規化
   auto normalized_data = normalizeData(raw_data);
   auto svg = generateSvg(normalized_data);
   ```

## まとめ

PBTレポート機能を活用することで、以下のメリットが得られます：

- テスト結果の可視化による問題の早期発見
- メトリクスの継続的な追跡による品質向上
- エッジケースカバレッジの向上
- チーム内での情報共有の促進

詳細な実装例は、`test/pbt/tests/test_reporting.cpp`を参照してください。