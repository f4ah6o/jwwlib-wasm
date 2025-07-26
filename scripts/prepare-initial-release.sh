#!/bin/bash

# 初回リリース準備スクリプト

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== jwwlib-wasm 初回リリース準備 ===${NC}"
echo ""

# 1. Git状態の確認
echo -e "${YELLOW}1. Git状態の確認...${NC}"
if [ -n "$(git status --porcelain)" ]; then
    echo -e "${RED}エラー: コミットされていない変更があります${NC}"
    echo "以下のコマンドで確認してください:"
    echo "  git status"
    echo "  git add ."
    echo "  git commit -m 'feat: initial npm package setup for jwwlib-wasm'"
    exit 1
fi

# 2. NPMログイン確認
echo -e "${YELLOW}2. NPMログイン状態の確認...${NC}"
NPM_USER=$(npm whoami 2>/dev/null || echo "")
if [ -z "$NPM_USER" ]; then
    echo -e "${RED}エラー: NPMにログインしていません${NC}"
    echo "  npm login"
    exit 1
fi
echo -e "${GREEN}✓ NPMユーザー: $NPM_USER${NC}"

# 3. GitHub Secrets確認
echo -e "${YELLOW}3. GitHub Secrets確認...${NC}"
if gh secret list | grep -q "NPM_TOKEN"; then
    echo -e "${GREEN}✓ NPM_TOKEN が設定されています${NC}"
else
    echo -e "${RED}警告: NPM_TOKEN が設定されていません${NC}"
    echo "GitHub Actionsでの自動公開には必要です"
fi

# 4. パッケージ内容の確認
echo -e "${YELLOW}4. パッケージ内容の確認...${NC}"
echo "以下のファイルがパッケージに含まれます:"
npm pack --dry-run 2>&1 | grep -E "^\s+[0-9.]+[kmg]?B\s+" | head -20

# 5. リリース方法の選択
echo ""
echo -e "${GREEN}リリース方法を選択してください:${NC}"
echo "1) 手動でnpm publish (推奨 - 初回)"
echo "2) GitタグでGitHub Actions経由"
echo "3) キャンセル"
read -p "選択 (1/2/3): " choice

case $choice in
    1)
        echo ""
        echo -e "${YELLOW}手動でのnpm公開を実行します...${NC}"
        echo "実行するコマンド:"
        echo -e "${GREEN}npm publish --access public${NC}"
        echo ""
        read -p "続行しますか？ (y/n): " confirm
        if [[ "$confirm" == "y" ]]; then
            npm publish --access public
            echo ""
            echo -e "${GREEN}✅ 公開が完了しました！${NC}"
            echo ""
            echo "確認コマンド:"
            echo "  npm view jwwlib-wasm"
            echo "  npm install jwwlib-wasm"
        fi
        ;;
    2)
        echo ""
        echo -e "${YELLOW}Gitタグによる自動リリースを準備します...${NC}"
        echo "実行するコマンド:"
        echo -e "${GREEN}git tag v0.1.0 -m 'Initial release'${NC}"
        echo -e "${GREEN}git push origin v0.1.0${NC}"
        echo ""
        read -p "続行しますか？ (y/n): " confirm
        if [[ "$confirm" == "y" ]]; then
            git tag v0.1.0 -m "Initial release"
            git push origin v0.1.0
            echo ""
            echo -e "${GREEN}✅ タグがプッシュされました！${NC}"
            echo ""
            echo "GitHub Actionsの状態を確認:"
            echo "  gh run list --workflow=release.yml"
            echo "  gh run watch"
        fi
        ;;
    3)
        echo "キャンセルしました"
        exit 0
        ;;
    *)
        echo "無効な選択です"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}=== 次のステップ ===${NC}"
echo "1. パッケージページを確認: https://www.npmjs.com/package/jwwlib-wasm"
echo "2. GitHubリリースを確認: https://github.com/f4ah6o/jwwlib-wasm/releases"
echo "3. READMEにバッジを追加:"
echo "   [![npm version](https://badge.fury.io/js/jwwlib-wasm.svg)](https://www.npmjs.com/package/jwwlib-wasm)"
echo "   [![GitHub release](https://img.shields.io/github/release/f4ah6o/jwwlib-wasm.svg)](https://github.com/f4ah6o/jwwlib-wasm/releases)"