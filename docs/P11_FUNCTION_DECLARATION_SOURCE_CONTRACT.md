# P11 ordinary function declaration source contract

Track: P。対象は[Issue #72](https://github.com/wakairo/NewLang_Compiler/issues/72)。
正本は`CURRENT_SPEC.md` → Draft 17.13（§18.1 / §19.1 / §21.8 / §28.2）。
この文書はproduction coverage/ownership contractであり、normative Draftを変更しない。

## Source / syntax

```text
fn name([parameter_name : parameter_type, ...]) -> result_type { existing block }
```

`nl_parser_parse_function_unit`はone-or-more **fn-only** top-level declarationsをEOFまで読む。
parametersはordered、zero parameterは`()`, trailing commaなし。
colonの前後は通常token whitespace。arrowは隣接した`-`/`>`のみ。
result typeはexplicit、bodyは既存lexical block、post-body semicolonなし。
name直後のgeneric parameter listはunsupported。local/nested declarationもunsupported。
`fn`はcontextual wordのまま、既存`fn()` call spellingを予約しない。

syntax rootは`NL_SYNTAX_FUNCTION_UNIT`、childrenは`NL_SYNTAX_FUNCTION`と
ordered `NL_SYNTAX_PARAMETER`。type/bodyは既存syntax nodesを再利用。
node namesはsource spans、semantic IDsやowned stringsではない。
`nl_syntax_next_argument`でdeclarations/parametersをsource順にinspectionできる。
既存fragment entriesの範囲を拡張してsource declarationをblock itemにしない。

Type sourceは既存P2 subset（name / ptr / ref / exclusive ref）。
認識可能なtype syntaxでもP8のsignature precision fencesを満たさなければrejectする。
source nominal declaration、generic declaration、associated source registrationは追加しない。

## Whole-unit registration

`nl_semantic_register_function_unit(context, inputs, count, diagnostic)`は
function-unit tree arrayを**一つのsemantic unit**として処理する。

1. contextをclone。全source declarationsのname/span/input indexをprivate collectionへ収集。
2. spelling順にcollectionを並べ、全exact signaturesをexisting ordinary function registryへinstall。
3. 全owned `NLFunctionBody` plansをattach。
4. 全definitionsをP8/P9の同じcheckerで検査。
5. sum/raw invariantsを確認し、全体成功時だけcommit。

text order / source path / host input orderによってcallee discoveryを変えない。
sortはdeterministic internal enumerationのためで、nameをpersistent declaration identityにしない。
IDsは依然context-local registry identitiesである。
signature-onlyのcoarse effectsへsource bodyを置き換える経路はない。

same ordinary function nameのduplicateはsignatureに関係なくerror。
preestablished ordinary function / visible binding / host nominal declarationとのcollisionもreject。
core spellingsを新たなreserved-word tableへ追加しない。
parameter duplicateはerror、exact unitはP10 predicateでordinary function/parameter/local名としてreject。
field/variant labelsはこのruleの対象外。associated candidatesを生成しない。

## Shared body semantics / precision

P8/P9のfresh parameter namespace、Copy/non-Copy use、actual-place substitution、
source-order effects、same-place swap、tail/return transfer、exit compatibilityを再利用する。
acyclic body-to-body callは`run_body`をnestedに実行し、owned nested body evidenceを保存。
definition flagと累積traversal depthをnested checkerへ伝播する。
formal sitesをactual alias-disjointnessの証明に使わず、各actual callを再checkする。

active synchronous call chainの**resolved function ID**が再出現する場合:

- status: `NL_CHECK_ANALYSIS_PRECISION_LIMIT`
- category: `precision`
- code: `P11-RECURSIVE-ANALYSIS-PRECISION`

unknown calleeのsemantic errorとは区別する。language-level recursion banではない。
recursive SCC fixpoint / summaryは未実装。
P9 multiple-normal-arm join、richer ref/authority/aggregate signatures、ordinary-ref result等の
既存precision/semantic fencesを保持する。

resource budgetsはlanguage limitsではない:

- parser: depth 128 / nodes 4096 per source tree（既存）。
- unit: at most 128 declarations、parameters 128 per function。
- nested body walk: 累積semantic depth 128、body-call work 4096。
- semantic storage / checked node / per-artifact body budgetsは既存のまま。

## Ownership / failure

parser/treeはowned、treeはsourceをborrowする。sourceはtree inspection/registrationまでliveであること。
registrationはinput array/tree/sourceを呼出中だけborrowする。
source body spanのbytesをcopyし、existing source-fragment parserでowned lexical bodyへ再構築する。
parameter namesもcopy。registered plansは全input owner破棄後も有効。
host body registrationは従来どおりfull body sourceを保持する。

call artifactsのnested evidenceはimmutable plan/sourceをretainし、public contextをborrowする。
semantic ID inspectionにはcontext lifetimeが必要。destructionだけならcontext破棄後も安全。
unit registrationはdurable context stateを返し、formal/hypothetical checked IDsをpublicへpublishしない。

failureではowned parameter/name collection、candidate（partial signatures/plans）、temporary checked
artifactsをcleanup。semantic failure / OOM / resource limitでもpublic stateは不変。
parse failureはpartial treeをdestroyしてowner slotを変更しない。
invalid API argumentsはINTERNAL_ERROR、diagnostic output未変更。
successもdiagnostic output未変更。

`NLFunctionUnitDiagnostic`はinput index + `NLCheckDiagnostic`。
definition diagnosticのowned-body-relative spanをoriginal input body spanへmapする。
nested callee failureは既存P8 conventionでcaller call siteに報告する。
messages/codesはstatic borrowed strings。単一source rendererを選んだinputへ適用できる。

## Plumbing boundary / stop

複数physical sourcesはNLSource + fn-unit tree arrayで同じsemantic unitに入力できる。
pathはdiagnostic metadataだけ。file driver/import/modules/CLI compile pipelineは追加しない。
現在associated registration APIはない。source fnはordinary registryだけへlowerし、
qualified member syntaxをordinary function discoveryに流用しない。

PR exact head / CI evidenceはIssue #72 final `Track: P` reportとPR checksに記録する。
P11 READY FOR REVIEWで停止し、merge/Issue close/別track開始を行わない。
