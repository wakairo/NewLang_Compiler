# North Star AVS — production implementation report

Track: P

## Task / authority

- [Issue #105](https://github.com/wakairo/NewLang_Compiler/issues/105): AVSのみ、OPENを維持。
- base main: `4428dbef0acf4c66d3fcdb013df8ce33d7eb2902`
- canonical: CURRENT_SPEC → Draft 17.17、変更なし。
- branch: `north-star-avs`
- semantic delta: **0**、field mutationはDeferred、`u8(-1)`もDeferred / unchanged。
- exact PR/head/CI runはIssue/PR final handbackへ記録する。

運用方針とIssue最新指示を先に確認し、既存aggregate registry / construction / whole
receiving / checked nodes / emitterをauditした。S1-S7は発火していない。

## Implemented data spine

actual sourceのone leading `struct NAME { LABEL:u8, LABEL:u8 [,] }`をparseし、single source
function unitのprivate registration transactionでnominal shapeを登録する。
一般aggregate declaration grammarではなく、Issueが認可したbounded D2 profileだけである。

既存§16.1/16.2 checkerが完全なnamed-field aggregate packageとwhole Copy destructuringを
検査する。checked nominal ID / field indices / scalar values / receiver symbolsから、
bounded C struct、immutable local、one-time whole RHS copy、u8 locals/useを生成する。
既存function signature能力やLLVM/backend frameworkを広げない。
C representationはsource layout/ABI保証ではない。

## Acceptance evidence

`avs_test`のpublic API検査（registration inputを破棄し、hidden seed/preludeなし）:

- E1: actual declarationからone new nominal Pair、field0=left/u8、field1=right/u8、
  Copy + Discardable、native layout unknown。
- E2: checked complete Pair、fieldsのresolved indexとchecked7,9。source順を逆にした場合も
  identityとvalueが一致し、source-order evidenceを保持。
- E3: same PairのCopy RHS、whole receiversのindex/symbol、u8 scalar7,9、Copy use。
- Copy whole destructuring後もoriginal aggregateはAvailable、そのmember packagesは保持。
  failed destructuringでpartial field bindingを公開しない。

`checked_c_avs_test.py`のactual-source integration:

- E4: synthetic nominal/field names、checked-value initializer、whole RHS temporary、receiver use。
  reordered fieldsとC keyword-shaped namesでもsource意味の再lookupなしでlowerする。
- E5: emitted CをC17 strict warningsでcompile/native execute。determinismとCopy reuseも確認。
- N1/N2/N3/N5: NewLang-side canonical field rejection、stdout空、C/native生成なし。
- accepted richer initializerのcallは明示的backend unsupported、stdout空。

initializerのleft-to-right checkingは、最初のfieldをmissing callee / range overflowに替えた
controlのdiagnostic priorityでも検査する。declaration profile外の入力はunsupportedとし、
新しいgeneral declaration semanticsを固定しない。

## Failure / integrity

malloc/realloc fault injectionでparser reuse、source registration、real body-sensitive
construction/destructuring callを検査する。OOM/semantic failureでpublic snapshot不変、
artifact未公開、retained field registry不変、cleanupと再試行成功を確認する。
source nominalもbody failure時にはprivate candidateと共にrollbackする。

V0/V1既存integrationとoracle adapter/smoke/integrityを保持する。新source profileは
leading `struct`とtwo-u8 constructionの組合せであり、historical smoke comparison inputとの
新overlapはない。frozen tests/programs 241件にleading `struct` / `u8(` inputは0件。
adapter/exclusion policyは変更しない。input-based / explicit / fail-closed。

## Validation / scope

固定rootless bootstrap、C17/CMake/CTest、GCC Debug / Release、Clang+format、ASan、UBSanを使用。
local full CTestは **152/152 PASS** × GCC Debug / GCC Release(NDEBUG) / Clang /
ASan / UBSanの全5構成。formatとchecksum bootstrapもPASS。
local baselineはDebian 13 x86_64、GCC14.2.0、Clang/LLVM/format23.1.2、
CMake3.31.6、Python3.12.14。既存Ubuntu24.04 CI matrixとpinは変更しない。
current-head PR-triggered CI evidenceはIssue/PR handbackで報告する。
通常のfull CTestにAVS parser/evidence/rejection/ownership/failure + native integrationを追加する。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-avs-gcc -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=none
cmake --build build-avs-gcc --parallel 2
ctest --test-dir build-avs-gcc --output-on-failure
bash scripts/check-format.sh
```

GCC Releaseは`-DCMAKE_BUILD_TYPE=Release`。Clangは`CC=clang-23`。
ASan/UBSanは別buildで`-DNEWLANG_SANITIZER=address` / `undefined`。

implementation limitsはone leading declaration / one input / two u8 fields、one nominal C
representation、effect-free scalar initializer、Copy local RHSだけ。language-invalidとは区別する。
新しいcanonical findingなし。一般declaration validity/order、recursive/nested/generic aggregate、
field projection/mutation、ptr/ref、allocation/lifetime、Node、cJSON、LLVM、F3.1、次sliceを開始しない。

open/unmerged PRで **AVS VALUE-LEVEL AGGREGATE SPINE READY FOR REVIEW** を記録して停止する。
