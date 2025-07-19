# jwwlib-wasm

ブラウザでJWW（JW_CAD）ファイルを読み込むためのWebAssemblyライブラリ

## 概要

jwwlib-wasmは、LibreCADのjwwlibからJWWファイル読み込み機能をWebAssemblyに移植したライブラリです。ウェブブラウザ上で直接JWWファイルを解析し、図形データを抽出することができます。

## 特徴

- JWW（JW_CAD）形式のファイルを読み込み
- 図形エンティティの抽出：
  - 線分
  - 円
  - 円弧
  - （今後さらに多くの図形タイプをサポート予定）
- 完全なクライアントサイド処理
- サーバー依存なし
- シンプルなJavaScript API

## インストール

### npmを使用

```bash
npm install jwwlib-wasm
```

### CDNを使用

```html
<script src="https://unpkg.com/jwwlib-wasm/dist/jwwlib.js"></script>
```

## クイックスタート

```javascript
import { JWWReader } from 'jwwlib-wasm';

// WASMモジュールの初期化
await JWWReader.init();

// JWWファイルの読み込み
const fileBuffer = await fetch('example.jww').then(r => r.arrayBuffer());
const reader = new JWWReader(fileBuffer);

// すべてのエンティティを取得
const entities = reader.getEntities();

// エンティティの処理
entities.forEach(entity => {
  switch(entity.type) {
    case 'LINE':
      console.log(`線分: (${entity.x1}, ${entity.y1}) から (${entity.x2}, ${entity.y2})`);
      break;
    case 'CIRCLE':
      console.log(`円: 中心 (${entity.cx}, ${entity.cy}) 半径 ${entity.radius}`);
      break;
    case 'ARC':
      console.log(`円弧: 中心 (${entity.cx}, ${entity.cy})`);
      break;
  }
});
```

## ソースからのビルド

### 前提条件

- CMake 3.10以上
- Emscripten SDK
- Node.js 14以上

### ビルド手順

```bash
# リポジトリのクローン
git clone https://github.com/f4ah6o/jwwlib-wasm.git
cd jwwlib-wasm

# 依存関係のインストール
npm install

# WASMモジュールのビルド
npm run build:wasm

# JavaScriptラッパーのビルド
npm run build

# テストの実行
npm test
```

## サンプル

[examples](examples/)ディレクトリには以下のサンプルがあります：
- 基本的な使用例
- JWWファイルの可視化ギャラリー
- Canvas描画との統合例

## API ドキュメント

### `JWWReader.init()`
WebAssemblyモジュールを初期化します。リーダーインスタンスを作成する前に必ず呼び出す必要があります。

### `new JWWReader(buffer)`
JWWファイルバッファで新しいリーダーインスタンスを作成します。

### `reader.getEntities()`
JWWファイルからすべての図形エンティティを取得します。

### `reader.getHeader()`
ファイルヘッダー情報を取得します。

## ライセンス

このプロジェクトはGNU General Public License v2.0でライセンスされています。詳細は[LICENSE](LICENSE)ファイルをご覧ください。

このソフトウェアには以下から派生したコードが含まれています：
- [LibreCAD](https://github.com/LibreCAD/LibreCAD) - フリーでオープンソースのCADアプリケーション
- Rallazによるオリジナルのjwwlib（LibreCADプロジェクトの一部）

## 貢献

貢献を歓迎します！プルリクエストをお気軽に送信してください。

## ロードマップ

- [ ] テキストエンティティのサポート
- [ ] 寸法のサポート
- [ ] ブロック/グループのサポート
- [ ] DXFエクスポート機能
- [ ] メモリ効率の改善
- [ ] 完全な文字エンコーディングサポート

## 謝辞

- オリジナルのjwwlib実装を行った[LibreCAD](https://github.com/LibreCAD/LibreCAD)チーム
- オリジナルのdxflibプロジェクトを作成したRibbonSoft
- ファイル形式のドキュメントを提供してくれたJW_CADコミュニティ