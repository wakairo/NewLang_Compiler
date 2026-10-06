# P13 — exact if source / semantic slice report

Track: P。Git historyを変更履歴の正とする。

## Authority / Phase A

- base main: `f33a0fc204349ea5b62f1bcade1d682411d654b8`
- canonical: `CURRENT_SPEC.md` → Draft 17.15。reference snapshotは変更していない。
- [Issue #84](https://github.com/wakairo/NewLang_Compiler/issues/84)
- branch: `p13-if-semantic-slice`
- [実装前Phase A audit](https://github.com/wakairo/NewLang_Compiler/issues/84#issuecomment-6016391310)

mainの運用方針、M9.11 #82 closure、canonical source/join sectionsとP6/P7/P9/P11/P12 evidenceを確認した。
canonical contradiction / ambiguityは見つからなかった。
精度不足をlanguage ruleへ変えず、bounded production incompletenessとして扱う。

## Implementation / ownership

core bool identity、Copy/Discardable allowlists、exact parser / dedicated syntax IF、
中央if/else ordinary-name reservation、owned checked IF/arm evidenceを追加した。
conditionはexact core boolの一normal resultを要求し、同じincoming stateから両armを検査する。
normal_arms=0/1/2を表し、P6 frame / Copy state widening / P7 ref alternative proof / P9 return boundaryを再利用する。
P11 definitions/actual callsに同じ有限checkingを適用する。

same-prefix flat non-Copy identityは保持し、distinct identityはstructured precision rejection。
private clone IDをpublicへcommitしない。
sole-normal continuationのcandidate再適用、nested evidenceのrecursive destruction、
function-level early-return result/effects joinと祖先prefix制限を追加した。
公開source grammar / body signature / matchの既存precision fencesを不必要に広げていない。
詳細は[P13 contract](P13_IF_SOURCE_SEMANTIC_SLICE_CONTRACT.md)。

## W1–W17 destructive matrix

| Issue witness | Production evidence / result |
| --- | --- |
| W1 simple value | source `choose(cond:bool,a:CopyT,b:CopyT)`登録・actual call PASS。両armからabstract Copy join |
| W2 one return | `early`登録/call、normal_arms=1。terminated result/effectsはlocal continuationへ混ぜずfunction出口で保持 |
| W3 both return | `both` / `none`登録/call。checked type=0 / normal result count=0 / terminates=true、normal unit/neverなし |
| W4 availability | one consume / one AvailableはP6-AVAILABILITY-JOIN。both-consume PASS。return後のfalse useを無視しsole normalでnon-Copyを再使用できる |
| W5 non-Copy identity | `owner`両arm / source `same` / `return_same` / `both_same`はexact ValueId保存。distinct normal identities・return-vs-tail・both-return別identityはP13-JOIN-PRECISION |
| W6 ref/dependency | 異なるpre-existing refsのplace/scopeを両alternative保持。nested joinのdedup、safe observe、第二alternativeだけのscope終了によるreject、joined ptr precisionを検査 |
| W7 ref + return | source functionのlocal ref IFとunit early return。normal結果に一concrete refだけが残り、terminated edgeからphantom alternativeなし。ordinary-ref function resultのP8 fence維持 |
| W8 match + return | IF内P9 match、match arm内IF、actual body calls PASS。既存P9 concrete-variant/normal-arm制限維持 |
| W9 nested if | depth≥2、return合成、owned nested artifacts。parser depth、arm evidence cap、片側return再適用のwork budget超過をresource/rollbackとして検査 |
| W10 name attacks | P12 ingress/header/member/failure suiteを6 structural namesへ拡張。function/parameter/local/multi/aggregate shorthand/payload/host APIでif/else reject。near spelling・loop/break/continue PASS |
| W11 no else | P13-IF-ELSE parse rejection、synthetic false unitなし |
| W12 direct else if | P13-IF-ELSE-BLOCK、else block tailのnested IFはPASS |
| W13 condition | canonical bool PASS、unit/Copy nominal/byte/u8/non-single result/non-normal result reject。no truthiness。condition pure fixtureがnon-Copy inputを一度consumeするwitness PASS |
| W14 parser boundary | aggregate construction入りcall condition / aggregate conditionを完全parseしてexact-type check。mandatory delimiters、malformed block/condition、trailing junk、non-tail semicolonを検査 |
| W15 labels | field construction、payloadless variants / match / source body labels if/else PASS。shorthand fresh ordinary bindingはreject |
| W16 transactionality | first arm write後のsecond arm semantic failure、result/state precision、resource failureでpublic snapshot不変。actual same-place aliasも保持 |
| W17 OOM | parse nodes、nested arm clone/evidence、ref-alternative value table/common join、source registration/retained plans、actual body callの全allocation indexをsweep。failureでNULL output / full snapshot不変 |

boolのtype lookup/core getter、host nominal collision、seed/root、aggregate/sum payload、
body bool result、abstract slot initialize→takeも直接検査した。physical layoutはunknownのまま。

caller-visible stateはboth-arm Copy storesをunknown current valueへ合流。
non-Copy divergent stores / early-return correlated non-Copy memoryはprecision reject。
mutating return armはlocal normal snapshotを汚染せず、function call exitでその可能性を消さずwidenする。
return側7 / normal側9のu8 storeは、fork後のnumeric ID一致を同一factとしない。
共有prefix guardを一時的に外したnegative controlではこの試験が失敗し、guard復元後はPASS。
source登録入力のsource/tree/parserを破棄した後のcallを全body workloadで検査する。
unused LinearTがreturn時に残ればscope obligation reject。implicit cleanup/restoreを追加していない。

## Validation / reproducibility

既存86 CTestsを保持し、`if_test`の9 groupsを追加して**95 CTests**。
P12の4 groupsはif/else namespace attackも追加した。
各source workloadは本番source→lexer→parser→semantic/checked APIを使用する。
CLI full-program compile、oracle differential coverage、runtime execution/LLVM loweringの実装は主張しない。
M7.5 oracle smokeとartifact integrity、既存全P/R4 regressionも同じCTestで維持する。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-p13-gcc -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=none
cmake --build build-p13-gcc --parallel 2
ctest --test-dir build-p13-gcc --output-on-failure
bash scripts/check-format.sh
```

GCC Releaseは別directory / `-DCMAKE_BUILD_TYPE=Release`。
Clangは`CC=clang-23`、ASan / UBSanはそれぞれ`address` / `undefined`。
CTestのASan leak detection、UBSan halt-on-error、NDEBUGでも有効なCHECKを使用する。
Linux x86_64 Debian 13 / C17 / GCC14.2 / Clang・LLVM・format23.1.2 / CMake3.31.6 / Python3.12.14。
bootstrapは既存TLS / signed metadata provenance / exact package / SHA-256 lockを再検証して成功。
lock/bootstrap/workflowは変更していない。

| Local configuration | Result |
| --- | --- |
| GCC Debug | 95/95 PASS |
| GCC Release/NDEBUG | 95/95 PASS |
| Clang Debug | 95/95 PASS |
| ASan/leak | 95/95 PASS |
| UBSan | 95/95 PASS |
| pinned format | PASS |

PR-triggered Ubuntu24.04のGCC13 Debug/Release、Clang23+format、ASan、UBSanは同じ95 testsを実行する。
exact PR/head/run/jobsのfinal evidenceはIssue #84のTrack: P handoffとPR本文へ記録する。
self-referential head SHAをreport commitへ埋め込まない。

## Findings / stop

- COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY: なし。canonical変更不要。
- COMPILER-PRECISION: distinct non-Copy conditional identity、richer aggregate/sum/authority result、
  非flat/hidden correlated memory、ref+mutation、joined write/ptr、既存P9/P11 fencesは明示reject。
  valid-but-unimplemented joinをsemantic language errorへ変えない。
- COMPILER-IMPLEMENTATION: function-exit joinのfork後numeric ID coincidenceを共有prefix proofで防止し、negative controlを追加。P13内で修正済み。
- COMPILER-IMPLEMENTATION-LIMIT: finite replay work / depth / table / evidenceはhost budget。
- DIAGNOSTIC-QUALITY: definitionはsource span、actual body diagnosticは既存call-site attributionを維持。

P13以外のP milestone、loop/break/continue、bool literals/operators/general grouping、general CFG/fixpoint、
F/R、LLVM/relocation/FFI/modules/concurrencyは追加・開始しない。
open/unmerged PR・open IssueをCoordination reviewへ渡し、**P13 READY FOR REVIEW**で停止する。
