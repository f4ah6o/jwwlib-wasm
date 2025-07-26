#!/bin/bash

# NPMトークン設定スクリプト

echo "=== NPMトークンのGitHub Secrets設定 ==="
echo ""
echo "1. まず、NPMトークンを生成してください："
echo "   - https://www.npmjs.com/ にアクセス"
echo "   - ログイン後、アカウントアイコン → Access Tokens"
echo "   - 'Generate New Token' → 'Classic Token'"
echo "   - Type: 'Automation' を選択"
echo "   - トークンをコピー（npm_で始まる文字列）"
echo ""
echo "2. トークンを取得したら、以下のコマンドを実行："
echo "   gh secret set NPM_TOKEN"
echo "   （プロンプトが表示されたらトークンを貼り付けてEnter）"
echo ""
echo "3. または、以下のコマンドで直接設定："
echo "   gh secret set NPM_TOKEN --body='YOUR_NPM_TOKEN_HERE'"
echo ""
read -p "準備ができたらEnterキーを押してください..."

# GitHub CLIがインストールされているか確認
if ! command -v gh &> /dev/null; then
    echo "Error: GitHub CLI (gh) がインストールされていません"
    echo "インストール: brew install gh"
    exit 1
fi

# GitHub認証状態を確認
if ! gh auth status &> /dev/null; then
    echo "GitHub CLIの認証が必要です。以下を実行してください："
    echo "gh auth login"
    exit 1
fi

echo ""
echo "GitHub Secretsを設定しますか？ (y/n)"
read -r response

if [[ "$response" == "y" ]]; then
    echo "NPMトークンを入力してください（表示されません）："
    gh secret set NPM_TOKEN
    
    echo ""
    echo "設定されたシークレット："
    gh secret list
    
    echo ""
    echo "✅ NPM_TOKENの設定が完了しました！"
else
    echo "手動で設定する場合は、以下のURLから設定してください："
    echo "https://github.com/f4ah6o/jwwlib-wasm/settings/secrets/actions"
fi