# Track T — 開発基盤・Tooling 運用Charter（暫定）

> **Status: PROVISIONAL / NON-NORMATIVE — Coordination Issue #426**
>
> 本文書は[開発運用方針](NewLang_Project_Development_Process.md)を補う、開発基盤専用Trackの小さな責務定義である。review後mainへmergeされた時点で運用上有効となる。言語仕様・安全性・製品戦略の正本ではなく、[CURRENT_SPEC](reference/CURRENT_SPEC.md)が選ぶDraftを変更しない。本文書だけで継続実行・Issue開始・PR mergeを許可しない。

## 1. 目的

**Track T（Tooling / Infrastructure）**は、Compiler開発者が信頼できる環境を再現し、限られた時間をProductionのsource/checker/backendとCoordinationの方向付けへ集中できるようにする、**Issue単位の暫定実行Track**である。

- 対象：GitHub Actionsの実行安定性、固定・検証済み依存パッケージの取得とキャッシュ、rootless bootstrap、build/testの起動手順、CI失敗原因の特定、開発環境README、狭い開発庶務。
- 成果指標：実際に減った環境由来の失敗・手作業再実行・セットアップ負担、正しい失敗の見分けやすさ、再現可能性、導入した保守費用。単にCI件数・ジョブ数・テスト数を増やさない。
- **常時監視・定例報告・自動Issue起票はしない。** Coordinationが個別の明示的Issueと上限を設ける場合だけ起動する。必要な作業がなければ沈黙する。

## 2. 他Trackとの明確な境界

| 主体 | 主責務・境界 |
| --- | --- |
| **T** | 承認済みの小規模インフラ作業の調査、提案、必要なら隔離PRでの実装・実測。全作業はIssueで固定authorityと停止条件を確認。 |
| **P** | コンパイラ本体、source/semantic/checked-owned/backend/ABI、正規のcompilerテストの意味・合格基準。Tがbuild/CTest登録・テスト契約に影響する場合はPとscopeを調整。 |
| **Coordination** | 優先順位、費用・信頼境界、他Trackとの分担、GO/HOLD、最終review、merge/close、停止・再割当ての裁定。Tの報告は自動的な承認ではない。 |
| **R / F / M / V / O / Q** | 既存の独立性・形式証明・設計・製品価値・モデル研究・品質提案の責務を維持。特にfresh BLIND Rの独立入力・CI branchをTが改変・公開しない。Qのリスク提案をTの自動タスクに変換しない。 |

Tは仕様Draft、Design-Intent Ledger、製品投資方針、compilerの公開証明・権限issuer、動作を変えるsource規則を単独で変更しない。Productionに関わる変更を「庶務」と呼んでP/R/Fの必要な検証を迂回しない。

## 3. 依存関係・CIの信頼境界

- 既存のreview済みtoolchain lock、LLVM/version、HTTPS/TLS、各package SHA-256、署名済みprovenance、検証後のみ展開する順序、fail-closedなエラーを守る。
- CI cacheは**信頼済みの実行結果ではなく、再検証が必要な外部bytes**として扱う。lockとenvironmentを取り違えず、復元したpackageも実際にSHAを再計算する。cache hitだけで成功・正当性・provenanceを主張しない。
- Actionsを追加するときはpinされたcommit SHA、PR/forkの信頼境界、cache read/write、権限、保存容量、消去・復旧、保守費用をIssue/PRに明記する。未review mirror、hash緩和、TLS無効化、権限拡大、秘密情報やアカウント設定の変更はこのCharterでは許可しない。
- 最初の外部URL404、失敗ジョブのみのrerun、全構成の初回success、コード不正、テスト異常、環境ブロッカーを区別し、実際のrun/attempt/HEADを添えて報告する。最終runの緑だけで初回の失敗を消さない。

## 4. 起動・レビュー・停止

1. **Issue-first**：目的と実測された問題、現在のmainと固定authority、変更可能path、代替案、予想と実測の区別、完了条件・上限・STOPを事前登録。作業開始時に[Compass](NewLang_Project_Compass.md)から関連正本を確認する。
2. 初回はread-onlyまたは狭いpreflightを優先。実装するときは専用branchとPRに固定SHA・差分・検証方法を保存する。workflow/依存lock変更は通常の品質gateと別に、信頼境界を明記してCoordinationがreviewする。
3. STOP：信頼検証の弱化、別の大規模compiler機構、複数の広い改修、許可外のcredential/権限変更、基礎となるfixed SHAや前提の不一致、実測上の効果が小さく維持費が大きい場合。作業者は結果を一度だけIssueへ報告し、別Trackの起動・merge・自己closeはしない。
4. P等の稼働中Issueについては[開発運用方針 §4.1.1](NewLang_Project_Development_Process.md)の停止・ACK規律を優先する。**Issue上に着手コメントがないことは未着手の証拠ではない。** 別Trackへ移す場合、作業中かもしれない担当者へ先に明示し、停止確認前の並行書込みをしない。

## 5. 起点・暫定運用上の裁定

[Issue #417](https://github.com/wakairo/NewLang_Compiler/issues/417)／[PR #418](https://github.com/wakairo/NewLang_Compiler/pull/418)の限定HTTP404再試行と、[PR #424](https://github.com/wakairo/NewLang_Compiler/pull/424)のREADME更新時に観測した同一locked URL取得失敗から、基盤庶務をProductionと分担する根拠が生じた。**PR #424 postmerge main CI #38496970332 は成功（attempt2、初回のGCC・ASanはHTTP404失敗）**。

[Issue #425](https://github.com/wakairo/NewLang_Compiler/issues/425)は既にPへ固定authorityで割当て済みのread-onlyキャッシュ費用・リスク評価であり、**本Charter成立だけでPからTへ移管しない**。その最終報告とCoordination裁定まではTによる競合キャッシュ実装・別評価を起動しない。次のT実務は必要性が確認できた場合の**別Issue・明示起動**から始める。

Tの継続や常設化・廃止はCoordinationが有益な実績と運用費をもとに判断し、必要なら本CharterとProcessの小さなreviewed PRで更新する。独立repositoryや追加審査組織は設けない。
