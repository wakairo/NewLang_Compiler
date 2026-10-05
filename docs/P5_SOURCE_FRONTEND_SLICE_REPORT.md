# P5 — Source / Frontend Integration Slice Report

Track: P

## Authority / scope

base main: `8fc45a0c86f68c5a8dcb2238e260e61e2cef2640`。
canonical selectorは`CURRENT_SPEC.md`、targetはDraft 17.9。
Issue #23のresume authorizationとdevelopment processに従い、
[Phase A再audit](https://github.com/wakairo/NewLang_Compiler/issues/23#issuecomment-5992998268)を
実装前に記録した。旧source-surface blockers A/B/Cはselected aggregate profileでclosed。

実装契約は[P5 contract](P5_SOURCE_FRONTEND_SLICE_CONTRACT.md)。
sum/match exact syntaxはProvisionalのままdeferし、P5のみを実施した。
canonical Draft、Backend Contract、oracle archive、toolchain pinsは変更していない。

## Implemented path / result

source -> lexer -> owned syntax -> existing semantic candidate -> owned checked representation。
新しいsource/check entryでsingle/multi receiving、semicolon block、registered aggregate construction /
whole destructuringを扱う。P2の4 entryは旧受理範囲とunsupported分類を保持する。

- non-Copy use-after-consume、same-scope duplicateを拒否。Copy値はAvailableを維持する。
- multi-result RHSを一回評価し、exact arityで各責任をtransfer。
  takeのTとslotは別ValueId/別binding。tuple合成、non-Discardable結果のdrop、duplicateは不可能。
- block scope exitはnon-Discardable localを検査する。
  semicolonはdiscard permissionやraw authorityを増やさない。nested shadowing後にouter nameを復元する。
- source `take -> initialize -> take -> initialize`でsame rangeのincarnationを更新し、
  各call-local child scopeを終了させ、outer endingをAvailableのまま再使用する。
  stale ptrはfresh initializeで復活しない。
- write-only backingではsource takeを拒否し、source destroyを許可する。
  返されたslotのBackingRegion/rangeをP4 erase/deallocateへ接続し、ledgerを閉じる。
- aggregateのfield identityとsource orderを分離。
  whole move/destructureでmember ownershipを保存し、Copy時はfresh member packagesを生成する。
  non-Copy constituentのpartial move、途中binding、partially-live targetを公開しない。
- 後続error/arity error、registration/receiving/Copy member OOM、resource limitで
  public semantic views、counters、raw state、output slotsをrollbackする。
- source syntax/unsupported、semantic/unsupported/precision、host failureとspansを検証した。

これはfull compiler CLIやgeneral aggregate/root/occurrence frontendの完成を意味しない。
P5 contractにunsupported boundariesを明記し、足りないfactsをsuccessへ変換していない。

## Exact module / file inventory

| Files | Responsibility / ownership |
|---|---|
| `include/newlang/parser.h`, `src/parser.c` | separate P5 source entry。borrowed source / tree-owned intrusive lists、one lookahead、bounded traversal。 |
| `include/newlang/syntax.h` | block/item/receiver/aggregateのimmutable source views。既存`syntax.c`のiterative destructionを利用。 |
| `include/newlang/semantic.h`, `src/semantic.c`, `src/semantic_internal.h` | owned fixed field registry、scoped binding lookup、aggregate member carriers、transactional registration/clone、Copy package/cleanup。 |
| `src/semantic_check.c` | source receiving/block/aggregate checkingを既存P3/P4 checkingへ接続。candidateの全成功後だけcommit。 |
| `include/newlang/checked.h` | owned artifactのreceiver/item/field roles、distinct result IDs、receiving transfer marker。 |
| `tests/unit/source_slice_test.c` | parser/binding/receiving/aggregate/failure/diagnosticの6 direct test groups。 |
| `tests/integration/source_slice_frontend_test.c` | actual sourceからtyped/raw cycle、write-only regression、aggregate ownership cycle。 |
| `tests/support/semantic_check.h` | P5 API selectionと新public field/carrier factsを含むrollback比較。 |
| `CMakeLists.txt` | 上記7 CTestsを通常test entryへ追加。 |
| `.github/workflows/compiler-ci.yml` | 既存4構成にGCC Release/NDEBUGを追加。全jobで同じCTestを実行。 |
| `README.md`, P5 contract/report | current coverage、再現手順、authority、ownership、limits、validation。 |

新しいmutable global state、allocator DSL、LLVM stateやformal ghost representationは追加していない。
OOM injection globalsはtest binaryだけに存在する。

## Validation

local: Linux x86_64 / Debian 13、C17、GCC 14.2.0、Clang/LLVM/clang-format 23.1.2、
CMake 3.31.6、Python 3.12.14。rootless checksum-locked bootstrapを再実行してverified。

| Configuration | Local result |
|---|---|
| GCC Debug / strict warnings | 45/45 PASS |
| Clang Debug / strict warnings | 45/45 PASS |
| Clang ASan / leak detection | 45/45 PASS |
| Clang UBSan / halt-on-error | 45/45 PASS |
| GCC Release / NDEBUG | 45/45 PASS |
| pinned formatter / diff whitespace | PASS |

既存38 CTestsを全て保持。P5の7件はdirect module/API testsとsource->checked integration。
失敗testsは再実行してdeterministic diagnosticsとrollbackを検査する。
OOM testsは各allocation位置にfailを入れ、最初のfaultなし成功まで到達する。
limit testsは途中までCopy aggregateを評価した後のvalue budget failureもrollback検査する。
ASan/UBSanは新しいpathsを含む全C targetsへ適用する。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build-p5-gcc -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p5-gcc --parallel 2
ctest --test-dir build-p5-gcc --output-on-failure

# Separate directories; same build/test commands after configure:
cmake -S . -B build-p5-clang -DCMAKE_C_COMPILER=clang-23 -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build-p5-asan -DCMAKE_C_COMPILER=clang-23 -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=address
cmake -S . -B build-p5-ubsan -DCMAKE_C_COMPILER=clang-23 -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=undefined
cmake -S . -B build-p5-release -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release
bash scripts/check-format.sh
```

PR CIはfresh Ubuntu 24.04でGCC 13 / pinned Clang 23、ASan、UBSan、GCC Releaseを実行し、
Clang jobでformatを検査する。exact-head run URL/SHA/statusはPR本文とIssue #23の
`Track: P` review handoffに記録する。過去headのgreenをcurrent-head evidenceとして代用しない。
M7.5 oracle smoke / LLVM C API smoke / artifact integrityも全構成の通常CTestで維持した。
M7.5は新しい17.9 surface/aggregate意味論のnormative oracleではなく、同等性の主張はしない。
Lean buildや新しいFormalProof作業は行っていない。

## Findings / limits

- **COMPILER-IMPLEMENTATION-LIMIT**: flat registered aggregate fields最大16、nested/authority fields、
  physical aggregate field roots、field ref/ptr、aggregate typed-root transitions、opaque aggregate seeds/
  function resultsは未対応。source multi-result grammarは受理するが現在operation結果は最大2。
  一般sum/match/loan-body/function-declaration/full-file grammarは追加していない。
- **COMPILER-PRECISION**: hidden/unknown dependency summariesはP3の保守的rejectを維持。
  source block/aggregateをdependency launderingとして扱わない。
- **COMPILER-PERFORMANCE**: whole-context clone、fixed field arrays、bounded linear lookupを採用。
  完全frontendやincremental analysisの性能保証ではない。
- **COMPILER-DIAGNOSTIC**: selected malformed syntax、unsupported surface、field/arity/binding error、
  scope obligation、precision、OOM/resource statusを区別。一般error recoveryは未対応。

新しいblocking **COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY**は発見していない。
Draftを実装都合で変更せず、P4/R1/F1.4/Sync #22のsemantic closureをreopenしていない。

## Review gate

branch: `p5-source-frontend-slice`。dedicated PRをmainに対して作成し、
current-head CIを確認してIssue #23へP5 READY FOR REVIEWを記録する。
Git history / PR current-head checksをcommit/runの正とし、文書内に別のchange historyを持たない。
PRはmergeしない。F1.5/M9/LLVM/relocation/新規Red Teamへ進まない。
