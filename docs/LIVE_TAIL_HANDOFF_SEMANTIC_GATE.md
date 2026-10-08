# Live-tail handoff — bounded production semantic gate

Track: P。対象は [Issue #193](https://github.com/wakairo/NewLang_Compiler/issues/193) の
[再開裁定](https://github.com/wakairo/NewLang_Compiler/issues/193#issuecomment-6058786761)。
base main は `90bfaca797e55edbcd6ecd0b26383929c25d8605`、canonical は
[Draft 17.27](reference/NewLang_v0_spec_Draft17_27.md) §18.1a。
PR #195 の独立 Coordination ACCEPT を前提とする。仕様変更はない。

Historical design audit: N/A — exact §18.1a faithful implementation; no language rule changed.
DI-011 は採用済みであるため status-only 修正を含む。DI-009/010/007/006/001/002
の bounded authority / projection / provenance / product 境界を維持する。
Ledger を normative authority として使用しない。

## 独立した definition-time checking

`src/typed_owner.c` は一つの completed recursive H に対する
`(ptr<H>, Allocation, LifetimeDomain) -> unit` receiver の有限な条件付き検査である。
parameter は互いに未対応の symbolic role として開始する。型から具体的な
Place、BackingRegion、LifetimeDomain、Storage や allocation authority を生成しない。
callee が呼ばれなくても定義を検査する。receiver 定義を全 caller 定義に先立って
検査するので、source declaration order に依存しない。

許可する trace は optional scoped read reloan、exclusive EndRoot、同じ H の
empty slot の erase、domain finalize、original Allocation/full raw の deallocate。
immutable local alias は role と affine availability を保存する。normal exit は unit、
すべての非Copy責任を消費済みとする。loan から ref/authority を逃がさない。
新しい allocation、forwarding、recursion、一般的な条件分岐/effect inference は対象外。
unsupported operation は `P193-DEFINITION-PROFILE` の structured precision rejection。
128 symbolic binder budget は resource rejection。通常の関数検査と既存 authority guard は維持する。

成功証明 `NLTypedOwnerDefinition` は H 型 identity、RequiredAtEntry の有限集合、
relative lifecycle trace のみを保持する。`definition_checked` は実際の O/R/D の
対応条件が既に証明済みであることを意味しない。

## known direct call での証明

既存の left-to-right argument checker が ptr を Copy、Allocation/Domain を consume
した private transaction で、callee parameter 作成前に毎回 applicability を検査する。

- exact current root incarnation、valid provenance、read access、completed H。
- root の BackingRegion と actual Allocation の region の同一性。
- actual Domain value の live D と root を govern する D の同一性。
- full range / size / alignment / ordinary access と raw recovery 条件。
- input の正確な dependency facts。Unknown / may-set は証明に使わない。
- 既存 occupancy validator、scope conflict、surviving value/occurrence blocker 検査。
- domain が別の surviving independent root を govern していないこと。

`nl_owner_relations` は read-only な relation predicate。
production wrapper `owner_entry` が unique occupancy と全 surviving blocker を追加検査する。
型一致だけで対応を仮定せず、receiver の有限条件が成立しない call は実行前に拒否する。
現在の実装は closed two-H main world 内の、一つの path artifact ごとの一 call に限定する。
これを一般 language restriction へ昇格しない。範囲外は analysis-precision rejection。

## transfer と owned evidence

callee は新しい parameter binding を持つが、受け取る original ValueId / R / D は変えない。
new O / backing は生成しない。callee body は既存 checker の primitive と scope 検査で再検査する。
caller の donor A/D binding は既に Consumed、callee の normal post-state は original O の
EndRoot、same full R の recovery/deallocation、same D の finalize、両 owner の consumed を要求する。

`owner_call` は definition、entry/post proof、三 actual value / donor / parameter、O の
Place/incarnation、R/range、D を保持する。`nl_checked_owner_entry` は argument 評価直後・
callee effect 前の actual state を別途 owned snapshot として保持する。
callee body と call は同じ artifact world、match arm は各 owned branch world に属する。
branch-local の数値 ID を public context へ持ち出さず、別世界の数値一致で対応を証明しない。
AST / original source teardown 後にも各証拠を検査できる。

## source controls と 3 world

`tests/fixtures/live_tail_handoff.nl` は Issue #192 の二関数候補を使用する
（説明用 full-line comments を除く）。head link の Some(tail ptr) → None、Option Copy、
tail 自身の D による reloan、unlinked live tail の receiver transfer、head cleanup を含む。

- first None: second trial / receiver / release なし。
- first Some, second None: head の明示的な EndRoot / release 一回。
- both Some: independent R/D の二 root。receiver が tail を release、donor が head を release。

unit evidence は conditional definition の未対応性、actual entry の同時生存二 root、
fresh frame / same value-owned identity、consumed donors、same root EndRoot と same R の full raw/free、
exact closed post-state を検査する。primary と renamed H/fields/binders、forward declaration、
caller/callee aliases、optional read なし、reversed nested arm の 8 actual-source positives がある。
21 destructive source controls と uncalled invalid-definition controls は、同型の三 mismatch、
good branch 後の bad call、owner reuse / repeated call、stale ptr、active loan、definition leak / order /
wrong slot / escape 等を拒否する。source-derived snapshot の 11 mutation controls は
Unknown/may-set/dependency、partial/wrong range、stale incarnation、wrong region/domain 等を拒否する。

## rollback / backend boundary / verification

registration、caller clone、callee binding/body、entry snapshot、branch/captured-state join は
既存 transaction 内でのみ行う。失敗は artifact と private world を破棄する。
malloc/realloc fault injection は registration 1,061 箇所、entry checking 740 箇所を通し、
各 OOM 後の snapshot 不変、output NULL、clean retry を検査する。

cross-actor accepted source の CLI 結果は exit 4 `V1-BACKEND-UNSUPPORTED`、stdout/C/executable なし。
新しい native emitter / allocator / observer を実装しない。既存 two-H native と全 legacy native は
回帰対象に含む。observer、oracle adapter、canonical Draft、toolchain lock は変更しない。

再実行:

```sh
. .deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
ctest --test-dir build -R typed_owner --output-on-failure
bash scripts/check-format.sh
```

required validation は全 213 CTests、GCC Debug、GCC Release/NDEBUG、Clang+format、ASan、UBSan。
exact-head CI evidence は Issue/PR の handoff report を正とする。

## findings / stop

新しい spec hole / ambiguity は発見していない。上記 finite receiver / exact world / symbolic budget
の制約は COMPILER-IMPLEMENTATION-LIMIT / COMPILER-PRECISION として明示的に保守的拒否する。
一般 owner/effect system、cross-actor native、third Node、traversal、FFI、LLVM、cJSON は未実装。
独立 Coordination review に OPEN / unmerged PR を渡し、Issue #193 の stop condition で停止する。
