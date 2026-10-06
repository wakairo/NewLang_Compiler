# P8 registered function body / direct-call slice contract

## Authority / scope

Track: P。開始mainは `e8d9bf31353174062de3347e0b77c7e8e5b70851`。
先にmainの `NewLang_Project_Development_Process.md` を確認した。
`reference/CURRENT_SPEC.md` → **Draft 17.10** がcanonical。
Backend Contract v0.4、merged evidence、[Issue #47](https://github.com/wakairo/NewLang_Compiler/issues/47)
の順で参照する。Phase Aは
[Issue comment](https://github.com/wakairo/NewLang_Compiler/issues/47#issuecomment-6007426183)
に記録済み。reportや本契約はcanonical Draftを上書きしない。

対象は§18.1–18.8、§13.5a/c、§19.1、§27のbounded ordinary-function slice。
source `fn` declaration / explicit `return` / bare returnのsurfaceは追加しない。
P8はPR #48でreview・merge済みでCLOSEDである。後続trackは別のCoordination handoffを待つ。

## Host registration boundary

```c
NLCheckStatus nl_semantic_register_function_body(
    NLSemanticContext *context, const char *name,
    const NLFunctionParameter *parameters, size_t count,
    NLTypeId result, const NLSyntaxTree *body,
    NLCheckDiagnostic *diagnostic);
```

name、ordered named parameters、exact types、one result、既存source-fragment parserの
lexical BLOCKを渡す。non-generic、no overload。parameter名の重複、登録済みfunction名、
non-BLOCK bodyはreject。host APIであってNewLang declaration grammarではない。

入力source/tree/name arrayは呼出中だけborrowする。成功時はregistryがowned namesと
immutable body planを保持し、すべての登録入力を破棄できる。planはsource bytes/nameの
owned copyと、そのcopyに対応するowned syntax treeを持つ。独立したsource ownershipを
確立するため登録時に一度既存parserを使用する。call時のtextual expansion、source書換え、
macro substitutionはない。body planの実行はsemantic checkingでありruntime executorではない。

成功時はdiagnosticを変更しない。semantic/unsupported/precision/OOM失敗はcandidateを破棄し、
registryと全public semantic stateを維持する。first diagnosticは登録入力source上のspan。
invalid API argumentsはINTERNAL_ERROR、parameter budget超過はRESOURCE_LIMITを返し、
これらの早期API rejectionではdiagnosticを変更しない。

## Definition-time validation

function entryをtransaction candidateへ仮登録し、別のformal contextでbodyを検査する。
formal contextはtype/function registriesを保持するがcallerのbindings、values、places、
domains、scopes、occurrences、raw regionsを除く。caller名をimplicit captureできない。

parametersへfresh value/binding/placeを作る。ordinary ref/ptr parametersにはtarget typeの
representative siteを作り、refにはformal外側scopeを付ける。これらはtype/ownership検査の
ためのsiteであって、formal disjointnessの証明ではない。definition検査で作ったIDや
post-stateはpublic contextへcommitしない。各callでactual relationに対してbody全体を再検査する。

names、types、Copy/consume、statement Discardable、block sequencing、tail/no-tail結果、
parameter exit obligations、function-local dependency escapeをuncalled時点で検査する。
first actual callだけでdefinition validityを決めない。general branch/relative analysisを
要するconstructはP8 profileから明示的に除外する。

## Body-sensitive direct known call

1. existing checkerでactual argumentsをsource orderに評価し、ordinary Copy/consumeを行う。
   `T -> ref<T>` のimplicit borrowはしない。
2. candidate内にfresh callee parameter bindingsを作る。名前解決floorでcaller bindingsを隠す。
   callerとparameterの同名も安全。value package、ref alternatives、place/incarnation、scope、
   provenance、occurrence dependencyを保持する。ordinary write→read compatibilityはstatic
   parameter typeをreadへ変え、complete underlying factsを保持する。
3. immutable body syntax planをactual-context semantic checkerへ適用する。
   replace/store/swapはactual referentへ作用する。distinct/same-place relationをformalの個数から
   推定しない。existing primitiveのconflict/liveness/singular-ref checksを通す。
4. normal tail packageをcall resultへforwardする。non-Copy identityのvalue IDは同一であり、
   declared result typeからfresh packageをmintしない。Copyのvalue-useは既存Copy semantics。
5. no-tailはunit。exact declared result type/countを検査する。semicolon expressionはDiscardableが
   必須で、returnではない。parameter/body localsはexisting block exitでhide/endする。
6. function-exit invariantを検査後、call-local scopesを終了する。forwarded resultを終了しない。
   caller-visible post-stateをcandidateへ保持し、全fragment成功時に一括commitする。

signature-only fixturesは別経路として保持する。従来のcoarse effect/dependency flagsは
unknownを安全とみなさずrejectする。body-backed entryにはそのno-effect仮定を適用しない。

## Supported / limited matrix

| 領域 | P8対応 | 境界 / classification |
|---|---|---|
| signature | unit、flat nominal、byte/u8/usize/addr。parameterはordinary ref/ptr to flat typeも可、resultはptrも可 | aggregate/sum/core authority/exclusive ref/ordinary ref result: `P8-SIGNATURE-PRECISION` |
| body | exact existing BLOCK、single binding、semicolon statement、NAME、CALL、nested BLOCK | constructor/match/aggregate/multi-receiving等: `P8-BODY-PROFILE` / COMPILER-COMPLETENESS |
| body primitive | ptr_from_ref、replace、store、swap | 他primitive: `P8-BODY-OPERATION`。raw/typed lifetime API自体は既存P3–P7で維持 |
| legacy callee | signature-only no-effect/no-hidden-dependency fixtures | unknown flags/result authorityは既存diagnosticでreject |
| body callee | top-level direct known body-backed call | body→bodyはacyclicもdeferred。self recursion: `P8-RECURSION-UNSUPPORTED`、他: `P8-NESTED-BODY-CALL` |
| ref actual | complete may-setを保持。read-only supported bodyで全alternativesを検査 | joined write/ptr_from_ref等のsingular-operation fenceを維持。general memory phiはCOMPILER-PRECISION |
| alias | flat Tのdistinct/same-place swap、source-order replace/store | sum-root body signatureをreject。P6 conditional-occurrence swap fenceは広げない |
| exit | local place/scope dependencyのsurvival拒否、全ref alternativesの検査 | coarse hidden/unknown dependencyはCOMPILER-PRECISION。ref-field carrierはinternal fixtureのみ |
| explicit return | なし | source grammarを追加しない。full early-return CFG coverageを主張しない |

これらはcompiler coverage/precisionの境界であってlanguage illegalityの新規ruleではない。
parameter host budgetは128、単一checked fragmentのbody-call evidence budgetは64。
継承するsemantic/node resource budgetも維持する。whole-program solver/SCC/serialized summaryはない。

## Exit compatibility / W4

`nl_sem_function_exit` はscope/place prefixを境界として、ENDEDではないsurviving packageを
検査する。REFの全alternativesにknown scope/placeを要求し、local place、local scopeまたは
local ancestor scopeへの依存をrejectする。scopeが既にdeadでもsurviving dependencyを消した
ことにしない。unknown/coarse dependencyは保守的precision rejection。ptr tokenはref authority
ではないので、dangling token自体の存在をこのexit checkで禁止しない。

source profileはcallee-local loan creationやref-valued carrier install/restoreを表現できない。
W4では既存internal constructorsを使い、実際の新しいlocal scopeとcapability、caller-visible
ref carrierを構築する。同じproduction exit helperに対して、temporary local dependencyが
残る状態をrejectし、そのpackageをendし元のglobal dependencyへrestoreした状態をacceptする。
global+local joined refも全候補検査し、localscopeをendするだけではrejectが続く。
production `.c` includeやprivate fieldの直接書換えは行わない。このcontrolled fixtureを
full source loan/body integrationとして報告しない。

## Checked ownership / identity

registry cloneはimmutable planをretainし、names/type arraysは従来通りdeep-copyする。
context destroy/rollbackはplanをreleaseする。checked callはowned child body evidenceを持ち、
`nl_checked_call_body` でborrowできる。childはplan/sourceをretainし、parent destroyがreleaseする。
入力登録sourceの寿命に依存しない。plan refcountはper-objectで、mutable global stateはない。
同一contextは非reentrant、shared retained plansへのconcurrent mutationはサポートしない。

body evidenceのsemantic IDsはそのcall artifactと同じactual semantic contextを指す。
formal validation contextのIDはexportしない。既存match arm内のcall evidenceはarm-owned
hypothetical context内に留まり、public context IDとして使わない。public callのbody localsは
ended/hidden historical IDsとして残り、caller namespaceには現れない。
source textはowned childからアクセスできるが、semantic ID lookupはborrowed contextの寿命内のみ。
contextを先にdestroyしてもartifact自体のdestroyは安全。

## Transactions / diagnostics

registration、actual callを含むfragment checkingはclone/check/commit。
argument consume後、caller-visible tentative store後、body artifact attachment allocation時の
OOMもpublic stateを変更しない。RESOURCE_LIMITも同じ。失敗artifactを公開しない。

definition error spanは登録source。actual-dependent body failureはcaller call spanへmappingし、
body diagnostic code/categoryを保持する。既存first diagnostic rendererを使用する。
checkerは独自stderr出力をしない。testsでのみ失敗時情報を出力する。
