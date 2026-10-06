# P14-pre — production prerequisite report

Track: P。対象は[Issue #95](https://github.com/wakairo/NewLang_Compiler/issues/95)。
base main `0cbc171c621334bf4ca7301ddfefb9293d7bb75a`、canonical Draft 17.17、F2 CLOSED。
branch `p14-pre-control-header`。
[実装前方針](https://github.com/wakairo/NewLang_Compiler/issues/95#issuecomment-6022522585)を記録した。
P14 #94のPhase A findingはcanonical gapではなくproduction representation prerequisiteとして扱う。
Git historyを変更履歴の正とする。

## Before / after

従来のtermination bool + one returned summaryに、bounded owned target-aware exit集合を追加した。
normal continuation / Return / Continue / Break / zero-normal-emptyを分け、finite alternativeとlexical sequenceを内部APIで合成できる。
source Return、IF、function MATCH、block、body boundaryとcall evidenceに接続した。
source loop syntaxやordinary-name policyは変更していない。
Hはroot/public-prefix origin、固定slot shape、Copy unknown abstraction、unchanged non-Copy identity、exact captured availabilityを持つ。
entry / 全supplied Continue successorのinclusion、nominal target compatibilityを検査する。
詳細・所有権・proof boundaryは[contract](P14_PRE_CONTROL_HEADER_CONTRACT.md)を参照。

## Positive controls（Issue Part F）

| # | 実際のproduction evidence |
| --- | --- |
| 1 | empty exit + no normal outcome、function boundaryでfake Returnなし |
| 2–3 | one/two Returnを保持、result type検査。actual source both-return IFも2件 |
| 4–5 | Return / Continue / Breakをkind+nominal targetで区別 |
| 6–7 | internal finite outcome joinでnested Return+Continue / Break+Continue、lexical propagation |
| 8 | 同targetへの2 Continueを保存して両successorを検査 |
| 9 | distinct live targetをaliasせず、wrong header targetをreject |
| 10 | header create時にentry inclusion、再検査PASS |
| 11 | Copy 3→7 / 9をstrict reject / unknown H accept |
| 12 | unchanged non-Copyのsame-origin一責任、duplicate slot reject |
| 13 | captured availability保持PASS、consume後mismatch reject |
| 14 | real source storeでouter current fact変更、entry-only H reject |
| 15 | larger wide Hがentryと両異なるCopy store successorを包含 |

## Negative controls（Issue Part G）

| # | Result |
| --- | --- |
| 1 | empty outcomeはReturn unit集合とcount/continuationが異なる。empty-left blockは後続Returnを追加しない |
| 2–3 | Continue/Breakのfunction target指定、Returnのloop target指定をreject。mixed集合はfunction boundaryを通らない |
| 4 | 1本目はstrict Hに入るが2本目は外れる集合をreject。one-edge representative実装なら失敗する試験 |
| 5 | outer Copy storeのchanged factにentry-only Hが非閉包 |
| 6 | captured non-Copy consumeによるavailability変更をsemantic reject / public state不変 |
| 7 | same-type fresh non-Copy packageをprecision reject |
| 8 | 2 forkが同じfresh affine ValueIdを作ってもshared originにならない |
| 9 | 2 forkの異なるstoreがsame numeric current fact/valueを作ってもstrict origin証拠にならない |
| 10–11 | hidden/Unknown dependency inputはprecision reject。外側に残ったblockerも無視しない |
| 12 | nominal targetのidentity/categoryを使い、同じsemantic result IDやsource spellingからtargetをaliasしない |

return-only regression、actual nested IF 3 Return、actual MATCHの全arm Returnを保存するsource integrationも追加した。
登録source/tree/parser破棄後のactual callでowned evidenceのdurabilityを検査する。
raw/P7/P9/P11/P13の既存95 CTestsは保持し、5 groupを追加して計**100 CTests**。

## Failure / resources

exit union、outcome composition、header clone/widen、exit growthの全malloc/realloc indexをfirst successまでfault injection。
failureでout=NULL/normal未publish、既存edge数とfull public snapshotが不変。
64 exit上限は次edge/unionをresource rejectし、dropしない。
source checker側の既存OOM testsも新Return record/unionのallocationを通る。
ASan/leakとUBSanで全suiteを検証する。

## Validation / handoff

既存checksum-locked bootstrapを再実行してPASS。LLVM23.1.2/C17、GCC14.2、Clang23.1.2、CMake3.31.6/Python3.12.14。
source parser、name policy、canonical/reference、toolchain lock、workflowは不変。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-p14-pre-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p14-pre-gcc --parallel 2
ctest --test-dir build-p14-pre-gcc --output-on-failure
```

GCC Releaseは別directory/Release、ClangはCC=clang-23、ASan/UBSanはそれぞれNEWLANG_SANITIZER=address/undefined。
formatはpinned `bash scripts/check-format.sh`。
local GCC Debug / GCC Release / Clang / ASan(leak detection) / UBSanは各100/100 PASS、pinned format PASS。
exact PR/head/run/job/CTest evidenceはIssue #95の最終Track: P reportとPR本文に記録する。
report自身へ自己参照head SHAを埋めてcommitを循環させない。

## Findings / remaining P14 boundary

COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY: 新規なし。
COMPILER-PRECISION: transformed/non-public affine origin、carried ref/hidden/Unknown dependency、rich occupancy/cyclic frame、iteration-local projectionはbounded substrateの外。
COMPILER-IMPLEMENTATION-LIMIT: 64 exits/16 slots、既存table/work/depth budgets。capをsoundnessの代わりに使わない。

supplied successor inclusionはsource body transferのsoundness / complete-edge supplyを単独では証明しない。
それらとsource syntax/receiving/target stack/break exitをP14へ残す。本taskでは#94を再開しない。
open/unmerged PRとopen IssueをCoordination reviewへ返し、**P14-PRE READY FOR REVIEW**で停止する。
