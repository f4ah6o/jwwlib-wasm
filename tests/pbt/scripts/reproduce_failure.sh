#!/bin/bash

# PBT失敗ケース再現スクリプト

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

usage() {
    cat << EOF
Usage: $0 [OPTIONS] COUNTEREXAMPLE_FILE

Property-Based Testing失敗ケース再現スクリプト

ARGUMENTS:
    COUNTEREXAMPLE_FILE    反例ファイルのパス

OPTIONS:
    -b, --build-dir DIR    ビルドディレクトリ (default: build)
    -d, --debug            デバッガ（gdb/lldb）でテストを実行
    -v, --valgrind         Valgrindでメモリエラーをチェック
    -h, --help             このヘルプを表示

EXAMPLES:
    # 反例を再現
    $0 build/test_parser_counterexample.txt

    # デバッガで再現
    $0 --debug build/test_parser_counterexample.txt

    # Valgrindでメモリチェック
    $0 --valgrind build/test_parser_counterexample.txt
EOF
}

BUILD_DIR="build"
DEBUG=false
VALGRIND=false
COUNTEREXAMPLE_FILE=""

# 引数解析
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -d|--debug)
            DEBUG=true
            shift
            ;;
        -v|--valgrind)
            VALGRIND=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            exit 1
            ;;
        *)
            COUNTEREXAMPLE_FILE="$1"
            shift
            ;;
    esac
done

# 反例ファイルの確認
if [ -z "$COUNTEREXAMPLE_FILE" ]; then
    echo -e "${RED}Error: Counterexample file not specified${NC}"
    usage
    exit 1
fi

if [ ! -f "$COUNTEREXAMPLE_FILE" ]; then
    echo -e "${RED}Error: Counterexample file not found: $COUNTEREXAMPLE_FILE${NC}"
    exit 1
fi

# 反例からテスト情報を抽出
echo -e "${GREEN}=== Reproducing PBT Failure ===${NC}"
echo "Counterexample file: $COUNTEREXAMPLE_FILE"

# 反例ファイルから情報を読み取る
TEST_NAME=$(grep "Test:" "$COUNTEREXAMPLE_FILE" | cut -d' ' -f2)
PROPERTY_NAME=$(grep "Property:" "$COUNTEREXAMPLE_FILE" | cut -d' ' -f2)
SEED=$(grep "Seed:" "$COUNTEREXAMPLE_FILE" | cut -d' ' -f2)

if [ -z "$TEST_NAME" ] || [ -z "$SEED" ]; then
    echo -e "${RED}Error: Invalid counterexample file format${NC}"
    echo "Expected format:"
    echo "  Test: <test_name>"
    echo "  Property: <property_name>"
    echo "  Seed: <seed_value>"
    echo "  Data: <counterexample_data>"
    exit 1
fi

echo "Test: $TEST_NAME"
echo "Property: $PROPERTY_NAME"
echo "Seed: $SEED"

# 環境変数の設定
export RAPIDCHECK_REPRODUCE=1
export RAPIDCHECK_SEED=$SEED

# テスト実行ファイルの検索
TEST_EXECUTABLE=""
for exe in "$BUILD_DIR/jwwlib-wasm/tests/pbt/pbt_tests" \
           "$BUILD_DIR/jwwlib-wasm/tests/pbt/pbt_$TEST_NAME" \
           "$BUILD_DIR/tests/pbt/pbt_tests"; do
    if [ -f "$exe" ]; then
        TEST_EXECUTABLE="$exe"
        break
    fi
done

if [ -z "$TEST_EXECUTABLE" ]; then
    echo -e "${RED}Error: Test executable not found${NC}"
    echo "Searched in:"
    echo "  - $BUILD_DIR/jwwlib-wasm/tests/pbt/"
    echo "  - $BUILD_DIR/tests/pbt/"
    exit 1
fi

echo "Executable: $TEST_EXECUTABLE"

# 反例データファイルの作成
TEMP_DATA_FILE=$(mktemp)
grep -A 1000 "Data:" "$COUNTEREXAMPLE_FILE" | tail -n +2 > "$TEMP_DATA_FILE"
export RAPIDCHECK_COUNTEREXAMPLE_FILE="$TEMP_DATA_FILE"

# テストの実行
echo -e "\n${YELLOW}Reproducing failure...${NC}"

if [ "$DEBUG" = true ]; then
    # デバッガの選択
    if command -v gdb &> /dev/null; then
        DEBUGGER="gdb"
        DEBUG_ARGS="--args"
    elif command -v lldb &> /dev/null; then
        DEBUGGER="lldb"
        DEBUG_ARGS="--"
    else
        echo -e "${RED}Error: No debugger found (gdb or lldb)${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Starting debugger: $DEBUGGER${NC}"
    echo "Breakpoint suggestions:"
    echo "  - Set breakpoint at the property check function"
    echo "  - Set breakpoint at RAPIDCHECK_ASSERT"
    echo ""
    
    $DEBUGGER $DEBUG_ARGS "$TEST_EXECUTABLE" --gtest_filter="*${TEST_NAME}*"
elif [ "$VALGRIND" = true ]; then
    if ! command -v valgrind &> /dev/null; then
        echo -e "${RED}Error: Valgrind not found${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Running with Valgrind...${NC}"
    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --verbose \
             "$TEST_EXECUTABLE" --gtest_filter="*${TEST_NAME}*"
else
    # 通常実行
    "$TEST_EXECUTABLE" --gtest_filter="*${TEST_NAME}*" --gtest_color=yes
fi

# クリーンアップ
rm -f "$TEMP_DATA_FILE"

# 結果の表示
if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}Reproduction completed successfully${NC}"
else
    echo -e "\n${RED}Reproduction failed${NC}"
    echo "The failure was successfully reproduced."
fi

# 追加情報の表示
echo -e "\n${GREEN}=== Additional Information ===${NC}"
echo "To minimize the counterexample:"
echo "  1. Modify the test to add shrinking logic"
echo "  2. Use RC_CLASSIFY to categorize the failure"
echo "  3. Add RC_PRE conditions to filter inputs"
echo ""
echo "To fix the issue:"
echo "  1. Analyze the counterexample data"
echo "  2. Identify the edge case or bug"
echo "  3. Fix the implementation"
echo "  4. Re-run the test to verify"