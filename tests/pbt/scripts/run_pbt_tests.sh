#!/bin/bash

# Property-Based Testing実行スクリプト

set -e

# カラー出力の定義
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# デフォルト設定
BUILD_DIR="build"
TEST_COUNT=100
MAX_SIZE=50
TIMEOUT=600
VERBOSE=false
MEMCHECK=false
COVERAGE=false
FILTER=""

# ヘルプメッセージ
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Property-Based Testing実行スクリプト

OPTIONS:
    -b, --build-dir DIR     ビルドディレクトリ (default: build)
    -c, --count N           各プロパティのテスト回数 (default: 100)
    -s, --size N            生成データの最大サイズ (default: 50)
    -t, --timeout SEC       タイムアウト秒数 (default: 600)
    -f, --filter PATTERN    テストフィルタパターン
    -v, --verbose           詳細出力
    -m, --memcheck          Valgrind/ASanでメモリチェック
    -g, --coverage          カバレッジ測定
    -h, --help              このヘルプを表示

EXAMPLES:
    # 基本的な実行
    $0

    # パーサープロパティのみ実行
    $0 --filter parser

    # メモリチェック付きで実行
    $0 --memcheck

    # カバレッジ測定付きで実行
    $0 --coverage
EOF
}

# 引数解析
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -c|--count)
            TEST_COUNT="$2"
            shift 2
            ;;
        -s|--size)
            MAX_SIZE="$2"
            shift 2
            ;;
        -t|--timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        -f|--filter)
            FILTER="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -m|--memcheck)
            MEMCHECK=true
            shift
            ;;
        -g|--coverage)
            COVERAGE=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            exit 1
            ;;
    esac
done

# ビルドディレクトリの確認
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Build directory not found: $BUILD_DIR${NC}"
    echo "Please build the project first."
    exit 1
fi

# テスト実行環境の設定
export RAPIDCHECK_MAX_SUCCESS=$TEST_COUNT
export RAPIDCHECK_MAX_SIZE=$MAX_SIZE

echo -e "${GREEN}=== Property-Based Testing ===${NC}"
echo "Build directory: $BUILD_DIR"
echo "Test count: $TEST_COUNT"
echo "Max size: $MAX_SIZE"
echo "Timeout: ${TIMEOUT}s"

# メモリチェックの設定
if [ "$MEMCHECK" = true ]; then
    # Valgrindの確認
    if command -v valgrind &> /dev/null; then
        echo -e "${YELLOW}Running with Valgrind memory check${NC}"
        TEST_PREFIX="valgrind --leak-check=full --error-exitcode=1"
    else
        echo -e "${YELLOW}Valgrind not found, using AddressSanitizer${NC}"
        export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
    fi
fi

# カバレッジの設定
if [ "$COVERAGE" = true ]; then
    echo -e "${YELLOW}Coverage measurement enabled${NC}"
    # カバレッジデータのクリア
    find "$BUILD_DIR" -name '*.gcda' -delete 2>/dev/null || true
fi

# テストの実行
cd "$BUILD_DIR"

# フィルタの設定
if [ -n "$FILTER" ]; then
    TEST_REGEX="pbt_.*${FILTER}.*"
else
    TEST_REGEX="pbt_"
fi

echo -e "\n${GREEN}Running PBT tests...${NC}"

# CTestでテスト実行
if [ "$VERBOSE" = true ]; then
    CTEST_ARGS="-V"
else
    CTEST_ARGS="--output-on-failure"
fi

if ! ctest -R "$TEST_REGEX" $CTEST_ARGS --timeout "$TIMEOUT"; then
    echo -e "${RED}Some tests failed!${NC}"
    
    # 反例の検索と表示
    echo -e "\n${YELLOW}Searching for counterexamples...${NC}"
    find . -name "*_counterexample.txt" -type f | while read -r file; do
        echo -e "${YELLOW}Found counterexample: $file${NC}"
        cat "$file"
        echo
    done
    
    exit 1
fi

echo -e "${GREEN}All tests passed!${NC}"

# カバレッジレポートの生成
if [ "$COVERAGE" = true ] && command -v lcov &> /dev/null; then
    echo -e "\n${GREEN}Generating coverage report...${NC}"
    
    lcov --capture --directory . --output-file pbt_coverage.info
    lcov --remove pbt_coverage.info '/usr/*' '*/test/*' '*/build/*' --output-file pbt_coverage_filtered.info
    
    # HTMLレポートの生成
    if command -v genhtml &> /dev/null; then
        genhtml pbt_coverage_filtered.info --output-directory pbt_coverage_html
        echo -e "${GREEN}Coverage report generated: $BUILD_DIR/pbt_coverage_html/index.html${NC}"
    fi
    
    # カバレッジサマリーの表示
    lcov --summary pbt_coverage_filtered.info
fi

# 統計情報の表示
echo -e "\n${GREEN}=== Test Statistics ===${NC}"
echo "Total properties tested: $(ctest -N -R "$TEST_REGEX" | grep -c "Test #")"
echo "Random seed used: ${RAPIDCHECK_SEED:-<not set>}"

# メモリ使用量の表示（Linux環境）
if [ -f /proc/self/status ]; then
    echo "Peak memory usage: $(grep VmPeak /proc/self/status | awk '{print $2 " " $3}')"
fi

echo -e "\n${GREEN}PBT tests completed successfully!${NC}"