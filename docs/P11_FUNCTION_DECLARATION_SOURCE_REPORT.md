# P11 ordinary function declaration source — production report

Track: P。[Issue #72](https://github.com/wakairo/NewLang_Compiler/issues/72)。
開始main: `d2e63c4f7122a2e6caaa910c34269cce60ce3745`。
運用方針を先に確認、CURRENT_SPEC → **Draft 17.13**。
[実装前Phase A](https://github.com/wakairo/NewLang_Compiler/issues/72#issuecomment-6013512340)を記録した。
[実装contract](P11_FUNCTION_DECLARATION_SOURCE_CONTRACT.md)にAPI/ownership/precisionを記載。
canonical Draft / Backend Contract / toolchain pins / workflow変更なし。

## Implemented slice

fn-only top-level parser + immutable function/ordered-parameter syntax。
既存type/block parserを使用し、arrow adjacency / explicit result / comma / body形を検査。
lexical function/parameter namesはP10 admissibilityを共有。
whole-unit signature collection + 全durable body attachment後のdefinition checkingでorder-independent visibilityを実現。
source declarationsは既存NLFunctionEntry / NLFunctionBodyとP8/P9 checkerへlowerする。
acyclic body chainsはactual factsで実行し、nested checked evidenceをretain。
再帰はresolved identityのactive-chain検出後にprecision-rejectする。

## Break-test evidence

新規7 groups: `p11_grammar`, `p11_names`, `p11_visibility`, `p11_precision`,
`p11_behavior`, `p11_failure`, `p11_limits`（CTest `.unit` suffix）。
全体は既存75 + new7 = **82 CTests**。

| requirement / pressure | production evidence |
| --- | --- |
| W1 zero-param `ping -> unit {return unit;}` | register / actual call成功、builtin singleton保持 |
| W2 concrete parameter / tail、multiple parameters | exact named signature、Copy/non-Copy paths成功 |
| trailing/missing comma、missing/invalid colon、missing result、omitted/non-adjacent arrow、post-body semicolon | structured parser rejection、tree未publish、parser reusable |
| local/nested fn、generic-looking fn、unsupported generic type source | structured unsupported、fragment subset維持 |
| unit function/parameter/local name、duplicate params、duplicate same/different signatures | P10/common name / P11 duplicate diagnostics、state rollback |
| preestablished ordinary function/binding/nominal collision | reject、signature差やfile boundaryでescapeしない |
| ordinary vs member candidate | ordinary checked registered-call identity、`CopyT.valid()`はordinary functionへresolveしない、`Labels.valid` variantは同名でもpass |
| forward two/three body chain / reversed text / reversed split-source inputs | 全4 presentationsでpass、non-Copy package identity・consume・nested evidence保持、same registry enumeration |
| Copy result / early nested return / unit result | Copy元Available、non-Copy exactly transferred、既存P9 flow保持 |
| mutation across source body chain / actual alias | workerへのswapでcaller current values交換、same actual placeはno-op、replace result identity保持 |
| nested function match / early failure path | guarded definition checking flagを伝播、actual Fail/Good双方pass |
| ordinary-ref result/nonescape / wrong result / hidden dependencies | signature/body/result fencesでreject、既存P8/P9 exit compatibility tests保持 |
| self/mutual recursion、mutual declaration reverse order | resolved identityに対するprecision diagnostic、unknown calleeと別status/code |
| invalid later body / duplicate signature / parse failure after valid declaration | unit全体失敗、partial registry/treeなし |
| OOM | parser、collection、signature installation、body attachment/validation、nested actual callをmalloc/realloc sweep |
| diagnostic source mapping | split inputsのinvalid returnはinput index 1 / original source return spanにlocation保持 |
| resource limits | declaration count、累積nested depth、exponential acyclic expansion workをbounded reject、public state不変 |
| durable ownership | source/tree owners破棄後もcall可、context破棄後もretained nested source inspection/destruction可 |

P8のobsolete blanket acyclic body-call rejectionはacceptanceへ更新し、actual invocationも検証。
self recursionはold unsupportedからresolved-ID precision diagnosticへ更新。
それ以外のP8/P9/P10/R、raw Storage/take/destroy/initialize、sum/match/ref joins、
parser/source/OOM、LLVM/oracle smoke / artifact integrityは保持する。

## Validation

ローカルの全required configurationsは成功:

| configuration | result |
| --- | --- |
| GCC 14.2 Debug | 82/82 |
| GCC 14.2 Release/NDEBUG | 82/82 |
| Clang 23.1.2 Debug + clang-format-23 | 82/82 / format pass |
| Clang ASan + leak detection | 82/82 |
| Clang UBSan (halt_on_error) | 82/82 |

checksum-locked bootstrap verified（LLVM 23.1.2、C17、CMake 3.31.6、Python 3.12.14）。
既存PR-triggered Ubuntu 24.04 workflowはGCC13 Debug/Release、Clang23.1.2、ASan、UBSanを使用。
exact final head SHA / run URL / final five-job resultは
Issue #72 final report / open PR checksを正とする。過去headのgreenはhandoff evidenceにしない。
既存checksum-locked bootstrap / `.deps/activate.sh` / CMake / CTest手順を使用する。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-p11 -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p11 --parallel 2
ctest --test-dir build-p11 --output-on-failure
ctest --test-dir build-p11 -R p11 --output-on-failure
bash scripts/check-format.sh
# README記載のRelease / Clang / NEWLANG_SANITIZER=address,undefinedも同じtests。
```

## Findings / limits / disposition

canonical ambiguity / spec holeの新規発見なし。新しいownership/lifetime/dependency ruleは不要、F2未trigger。
recursive SCCはCOMPILER-PRECISIONとしてbounded reject。
source nominal declarations / richer type signatures / multiple-normal function-match joinsは既存未対応範囲。
source syntaxはhidden local-scope refのcaller carrier installをまだ表現できない。
そのfunction-exit incompatibility invariantは既存P8/P9のreal-API fixture testsでpressure-testを保持する。
source-declared functionをhidden-dependent public stateへcallしても既存P3 precision admissionを通過させない。

physical-file plumbingは複数NLSource/tree arrayまで実証。new file driver/modules/separate compilationなし。
associated registration subsystem/source syntaxは追加しない。
general top-level item grammar / generic declaration / Red Team / M/F / LLVM / relocation / FFI /
concurrency / next P milestoneも未実施。
open/unmerged PRとexact-head CI、Issue #72 `Track: P` reportを残して
**P11 READY FOR REVIEW**で停止する。
