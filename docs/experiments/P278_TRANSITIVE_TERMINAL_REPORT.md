# P #278 — 二 member の source-defined terminal 実験（未採用）

Track: P

**DO NOT MERGE / NONCANONICAL / native C17 未対応。**
`finish_two(TreeTwo) → finish_root(child); finish_root(root)` の二つの
conditional demand を独立した source body から推論し、actual caller/body
へ写像する最小の有限 adapter を実験した。8項目の要求を満たすという
formal heap world や `Matched` fact を definition 時に生成していない。
正例は意味論的に通るが CLI は exit 4 `V1-BACKEND-UNSUPPORTED`。

## 固定 authority と依存

- Compiler/main: `b75baea96a644e68634baee383299b66981b3c62`、canonical Draft17.30。
- FormalProof/main: `08c8b8da4b9dbe5e125e4be0643bffb28294bfad`、F #47 research HOLD。
- 未採用 Draft17.31/DI-015 の今回の固定入力: PR #273 の original head
  `e6b4e98d4f19ed03bdba9a63ff0e86d6b40dc0a2`。並行 M #279 の更新を入力にしない。
- stacked dependency: PR #275 `059b85497862d1461688ec5c6212e5ee87adf94c`
  → PR #277 `8fcda9e07a05626038508427794e4599563cdf5b`
  → fresh branch `p278-transitive-terminal-experiment`。先行 PR は変更しない。
- #278、#276 完了報告と独立裁定、#277 independent code/CI review、#268
  最新裁定、#274、F #43/#47 の未実証 rich adapter 境界を確認した。

三つの独立 flag は default OFF。新しい
`NEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL` には既存
`NEWLANG_EXPERIMENTAL_NESTED_CALLER` と
`NEWLANG_EXPERIMENTAL_ORIGINAL_GRANT` が必要。
新 profile の判定は構造/signature と**実 body**による。`finish_two`、
`LiveRoot`、`TreeTwo` の名前を grant や product policy として扱わない。

## 定義から actual caller への証拠

`src/transitive_terminal.c` は一 nested formal の complete decomposition
と、異なる二 member を一度ずつ渡す known-direct source-defined terminal
call を有限に解析する。保存するのは member index、resolved callee、source
span、および各 callee が独立推論した `NLTypedOwnerDefinition` の conditional
requirements。親の formal は uncorrelated p/A/D であり、具体的 O/R/D や
allocated region、CanEnd、Matched を仮定しない。

single-root definitions → nested definitions → ordinary definitions の順に
独立チェックする。callee の actual body が destroy→erase_slot→finalize→
deallocate を閉じなければ、favorable caller や未使用であっても拒否する。
親で同じ member を二度使えば `P278-DEFINITION-CONSUMED`、正常終了で一つ
失えば `P278-DEFINITION-OBLIGATION`。定義の並び順と alpha 改名も対照にした。

actual call は通常の nonCopy consume、private callee binding、whole
member decomposition を実行し、両方の `finish_root` を**元の LOOSE-only
primitive checker**で順に検査する。元の current typed root、provenance/
readability、同じ原 BackingRegion の Allocation、governing/live Domain、
complete full-range recovery、scope/loan compatibility と責任 consume を
各々要求する。callee の拒否は外側 actual `finish_two(pair)` span に写す。
一 member の成功を他 member の成功として転用しない。後の拒否でも登録
transaction は全体を discard し、snapshot 不変、owned artifact なし。

`NLTwoRootDefinition` と `NLTwoRootCallView` は read-only public observation。
call entry は argument consume **前**、return は actual body と scope exit
**後**の、artifact が所有する別々の snapshot。各 snapshot は元の nominal
anchor に一致しなければ使えない。equal clone の numeric ID 一致は証拠にならない。

owned validator は immutable source plan から member demands を再推論し、
保存 summary と source spans、現在の caller donor、消費済み formal、complete
pattern、二 nested call の actual operand/callee を照合する。全 surviving
Allocation/Domain の current carrier と active loans を走査し、無関係な
carrier を省略しない。この有限 entry observer は active loan を持たない
profile に限る。actual semantic checks の domain conflict と混同しない。

entry aggregate の readonly correlation 検査では actual containment chain
を検査する。primitive 用 `nl_owner_relations` の LOOSE 要求は維持し、
aggregate carrier を書き換えたり、Storage や origin fact を host-seed しない。
source→finish_two→finish_root の owned bodies を 3-frame trace で再生し、
五 original の既存 release machine が原 A/D/typed root と full raw の
一致を各 primitive で再確認する。summary bit や closed final world は
primitive trace の代わりにならない。

## 最重要正例: exact returned whole、after A/C/src

`tests/fixtures/experimental_transitive_returned_whole.nl` は既存 B.next の
write と local domain loan を assembler **前**に終え、named assembler が
返した current `adopted` を分解しないで保持する。A/C/src を先に完全解放し、
唯一の operand `finish_two(adopted)` で元 B と dst を解放する。

| 原物理 owner | src | A | B | C | dst |
|---|---:|---:|---:|---:|---:|
| original R / D | 1 | 2 | 3 | 4 | 5 |
| 解放順の位置 | 3 | 1 | 4 | 2 | 5 |

public observer が見る requiring entry:

- nested member paths は `[1,0]`、packet の ptr R / Allocation R / Domain は
  それぞれ `[5,3]` / `[5,3]` / `[5,3]`。
- original roots `[57,31]`、incarnations `[60,34]` は heap allocation/init
  由来で移転前後不変。current whole は actual returned valueそのもの。
- actual terminal input=124、caller donor=70、callee formal=83。
- live Allocation=2、live Domain=2、active loans=0。A/C/src の root・Domain・
  region はすべて終了済み。古い A head の live predicate は要求しない。
- named callee の primitives による原 full release order は `[2,4,1,3,5]`。
  五原物は一度ずつ解放される。physical failure worlds 0..5 の正しい refunds
  も existing public captured closure observer/validator が検証する。

`experimental_transitive_terminal.nl` は returned whole を一度分解し、
既存 domain loan と B.next write、loan 終了、whole 再構築を行う別正例。
この形でも二原物の原 A/D/root は維持され、current packet 149 を渡す。
ordinary whole-value move/return と解放権限の対応を別々に観測する。

## source 反例と unsupported の区別

[完全な測定 JSON](P278_SOURCE_OBSERVATIONS.json) の36 source は二回ずつ
独立 CLI 実行し、exit と diagnostics が一致、stdout 空、出力 artifact なし。
期待値は runner で固定した。受理を backend/native 成功とは数えない。

| 分類 | 件数 | 結果 |
|---|---:|---|
| semantic accepted / backend unsupported | 9 | exit4 `V1-BACKEND-UNSUPPORTED` |
| genuine semantic reject | 21 | exit3 `error(semantic)` |
| semantic precision unsupported | 4 | exit3 `error(precision)` |
| semantic profile unsupported | 1 | exit3 `error(unsupported)` |
| parser unsupported | 1 | exit3 `AVS-DECL-PROFILE` |

決定的 wrong-A source は original p_B/D_B と Allocation_C を child に入れ、
B.prev/B.next write と pure assembler の ordinary return を行う。A 解放後、
C の Allocation はまだ child 内にある時点で actual `finish_two(pair)` が
**`P193-CALL-BACKING`**、bytes `[13712,13728)` で拒否する。
C の Allocation を使わずに C を先に完全 deallocate した、と偽装しない。
B と C の original regions 3≠4 が原因である。

同じ mixed ordinary return を行い、呼出前に A_C を C 用に戻し、caller が
保持する genuine A_B で B を repack した complete source は通る。public
returned snapshot は ptr regions `[5,3]`、Allocation regions `[5,4]`、
requiring entry は genuine `[5,3]`。matchedness を pure result の型へ貼らない。

wrong Domain_C は actual requiring call の `P193-CALL-DOMAIN`。第二 member
(dst) の wrong A_C / D_C も独立に同じ semantic code で拒否される。
Allocation/Domain の duplication、current child duplication、旧 caller donor
の再使用、terminal result の再使用は `P3-USE-AFTER-CONSUME`。
local D loan / derived H loan を跨ぐ repack/move は **move 自体**の
`P3-REF-CONFLICT`。これを terminal entry span の拒否とは言わない。
actual original B を EndRoot してから stale pointer を詰めた source、および
already-dead original A ptr は requiring call の `P193-CALL-ROOT`。
wrong full-raw symbolic operand、旧 A-head primitive、未使用の壊れた root
terminal も独立 definition semantic reject。

whole record の `loan_read(pair)` は `LOCAL-LOAN-PROFILE` unsupported。
wrapper branch、explicit return、third transitive layer、temporary terminal
operand は `P278-MEMBER-PATH-PRECISION` / `P278-CALL-ARGUMENT` unsupported。
特に branch で一 owner を落とす source は今回 semantic oracle を得ていない。
正常終了での lost member semantic reject と区別する。

H0 も継続: B を caller の別 nonCopy packet に保持し、dst.child に Copy
ptr_B を入れ、result に dst+C を返し、caller の B と named two-member
terminal の dst+C を正しく解放する source は core-safe。public result の
H1 dst+B member predicate は false。この特定 product 条件を一般安全則にしない。

[固定12 source対照](P278_FIXED_CONTROLS.json): 正例・wrong A・mixed repair・
exact returned whole を固定main / #275 opt-in / #277 headで実行。main/#275
は parser unsupported、#277 はすべて `P276-TERMINAL-SUMMARY-PRECISION`。
従って旧 profile の拒否を今回の semantic 拒否の安全証拠に数えない。

## falsifiers / 回帰 / 再実行

33 actual owned artifact poison attacks: entry/return の equal clone・wrong
world、donor/formal/input/current carrier、original region/Domain/incarnation、
unrelated C carrier の復活、active loan、nested body/source substitution、
whole pattern/consume、member summary、各 nested primitive の削除・wrong raw を
拒否し、復元後には validator が通る。no backend artifact。
source registration/checking、owned entry/return snapshots と public validator
の allocation-failure sweeps は OOM を漏れなく伝え、失敗 transactionを rollback。

四構成(default249 / #275-only250 / #277254 / new258)を GCC Debug/Release、
Clang、ASan/LSan、UBSan で実行する。先行22/38 source対照も含む。
new flag時だけ旧 `named-finish-two-summary` の期待を accepted/backend
unsupported に変え、先行 profileでは precision の期待を保持する。
exact-head CI と最終ローカル実測の状態/リンクは Issue #278 と実験 PR に記録する。
format / diff checks を含む。C emitter/layout に変更はない。

```sh
. /workspace/NewLang_Compiler/.deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DNEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON \
  -DNEWLANG_EXPERIMENTAL_NESTED_CALLER=ON \
  -DNEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 tests/integration/transitive_terminal_test.py \
  build/newlangc build/transitive_terminal_evidence build/captured_closure_test \
  /tmp/p278-observations
```

## Historical gate と最小未実証境界

Process §4.2、Design Decision Procedure、Testing Strategy、canonical
§§13.5/14.5/16/18.1a–c/18.6–8/26–27、DI-011/012/013/014、frozen candidate
DI-015/#272/#273、#274/#276 と independent reviews を照合。
**KEEP** 明示的 A/Storage/slot/D、原物理root identity、source-body applicability、
ordinary nonCopy/scope/placement semantics、既存 bounded LiveTail、coreとH1の区別。
**DEFER** canonical ordinary-record source expansion、branch/recursive/general
call summaries、field loans/generic owners、RAII、native ABI/layout と full cJSON。
旧 surface Draft1/1.1、module/private receiver/M0 sketch は source experimentsであり、
required A/D/typed destroy を自動的に証明しない。旧 full M9 transcript は
今回網羅監査していない。normative selection なしにつき Ledger は編集しない。

source boundaryの最初の未対応は wrapper の branch/refund/general member-path
flow（`P278-MEMBER-PATH-PRECISION`）。今回の plain unit fallthrough、exact二 call
以上は推論していない。Unknown を matched として通す bypass はない。

**最初の rich F0/F1 blocker** は、この source-derived current aggregate / actual
call / per-root destroy→typed empty slot→full raw→original deallocation の adapter
を accepted rich formal interfaces と結ぶ refinement theorem。F #47 の conditional
projection や今回 C predicate を、その accepted Lean proof と同一視しない。
full cJSON 四detach/二adopt Changes、native C17、H1 product PASSは未実装/未証明。

`P CJSON-B-STATE-1 TRANSITIVE TWO-ROOT TERMINAL: EMPIRICAL CANDIDATE — TRUE SOURCE-CALL REQUIREMENTS VERIFIED, DRAFT UNSELECTED`
