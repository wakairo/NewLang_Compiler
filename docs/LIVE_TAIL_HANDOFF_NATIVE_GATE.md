# Draft17.27 cross-actor two-heap native handoff

Track: P。[Issue #197](https://github.com/wakairo/NewLang_Compiler/issues/197) の bounded native gate。
base main は `aed9e7b7d49de338c570c8c6e89197f2251db134`、CURRENT_SPEC は Draft17.27。
[PR #196 の独立 Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/196#issuecomment-6059493926)
および merged semantic artifact を再利用する。

Historical design audit: N/A — faithful implementation of Draft17.27 §18.1a;
no language rule changed。Process §4.2、Design Decision Procedure、Ledger DI-009/010/011
を確認した。採用済み source / authority / same-D transfer / explicit lifecycle を KEEP。
新しい source/API 選定はないため Ledger と canonical Draft は変更しない。
これは一般的な関数・allocator・owner/effect system の native support ではない。

## Actual source と lowering 境界

primary は既存 `tests/fixtures/live_tail_handoff.nl` をそのまま使用する。
二つの同時生存 heap H の head link を Some(real tail ptr) から None に戻し、
`receive_and_release_tail(ptr_t, allocation_t, life_t)` を呼び、receiver 内で explicit
EndRoot / erase_slot / finalize_domain / matching deallocate、その後 donor が head を解放する。

`src/checked_c_node.c` の新しい経路は `owner_call` の検査済み証拠だけを受け付ける。
source text / AST / 名前のspellingから意味を再構築しない。

- independent definition と current function applicability を照合し、exact H / ALL requirements /
  順序付き 4/5-step trace、entry/post proof、owned callee body の同一worldを確認。
- call-qualified entry snapshot の current O/incarnation/R/D、valid exact ptr provenance/access、
  live original Allocation/Domain、full range/size/alignment/ordinary access を確認。
- 元の tail の unique occupancy、available raw/duplicate owner がないこと、surviving package の
  dependency が Free であること、source loans の終了を確認。Unknown/may-set は使用しない。
- selected checked operands、donor binding、input ValueId、fresh parameter binding、元の value-owned
  R/D を照合。parameter の stack place は donor place / heap-root place と別物である。
- exact callee post-state の O ended / cleared occupancy / R ended / D ended / owners consumed を照合。

現経路は closed two-H world 内、一回の receiver call のみ。
transfer point ではすべての source loan が終了している bounded subset をlowerする。
それ以外は backend unsupportedであり、追加の language-invalid rule を設けない。

## Real callee と runtime carrier

source receiver は一つの別の `static void nl_owner_<checked function ID>(...)` 定義と
実際の direct C call になる。parameter は `const nl_node *`、private allocation handle、
private domain token の C value carrier。caller が left-to-right に argument carrier を取得し、
元の nonCopy binding carrier をゼロ化した後に元の A/D を渡す。
callee は fresh parameter object を持つが、元の heap pointer / malloc handle / D token を変えない。
heap root の placement は checked O/R から選び、parameter stack place から作らない。

callee C は owned checked body の既存 primitive emitter を使用する。
proof summary から cleanup code を生成せず、source-defined operations のみをemitする。
実際の `free` は Allocation operand の original handle を使う。
Copy ptr を deallocation owner として使わない。
receiver function に allocation はなく、donor の head carrier と region stage は変更しない。

C text は main と receiver の private buffer にstagingし、すべての検査と resource bound が
成功した後だけ publishする。失敗はoutput pointer/lengthを変更しない。
receiver function を main の前へ配置する bounded assembly であり、generic call ABI / MIR / optimizer はない。
representation は既存 Linux x86_64 private C17 target の H size24 / alignment8 と member type/offset
static asserts を継承する。NewLang C ABI/layout保証へ昇格しない。

## 独立したread-only native oracle

`handoff_checked_probe.c` はactual sourceをproduction checkerで検査し、original AST teardown後の
checked INIT / ROOT / FIELD / CALL証拠を取得する。observer expectationsはこの独立probeから渡す。
emitterのsource spellingや生成Cの定数から期待値を作らない。

`handoff_observer.h` は既存two-heap observerの独立したread-only拡張。
Hのbytesを書かず、authorityを作らず、link修復やfreeをしない。mutableなのは観測bookkeepingのみ。
既存 `two_heap_platform.c` のNULL-only shimは成功時にreal malloc/freeへ必ず委譲する。
observerはmallocで得た実アドレス、physical link None→Some(tail)→None、Option Copy、selected safe reloan、
checked field/scope/root identity、EndRoot→full raw→finalize→release→real freeを検査する。

transfer/receiver entryでは、元のtail ptr/handle/D、donor A/D carrierゼロ、fresh callee parameter address、
checked parameter symbolを確認する。free callbackはcallee entry/exitの間にtailを一度だけ解放し、
その後donorがheadを解放したことを区別する。EndRoot後はH bytesを読み直さない。
NDEBUGでもVERIFYは有効で、observerなしの同じ生成Cのplain nativeも実行する。

同じ生成Cを3つの実malloc結果で実行する:

| outcome | simultaneous roots | receiver calls | receiver real free | donor real free |
|---|---:|---:|---:|---:|
| first NULL | 0 | 0 | 0 | 0 |
| second NULL | headのみ | 0 | 0 | 1 |
| both success | distinct head/tail | 1 | original tail 1 | original head 1 |

both successではhead/tail addressが非zeroかつ異なり、callee前にunlinkしtailがlive、
callee return時tail release済み/head live、donor A/Dは再利用不能であることを確認する。
optional receiver readがある場合は既存4 root/scope観測にsame-D receiver readを1つ追加する。

## Falsification / rollback / regression

- native primary / renamed H・fields・parameter binders・receiver / optional readなしの3 source variants。
  それぞれ3 outcomesをobserverあり/なしでNDEBUG実行し、generated Cは2回生成して完全一致。
- 三同型mismatch、Consumed donor、stale ptr、scope conflict等の既存21 actual-source negativesは維持。
  rejectionはC公開前。既存8 semantic positivesはreal receiver Cの生成を要求する。
- 10 generated-C corruption controls: wrong callee tail ptr / wrong call ptr / forged Some / missing unlink /
  cross free / donor premature tail free / double free / early free / second-NULL head leak / omitted receiver free。
  random crashやsanitizer failureのみをPASSにせず、observerのSIGABRTを要求する。
- 32 checked certificate / operand / entry / post / scope / mismatched or cloned-world mutationsが
  unsupportedでoutput NULL・length sentinel不変。その後restorationで同じCをregenerate。
- 二つのstaged buffer mallocを個別fault injectionし、OOM時no output、clean retryで完全一致。
  既存registration/call/branch 1,801 malloc/realloc rollback controlsも維持。
- 既存two-heap native、one-heap / heap field / lexical / earlier executable testsを維持。
  oracle.adapter / oracle.smoke / artifacts.integrityは変更せずfail-closedのまま。

再検証:

```sh
. .deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
ctest --test-dir build -R 'live_tail_handoff|typed_owner' --output-on-failure
bash scripts/check-format.sh
```

local GCC Debug / GCC Release-NDEBUG / Clang / ASan / UBSanの全215 CTestsが成功。
current-head PR-triggered 5-job CIのSHA/run/resultはIssue/PR最終handoffを正とする。

## disposition

新しいsource/semantic gapや安全性矛盾は発見していない。
既存のfinite receiver trace / exact two-H / one call / private layout / source-loans-ended制約は
COMPILER-IMPLEMENTATION-LIMIT / COMPILER-PRECISIONの保守的backend fenceとして扱う。
一般allocator、third Node、traversal、recursive delete、FFI、LLVM、cJSON、後続Trackは未着手。

**DRAFT17.27 CROSS-ACTOR TWO-HEAP NATIVE HANDOFF READY FOR REVIEW**

candidate PRはOPEN / unmergedで独立Coordination reviewへ渡す。自己merge/自己closeしない。
