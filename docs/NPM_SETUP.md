# NPM公開とGitHub設定ガイド

## 1. NPMアカウントの準備

### NPMアカウントの作成（既にある場合はスキップ）
```bash
# NPMアカウントを作成
npm adduser

# または既存アカウントでログイン
npm login
```

### 現在のログイン状態を確認
```bash
npm whoami
```

## 2. NPMトークンの生成

### 方法1: npmコマンドを使用
```bash
# 公開用のトークンを生成
npm token create --read-only=false

# 出力例:
# ┌────────────────┬──────────────────────────────────────┐
# │ token          │ npm_xxxxxxxxxxxxxxxxxxxxxxxxxxxxx    │
# │ cidr_whitelist │                                      │
# │ readonly       │ false                                │
# │ automation     │ false                                │
# │ created        │ 2024-01-26T10:00:00.000Z            │
# └────────────────┴──────────────────────────────────────┘
```

### 方法2: NPM Webサイトから生成
1. https://www.npmjs.com/ にログイン
2. アカウントアイコン → Access Tokens
3. "Generate New Token" → "Classic Token"
4. Type: "Automation" を選択
5. トークンをコピー

## 3. GitHub Secretsの設定

### gh CLIを使用した設定
```bash
# リポジトリのシークレットとして設定
gh secret set NPM_TOKEN --body="npm_xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

# 設定確認
gh secret list
```

### GitHub Web UIから設定
1. リポジトリページ → Settings → Secrets and variables → Actions
2. "New repository secret" をクリック
3. Name: `NPM_TOKEN`
4. Secret: npmトークンを貼り付け
5. "Add secret" をクリック

## 4. 初回リリースの準備

### パッケージ名の確認
```bash
# パッケージ名が利用可能か確認
npm view jwwlib-wasm

# もし既に存在する場合は、package.jsonの名前を変更
```

### ローカルでのビルド確認
```bash
# 依存関係のインストール
pnpm install

# ビルドの実行
pnpm run build

# パッケージの作成（公開はしない）
npm pack --dry-run
```

### パッケージ内容の確認
```bash
# 実際にパッケージに含まれるファイルを確認
npm pack
tar -tzf jwwlib-wasm-*.tgz | head -20

# 不要なファイルが含まれていないか確認
# 必要に応じて.npmignoreを調整
```

## 5. 初回リリースの実行

### 方法1: 手動でのnpm公開（推奨）
```bash
# 初回は手動で公開（アクセス権限の確認）
npm publish --access public

# 公開後の確認
npm view jwwlib-wasm
```

### 方法2: GitHubタグによる自動リリース
```bash
# バージョンタグを作成
git tag v0.1.0 -m "Initial release"

# タグをプッシュ（GitHub Actionsがトリガーされる）
git push origin v0.1.0

# GitHub Actionsの実行状況を確認
gh run list
gh run watch
```

## 6. リリース後の確認

### NPMでの公開確認
```bash
# パッケージ情報の確認
npm info jwwlib-wasm

# インストールテスト
npm install jwwlib-wasm@latest
```

### GitHub Releasesの確認
```bash
# リリース一覧
gh release list

# 最新リリースの詳細
gh release view v0.1.0
```

## トラブルシューティング

### NPM公開エラー: 403 Forbidden
- NPMトークンの権限を確認
- パッケージ名が既に使用されていないか確認
- npmログイン状態を確認: `npm whoami`

### GitHub Actions失敗
```bash
# ワークフロー実行状況の確認
gh run list --workflow=release.yml

# 失敗したrunの詳細
gh run view [RUN_ID]

# ログの確認
gh run view [RUN_ID] --log
```

### セマンティックリリースが動作しない
- コミットメッセージがConventional Commits形式か確認
- mainブランチへの直接プッシュか確認
- NPM_TOKENが正しく設定されているか確認

## 参考リンク
- [npm Docs - Creating and publishing scoped public packages](https://docs.npmjs.com/creating-and-publishing-scoped-public-packages)
- [GitHub Docs - Creating encrypted secrets for a repository](https://docs.github.com/en/actions/security-guides/encrypted-secrets)
- [semantic-release Documentation](https://semantic-release.gitbook.io/semantic-release/)