# P12 structural ordinary-name reservation — production report

Track: P。[Issue #77](https://github.com/wakairo/NewLang_Compiler/issues/77)。
開始main: `e9a9522e0ddc72d09b8ebad9ca0e1d48f178ed79`。
最初にmainのdevelopment processを確認、CURRENT_SPEC → **Draft 17.14**。
M9.9 #75のfinal A' decision（initial B candidateはsuperseded）、§4.9 / §18.1 / §21.8 / §28.2を確認。
[実装前Phase A](https://github.com/wakairo/NewLang_Compiler/issues/77#issuecomment-6015187567)。
canonical Draft / Backend Contract / workflow / dependency pinsの変更なし。

## Shared classification / diagnostics

private `src/ordinary_name.h`はborrowed bytes+lengthを受けるpure、allocation-free classifier。
parser/semantic checker/binderが同じclassificationを使う。lexer token kinds / general keyword inventoryではない。

| exact spelling | class | structured diagnostic |
| --- | --- | --- |
| `unit` | `NL_NAME_CORE_UNIT` | existing `P10-RESERVED-NAME` / core singleton spelling |
| `fn`, `let`, `return`, `match` | `NL_NAME_RESERVED_STRUCTURAL` | `P12-RESERVED-STRUCTURAL-NAME` / ordinary lexical namespace reservation |
| others | `NL_NAME_ADMISSIBLE` | this ruleでrejectしない |

case-sensitive/exact length比較。prefix/suffixの巻き込みなし。unitをstructural setへ入れない。
diagnostic code/messageもshared static borrowed strings。structural messageはordinary lexical namespaceを明示し、
member labelがglobally illegalだとは述べない。
semantic/host bodyはsemantic-error status、early source headerはsyntax-error statusで同じstructural diagnosticを使う。
診断outputを持たないprogrammatic APIsは既存semantic-error statusを返す。

## Ingress / parser ordering

| ordinary introduction | admission / evidence |
| --- | --- |
| P11 source function name | header name read直後、bodyより前にstructuralをreject |
| P11 source parameter name | name read直後、type/bodyより前にstructuralをreject |
| P3 single / P5 single local | existing receiver/fresh-name preflight |
| P5 multi-result receiver | 全receiver preflight、sibling publicationなし |
| aggregate shorthand fresh local | field labelからordinary localへ変わるboundaryでreject |
| P6/P7 consuming/borrowed match payload | existing arm preflight |
| P9 function match payload | existing function-arm preflight |
| P3 loan-header as-plan name | existing header preflight、loan body semanticsは追加しない |
| host body function/parameter metadata | existing owned-plan construction前admission |
| signature-only ordinary function / low-level binder | P10 boolean predicateがshared classへdelegate |
| seed value/domain/slot/reference/scalar / bind-result | low-level binderへ到達、private candidateをrollback |
| host nominal/aggregate/sum declaration name | P11のordinary top-level collision modelと整合するdefensive guard |

host nominal **declaration名**と、そのtypeに属するfield/variant labelsは別のboundary。
generic string-copy、member fields/variants、qualified constructor qualifier/label parsingへguardを置かない。

元witness:

```text
fn match() -> unit { unit }
fn caller() -> unit { match() }
```

最初のdeclaration name `match` spanで`P12-RESERVED-STRUCTURAL-NAME`。
後続`match()`のbody parse failureまで進まず、function unit treeもpublishしない。
同じearly ruleで他3 function名 / 4 parameter名 / later declaration・parameterもreject。

P10 unitは従来どおりsemantic admissionでcore reason/code/messageを保持。
P11のfn distinguishing-shape probeを除き、source expression/block位置のfnはstructural扱い。
malformed fn/let/return/matchをordinary same-spelling callへfallbackしない。
P2専用fragmentのsyntax subsetやtype/member grammarをlexer-wideに変更しない。
ordinary free `fn()` callのP11 positive witnessはDraft 17.14に従いearly name rejectionへ更新した。

## Non-reservation / source regression matrix

| pressure | result |
| --- | --- |
| variants `match`, `return`, `fn`, `let`, `unit` | host registration / qualified `Code.label` / variant match armsすべてpass |
| fields `let`, `fn`, `return`, `match`, `unit` | registration / `Record{label:x}` construction pass |
| same field labelをshorthand receiverへ受ける | fresh ordinary nameとしてreject、他local未publish |
| `Fn`, `Let`, `Return`, `Match` | source local/function/parameter/actual call pass |
| `fn_`, `let_`, `return_`, `match_`, `fn2`, `match_value` | 同上 |
| `unit_`, `units`, `Unit` | 同上、unitとはclassが異なる |
| `ptr`, `ref`, `read`, `write`, `exclusive`, `using` | 同上、structural setへ自動追加なし |
| non-NUL borrowed `match_` view | length 5だけstructural、length 4/6はadmissible |
| fn declaration + let local + match + explicit return unit | source `run` body definition / all5 variant actual calls pass |
| unit type/value/argument/tail/member behavior | existing P10 tests保持 |

ordinary-name testsは4 structural spellingsすべてをsource/host/seed/binderへ入力する。
source parameter testはUnknown type/malformed match bodyよりheader name errorが先になることを確認。
error messageにunit reasonが混入しないことも検査。

## Transactionality / ownership / OOM

allocation-free classificationを既存clone/check/commitとpreflightへ接続する。
first diagnostic、borrowed source spans、owned tree/body/checked artifactのcleanup契約を保持。

- later reserved function/parameter: partial tree owner未publish、public registry不変。
- multi take receiver: sibling未publish、root live、ending Available。
- destructuring: sibling `ok`未publish、input responsibility/state不変。
- consuming/borrowed/function match: payload binding未publish、caller-visible state不変。
- source function-unitのlater body local failure: 全signatures/owned plansをrollback。
- prior store後のreserved local failure: caller writeもrollback。
- programmatic APIs: output sentinels / full public snapshots不変。
- malloc/realloc injection: early header parse、source single/multi/destructure/match、unit registration/owned body、seed/binderに隣接するallocationsをsweep。

P11 input-owner破棄後のdurable calls/nested evidence契約を変更しない。

## Validation / reproduction

existing82 + new4 = **86 CTests**。
new groups: `p12_ingress.unit`, `p12_header.unit`, `p12_member.unit`, `p12_failure.unit`。
P10 unit、P11 visibility/order-independent acyclic chains/recursive precision、P9 return、P6/P7 match/ref joins、
R4 argument compatibility、raw Storage/initialize/take/destroy、source/parser/ownership/OOM、LLVM/oracle/integrityを保持。

required local configurations:

| configuration | result |
| --- | --- |
| GCC 14.2 Debug | 86/86 |
| GCC 14.2 Release/NDEBUG | 86/86 |
| Clang 23.1.2 + clang-format-23 | 86/86 / format pass |
| ASan / leak detection | 86/86 |
| UBSan / halt_on_error | 86/86 |

frozen bootstrap verified: LLVM23.1.2、C17、CMake3.31.6、Python3.12.14。
exact final head / PR / PR-triggered five-job CI evidenceはIssue #77 final `Track: P` report / PR checksを正とする。
過去headのgreenをfinal handoffに使わない。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-p12 -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p12 --parallel 2
ctest --test-dir build-p12 --output-on-failure
ctest --test-dir build-p12 -R p12 --output-on-failure
bash scripts/check-format.sh
# READMEのRelease / clang-23 / sanitizer=address,undefinedも同じtests。
```

## Findings / limits / stop

canonical ambiguity / spec holeの新規発見なし。F2未trigger。
existing source/profile/analysis precisionとordinary vs associated candidate分離を保持。
今回はproduction correctionの証拠を用意しただけで、R6-01 targeted revalidationを実施していない。
lexer keyword redesign、full keyword inventory、escaped identifiers、generic declaration、M/F、modules、LLVM、
relocation、FFI、concurrency、別P milestoneは未実施。
open/unmerged PRとexact-head validation / Issue final reportを残し、**P12 READY FOR REVIEW**で停止する。
