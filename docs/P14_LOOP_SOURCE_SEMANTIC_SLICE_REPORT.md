# P14 — loop production slice report

Track: P

Base main: `e8fd9fbc3e8dd7bc27e328e3d5529c0c9ec9e262`。
Canonical: `CURRENT_SPEC.md` → Draft 17.17（変更なし）。
FormalProof authority: `e591e53f295d4ab76d00d3bc1a2cdc443e06b7b4`、F2 CLOSED。
P14-pre #95 / PR #96 は merged prerequisite。P14 のみ、Issue #94 に対する review handoff。

## Delta audit / contract

[Track: P delta audit](https://github.com/wakairo/NewLang_Compiler/issues/94#issuecomment-6024808301)
は merged precondition と current parser/finite joins の接続を確認した。
full Phase A のやり直しは不要との Coordination 指示に従う。
parser / syntax、nearest target、projection、all-edge transfer、finite result join の契約は
[P14 contract](P14_LOOP_SOURCE_SEMANTIC_SLICE_CONTRACT.md) に集約する。
F2 の Entry inclusion / 全 Continue closure / exact layer と abstract layer の分離へ対応する
production evidence であり、新しい formal theorem / language decision ではない。

body input は unknown flat Copy slots + exact outer memory + unchanged affine origin。
全 reachable Continue を検査して exact memory invariant を証明する。
first-entry shortcut / single-successor selection / affine type-only identity / dependency erase
を用いない。finite rich Break/ref joins と cyclic precision fence を分離する。

## W1–W40

`tests/unit/loop_test.c` の `p14_wN.unit` は workload ごとに独立した CTest。
既存100 CTestsを残し、40追加、合計140。P12中央 ingress/member/failure test も9語へ拡張。

| Workload | production evidence |
|---|---|
| W1 | zero-parameter Continue、zero normal/result、owned body |
| W2–W4 | parentheses / shorthand / trailing-comma parse rejection |
| W5–W6 | duplicate / structural / unit parameter diagnostic、rollback |
| W7 | shadowed parameter と後続 initializer が outer `x` を参照、fresh name不可 |
| W8 | earlier owner consume が later initializerへ反映、failure atomic |
| W9 | 両 IF arm の zero-argument Continue が別々に保持される |
| W10–W13 | Continue arity/type と outside-target Continue/Break rejection |
| W14–W17 | missing semicolon / bare Break rejection、explicit unit Break |
| W18 | direct / one-normal IF body fall-through rejection |
| W19 | changing Copy carry、known entry を public unknown result として扱う |
| W20 | unchanged affine Continue、same-origin normal affine Break receiving |
| W21 | transformed affine carry を precision reject |
| W22 | captured outer non-Copy availability mismatch |
| W23 | carried-ref precision fence、ended local-derived capability projection rejection |
| W24–W25 | Copy Break、inconsistent Break static type rejection |
| W26 | complete two-origin ref result alternatives / scopes |
| W27 | distinct private affine Break results を precision reject |
| W28 | unreachable useを実行せず zero normal、non-Copy-result function divergenceにfake resultなし、divergence + real Copy/unit Return join |
| W29 | Return/Continue/Break mix、Return-only function ownership消費、terminating initializer |
| W30 | nested block / IF が nearest targetを継承 |
| W31 | all MATCH arms + sole-normal continuation の両 Continue を保持 |
| W32 | finite inner Break shadows/restores target、inner cyclic precision fence |
| W33 | ordinary callee fresh control context、independent inner Break、callerへContinue戻る |
| W34 | post-fork coincident private affine IDs の backedge rejection |
| W35 | different Copy successors全保持、known boolでも unsafe/blocked alternativeを落とさない |
| W36 | outer Copy `store` / fresh current factを cyclic precision reject |
| W37 | malformed structural sourceにordinary fallbackなし |
| W38–W39 | field labels保持、near spellings admitted（variant/memberは拡張P12 controlsも保持） |
| W40 | exhaustive allocation failure（parser、H、clone、exit、ref alternatives、Return、commit result）と16-slot resource fence |

W23のlocal-capability controlは、既存P3 programmatic ref APIで作ったlocal-derived capabilityを
projectionへ渡す destructive test。loan/callable sourceを追加していない。
W33のloan/callableはheader-only / absent source boundaryであり、新しいbody grammarはない。

## Validation / handoff

Local: Linux x86_64 Debian 13、GCC 14.2.0、Clang/LLVM/format 23.1.2、C17。
locked bootstrap / artifact identity と LLVM/oracle smoke は既存のまま。

Reproduce（各shellで `. .deps/activate.sh`）:

```sh
python3 scripts/bootstrap.py
cmake -S . -B build-p14-gcc -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=none
cmake --build build-p14-gcc --parallel 2
ctest --test-dir build-p14-gcc --output-on-failure
bash scripts/check-format.sh
```

GCC Releaseは `-DCMAKE_BUILD_TYPE=Release`。Clangは `-DCMAKE_C_COMPILER=clang-23`。
ASan / UBSanはClangで `-DNEWLANG_SANITIZER=address` / `undefined`、別build directory。
Local GCC Debug / GCC Release / Clang / ASan / UBSan は各140/140 PASS、format PASS、
locked bootstrap / artifact integrity / LLVM C API / oracle smoke PASS。
CIは既存Ubuntu24.04の5 jobs（GCC、GCC Release/NDEBUG、Clang+format、ASan、UBSan）。
exact head / PR-triggered CI URLと結果は、immutable commitを対象としたIssue #94 / PR reportで記録する。
この文書自身のcommit SHAを本文へ自己参照で埋め込まない。

## Findings / limits / stop

canonical spec hole / ambiguity: 発見なし、Draft変更なし。
COMPILER-PRECISION: cyclic rich memory / ref / transformed identity / nested recurrence、
private affine finite result、richer Return/frame correlationは契約に明記したbounded拒否。
COMPILER-IMPLEMENTATION-LIMIT: 16 slots / 64 alternatives / 既存traversal budgets。
resource / OOMによりpartial publicationやunsound successを起こさない。

P14 READY FOR REVIEW のhandoffで止まる。PR / Issueはopen、merge / closeはCoordinationのreview後。
新しいM/F/R、LLVM、relocation、FFI、modules、concurrency、次Pは開始しない。
