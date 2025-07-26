<instructions>
spec-driven developmentを実行してください。以下の構造化されたフェーズに従って段階的に開発を進めます。
</instructions>

<methodology>
## spec-driven developmentとは

Spec-driven developmentは、仕様を軸とした段階的な開発手法です。要件から設計、実装まで一貫した流れで進める5つのフェーズで構成されています。
</methodology>

<development_flow>
## 開発フロー

<phase name="preparation" number="1">
### 事前準備フェーズ

<objective>タスクの概要を整理し、作業環境を準備します。</objective>

<tasks>
* ユーザーがClaude Codeに実行したいタスクの概要を伝える
* `mkdir -p ./.cckiro/specs` を実行する
* タスク概要から適切なspec名を決定し、ディレクトリを作成する
* 以降の成果物はすべてこのディレクトリ内に作成する
</tasks>

<example>
「記事コンポーネントを作成する」というタスクの場合：
`./cckiro/specs/create-article-component` ディレクトリを作成
</example>
</phase>

<phase name="requirements" number="2">
### 要件フェーズ

<objective>タスクの要件を明確に定義します。</objective>

<workflow>
1. Claude Codeがタスク概要から要件ファイルを作成する
2. 作成した要件ファイルをユーザーに提示する
3. ユーザーの確認とフィードバックを受ける
4. 合意が得られるまで要件ファイルを修正する
</workflow>

<deliverable>要件ファイル（requirements.md）</deliverable>
</phase>

<phase name="design" number="3">
### 設計フェーズ

<objective>要件を満たす設計を策定します。</objective>

<workflow>
1. Claude Codeが要件ファイルに基づいて設計ファイルを作成する
2. 作成した設計ファイルをユーザーに提示する
3. ユーザーの確認とフィードバックを受ける
4. 合意が得られるまで設計ファイルを修正する
</workflow>

<deliverable>設計ファイル（design.md）</deliverable>
</phase>

<phase name="implementation_plan" number="4">
### 実装計画フェーズ

<objective>設計を実装するための計画を立案します。</objective>

<workflow>
1. Claude Codeが設計ファイルに基づいて実装計画ファイルを作成する
2. 作成した実装計画ファイルをユーザーに提示する
3. ユーザーの確認とフィードバックを受ける
4. 合意が得られるまで実装計画ファイルを修正する
</workflow>

<deliverable>実装計画ファイル（implementation-plan.md）</deliverable>
</phase>

<phase name="implementation" number="5">
### 実装フェーズ

<objective>計画に基づいて実装を行います。</objective>

<constraints>
* 実装計画ファイルに従って実装を開始する
* 要件ファイルと設計ファイルの内容を遵守しながら実装する
</constraints>

<deliverable>実装されたコードとドキュメント</deliverable>
</phase>
</development_flow>
