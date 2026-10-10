# P #281 — original FIVE-H detach/adopt source gate: HOLD

Track: P

**EXPERIMENTAL / DO NOT MERGE / 非canonical。** 優先1の実 source は
`TreeFour` 第3フィールドで parser UNSUPPORTED。actual detach/adopt の
所有責任・Change/Reset・refusal は検査に到達しない。core を拡張せず、
Issue #281 が明示した最初の具体的 source blocker で停止する。

## Authority / historical audit

- Compiler/main `b75baea96a644e68634baee383299b66981b3c62`、
  `CURRENT_SPEC.md` canonical Draft17.30 §3.2b。
- fresh `p281-five-root-source-gate` の基点は実験 #280 exact head
  `4bc6b690e87b4646596dfd460bea26e9a5d8dac5`。
- Draft17.31 候補 #273 exact head
  `f778eba116d1834cc79c0bef6f6c17051102049f` §3.2c は UNSELECTED。
- FormalProof/main `08c8b8da4b9dbe5e125e4be0643bffb28294bfad`。
  F43/F47 の source/current-carrier/rich refinement は HOLD。
  並行 F49 の結果を仮定しない。
- #281、#268 最新 Semantic Sync、#280 独立レビュー、先行 P274/P276/P278
  の committed reports、Process §2/§4.2/§6/§9、Design Decision Procedure
  Gates A–D/E、canonical §3.2b、候補 §3.2c、Testing Strategy、North Star
  Product Validation を確認した。

Process §4.2: **N/A（compiler/source/API/semantic 選定を変更しない測定）**。
DI-011 の独立 body/actual caller 条件、DI-012/013 の旧二-H LiveTail
限定、DI-014 の五-H PRE-detach substrate を KEEP。DI-015 候補は #281/#268
指定の未採用入力として扱う。旧 §3.2b の6書き込みと §18.1a–c の
二-H条件を五-H detach/attach 成功へ自動拡張しない。一般 Owner、generic、
privacy、ABI/FFI、actor は DEFER。旧 M9 chat 全文は網羅監査していない。
Ledger/仕様/先行 PR は変更しない。今回の unsupported は将来の一般
owner source を禁止する normative 判断でも core 矛盾でもない。

## Actual complete input / first refusal

入力は `tests/fixtures/experimental_five_root_detach_attach.nl`。
SHA256 `8763eb98f68d502b8a26b49d8a023ba38dcce45090253bf2c6695fc5391d3512`。
実際の五つの独立した `try_allocate_one<Node>()` と全 None cleanup を
先行の完全 source から保持し、named detach/attach body と caller を記述した。
source に allocation site はちょうど5、`replace` はちょうど12。
**これは入力の静的数え上げであり、checked physical Change trace ではない。**

```text
complete-five-root-detach-attach.nl:10:59-10:67:
error(unsupported)[AVS-DECL-PROFILE]:
only two-u8 or exact Option<ptr<H>>,u8 profile
```

CLI exit **3**。public parser は `NL_PARSE_SYNTAX_UNSUPPORTED`（2）、
span `[319,327)` = TreeFour 第3フィールドの `LiveRoot`。
syntax tree は NULL、function-unit registration は未実行、context snapshot
不変、owned entry artifact はなし、backend 未到達、生成 C なし。
同じ入力の CLI を2回実行し exit/stdout/stderr は一致した。
これは **semantic rejection ではない**。

### 六 + 四 + 二の入力箇所と未到達範囲

| 段階 | 元 physical node の intended field write | source 行 | checked trail |
|---|---|---:|---|
| initial 1 | src.child=A | 229 | 未到達 |
| initial 2 | A.prev=C | 234 | 未到達 |
| initial 3 | A.next=B | 239 | 未到達 |
| initial 4 | B.prev=A | 244 | 未到達 |
| initial 5 | B.next=C | 248 | 未到達 |
| initial 6 | C.prev=B | 252 | 未到達 |
| detach 1 | A.next=C | 23 | 未到達 |
| detach 2 | C.prev=A | 28 | 未到達 |
| detach 3 | B.prev=None | 33 | 未到達 |
| detach 4 | B.next=None | 38 | 未到達 |
| adopt 1 | dst.child=B | 55 | 未到達 |
| adopt 2 | B.prev=B | 60 | 未到達 |

dst は `child=None` で initialize。三つの Copy link と payload を維持し、
第6 H、新 owner-bearing H field、Drop/GC、hidden grant は入力に加えない。
全 write は selected local-domain loan/ref projection の spelling を使う。
helper 内の loans は source 上で閉じてから full repack する。
**実際の sibling occurrence preservation、Change/Reset、B heap 生存、
domain conflict はこの source では未測定。**

### B ownership ledger: intended source flow, not a proved grant

`allocation_B/life_B/ptr_B` は3番目の Some の initialize 由来の変数。
TreeFour.middle → consuming `detach_middle(donor)` → complete destructure →
`LiveRoot{p:pb,a:ab,d:db}` → DetachResult.detached → consuming
`attach_whole(receiver,detached,ptr_B)` → TreeTwo.child が intended flow。
donor TreeThree は src/A/C。caller は A/C/src を先に finish し、最後に
**返された同じ whole** `finish_two(adopted)` を渡す（269行）。意図した
release order は `[2,4,1,3,5]`。ptr_B は Copy link 用引数であり A/D を作らない。
この current-carrier/非Copy identity/primitive release は **未検査**。
型名・関数名、上記変数対応だけで owner 権限を認定していない。

## 20 source observations / fixed controls

`P281_SOURCE_OBSERVATIONS.json` は20 actual CLI 入力、SHA、診断、2回一致、
public boundary status/span を保存。runner が全入力を再生成する。

| 分類 | 件数 | 意味 |
|---|---:|---|
| parser unsupported | 14 | 完全入力 + 10 attacks + 改名 + minimal Four/Three |
| semantic/profile unsupported | 3 | two declarations、mixed nested result、独立 edit helper |
| genuine semantic reject | 2 | **先行 P278 対照だけ**。Allocation_C / Domain_C terminal |
| semantic ACCEPT / backend unsupported | 1 | **先行 P278 対照だけ**。exact returned whole after A/C/src |

完全入力への10 attacks: genuine original Allocation_C swap、Domain_C swap、
duplicate B owner constituent、donor 再使用、active D_B loan over repack、
partial TreeFour、partial TreeThree、B omitted、normal refusal owner loss、
Copy B link without B responsibility。全て同じ TreeFour parser blocker。
これらを semantic 安全拒否の10件と数えていない。stale sibling/payload
occurrence、stale B、H-loan/refund の full composition は未到達であり、
後続 oracle を証明していない。helper callback/new generic effect は追加していない。

独立 min Four/Three も第3 nominal field で parser unsupported。3/4 arity
だけを増やして終わらないことも、**別の admitted-shape probe** で測定した:

1. 二つの2-member owner declarations は parse OK、registration
   `NL_CHECK_SEMANTIC_UNSUPPORTED`（2）/ `P276-NESTED-PROFILE`。
   現在は「one two-triad record in one unit」限定。mixed nested result の
   probe もこの registry fence で止まり、heterogeneous members は未検査。
2. 既存 LiveRoot/TreeTwo fixture に追加した独立
   `edit_packet(LiveRoot)->LiveRoot` は parse OK、body の local-domain loan
   で `ALLOCATED-DOMAIN-LOAN` / semantic UNSUPPORTED。良い actual caller を
   用意する前でも、whole-returning field-edit helper の conditional
   applicability を独立 body から推論する profile が未実装。

public observer は parser/register の readonly API のみ。seed/injected
R/O/A/D、Matched bit、private header は使わない。parser/profile/semantic
失敗の context snapshot は全て byte-equal に rollback。entry/backend を
呼ばない observer 自身の `owned_entry_artifact:false` と、実 CLI が
backend に到達したかを混同しない。

`P281_FIXED_CONTROLS.json`: fixed main と fixed #280 の各20入力を各2回、
合計40対照行。実 binary SHA と checkout SHA を記録。
main は完全入力の LiveRoot 第1 ptr field（9:22–9:25）で unsupported、
#280 は上記 TreeFour 第3 field で unsupported。新 branch は core 無変更で
#280 と同じ診断。固定 branches/main には変更を加えない。

先行 P278 positive の public owned validator 再測定: exact current result
input124/donor70/formal83、member R/A/D `[5,3]`、元 root `[57,31]`、incarnation
`[60,34]`、active loans0、live A/D各2、A/C/src already ended3、actual primitive
release `[2,4,1,3,5]`。CLI exit4 `V1-BACKEND-UNSUPPORTED`。間違った元 C
Allocation/Domain は `P193-CALL-BACKING` / `P193-CALL-DOMAIN` semantic
error（registration status1）、actual `finish_two(pair)` span、artifactなし。
これは有効な **predecessor terminal regression evidence** であって、
今回の detach/attach を省略した成功結果にはしない。

## Smallest blockers / bounded stop

FIRST `SOURCE-SURFACE-GAP`: parser `avs_struct` は nested nominal member を
index0/1だけ受理し、two-member declaration を要求する。必要最小対象は
complete acyclic 3/4-member owner AST と、複数 owner nominal/mixed
DetachResult の登録・structural nonCopy/full decompose/repack。
現profileの制限であり、general owner を禁止する新 core law ではない。

その後の独立 `SOURCE/CALL-PROFILE` gate は、元 p/A/D を仮定せず
whole-returning edit body の domain/ref requirements、Change/Reset と
nonCopy outputs を actual caller へ推論・伝播する adapter。現 pure whole
call validator は constructor/identifier/return の有限 body のみ。
closure replay は nested REPLACE を拒否し、captured Change は一 fragment
6まで。将来の修正は caller 初期6 + detach4 + attach2 を各 actual body /
immutable world に帰属させて replay しなければならない。単純な capacity
増加や固定 graph assertion、good caller の formal fact 化では足りない。
これら後続箇所は code inspection の risk であり、この完全 source の
実測 first error は parser である。

0..5 allocation-failure worlds の cleanup は source に保持し、既存 P278
validator/tests は維持。ただし新 full composition の0..5/refusal worlds
は未検査。normal refusal は refund または exact cleanup が必要という
条件を保留し、bool/None の隠れた責任廃棄を認める実装には進まない。

## Reproduction / validation

three predecessor flags は全て default OFF のまま。新 core flag はない。
new readonly evidence executable と1 source-gate integration test を
TRANSITIVE opt-in にだけ追加した。CMake test inventories は
default249 / original250 / nested254 / transitive259（従来258 +1）。
元の22/38/36 source contrasts、poison/OOM、canonical native PRE-detach
baseline tests を削除・緩和していない。new path native は未実装・未実行。
GCC Debug/Release、Clang、ASan/LSan、UBSan の4-phase regression と
exact-head CI の最終実測は Issue #281 の唯一の Track P 報告を参照。

```sh
cmake -S . -B build -DNEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON \
  -DNEWLANG_EXPERIMENTAL_NESTED_CALLER=ON \
  -DNEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL=ON
cmake --build build --parallel 2
ctest --test-dir build -R five_root_source_gate --output-on-failure
```

No canonical/Draft/既存 PR/native emitter/merge/他 Track 起動。
Issue は independent Coordination 用に OPEN のまま、実験報告後 STOP。
full original five-H cJSON native、accepted-rich refinement、H1、B1–B8/human
cost は未完。Draft17.31 MERGE HOLD、H1 PASS/FAIL UNDECIDED。

`P CJSON-B-STATE-1 FIVE-ROOT DETACH/ADOPT SOURCE: HOLD — PRECISE SOURCE/CALL/FIELD PROFILE INTERFACE UNPROVED`
