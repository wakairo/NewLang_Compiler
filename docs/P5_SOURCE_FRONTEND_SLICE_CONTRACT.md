# P5 — Source / Frontend Integration Slice Contract

Track: P

## Scope / authority

base main: `8fc45a0c86f68c5a8dcb2238e260e61e2cef2640`。
`docs/reference/CURRENT_SPEC.md`が指すDraft 17.9をcanonicalとする。
production policyは`NewLang_Project_Development_Process.md`、C / testing / review guidelines。
Backend Contract v0.4とmerged P/F evidenceはsource spellingを上書きしない。
Issue #23のDraft 17.9 resume authorizationに従う。

実装対象はsourceから既存transactional checkerまでの閉じたslice。
§27.1/27.1a、§19.1、§16.1/16.2をsource authorityとして使用し、
§12.1/§14.2のexclusive reborrow、P3/P4/R1のtyped/raw transitionsを再実装しない。

## Phase A / selected source profile

| Audit item | Selected / deferred |
|---|---|
| single binding | `let name = expr`。block itemには`;`が必要。 |
| multi-result receiving | `let (a,b,...) = expr`。2以上、distinct/fresh、exact arity、atomic receiving。 |
| aggregate | registered fixed-shape nominal typeの`Type { field: expr, ... }`、`let Type { field, ... } = expr`。全field exactly once、trailing field comma可。 |
| sum / variant | exact surfaceはProvisional。未対応。 |
| match | semantic rulesは保持するがexact source formは未対応。 |
| lexical block | `{ item; ... tail_expr }`。newlineはwhitespace、tailなしはunit。 |
| callable / declaration | registered named-callのみ。fn/callback/type declaration、loan body parsingは未対応。 |

Phase Aで報告したA/B/CはDraft 17.9のこのprofileで解消した。
[Issue audit](https://github.com/wakairo/NewLang_Compiler/issues/23#issuecomment-5992998268)に
`Track: P`として実装前に記録した。sum/matchのProvisional surfaceは採用しない。

`nl_parser_parse_source_fragment`がP5 entry。
既存P2のtype/expression/binding/loan fragment entriesは元のsubsetを維持し、
旧multi-result rejectionを恒久的なlanguage rejectionへ変更しない。
P5 entryもstandaloneのbinding/expressionとEOFを受け付ける。
standalone bindingの末尾`;`は省略可、expressionの`;`はdiscard statementを表す。
一般source file grammarは導入しない。

## Module boundaries / ownership

```text
NLSource -> P1 lexer -> P2/P5 syntax -> semantic candidate -> owned checked fragment
```

- source/parser/treeのownership contractはP1/P2を継続。
  Syntax listsはtree-owned nodes間のsource-order link。source spanだけを保持し、semantic authorityを持たない。
  node destructionはiterative、parserはone lookahead、depth/node予算を維持する。
- `NLSemanticContext`はtype/field names、binding registry、packages、places/scopes/raw ledgerを所有。
  aggregate registrationは借用host stringsをcopyし、成功時だけtype IDを返す。
- `nl_semantic_check_source_fragment`はtreeをsynchronous borrowし、既存candidate clone/commitを使用。
  成功時だけcontextをcommitし、caller-owned checked artifactを返す。
  semantic/unsupported/precision/OOM/resource failureではcontextとowner slotを保持する。
- checked nodesはsyntax pointerを保持しない。type/symbol/value identitiesとsource spansを保持し、
  tree destruction後もartifactを利用できる。source text accessにはsource lifetimeが必要。
  artifact destructionはsemantic discardではない。
- context IDsはappend-only。block終了後のlocal binding IDsもhistorical inspection用に保持するが、
  name lookupから除外する。成功したbinding/place/valueへの参照をreallocのアドレスに依存させない。

## Receiving / blocks

RHSを一回評価し、result countとreceiver countを検査してから全bindingをcandidateへ移す。
produced packageのreceivingを`NL_VALUE_RECEIVED`として記録し、ordinary Copy-useとは区別する。
`take`のT/slotはchecked resultsの別entryであり、tuple package/typeは生成しない。
receiver名はRHS内ではscopeに入らない。単一bindingへ複数結果を受け取る場合もarity error。
重複、arity failure、後続item failure、OOMはいずれもpublic contextへ部分commitしない。

blockはlexical binding floorを保存して内部scopeを導入する。
同scopeのduplicateを拒否し、outer shadowingはinner-first lookupで扱う。
inner終了時にはouter nameを再び参照できる。
semicolon statementは全visible resultがDiscardableの場合だけdiscardする。
normal exitにAvailableなnon-Discardable localがあれば拒否する。
Discardable localのordinary scope endはroot/current factを終了させるが、
explicit Storage/slotやnon-Discardable authorityの暗黙cleanupは生成しない。
non-Copy tailはpackageを外へtransferし、Copy tailはfresh packageを返す。

## Registered aggregate / identities

`nl_semantic_register_aggregate`は固定されたfield名/type identityを登録するhost API。
fieldごとのsemantic typeからCopy/Discardableを導出する。
P5ではflat dependency-free nominal/scalar fieldsに限定する。

constructionはnamed fieldsを全て検査し、initializerをsource orderで評価する。
field indexはdeclaration order、checked field nodesの順序はsource order。
complete aggregate packageだけを成立させ、member packageは`NL_CARRIER_AGGREGATE`と
`aggregate_owner: NLValueId`で所有者を保持する。
このcarrier identityはPlaceId、incarnation、current-value fact、BackingRegionではない。

non-Copy whole-value transferはaggregateとそのmember packageを保持してcarrierを移す。
Copy aggregateのvalue-useでは全memberにfresh package IDを生成する。
whole-value destructuringはcomplete aggregateを終了し、全field packageを一括してfresh bindingへ移す。
source aggregateのpartial-move stateは作らない。
Copy destructuringやtemporary discardが元のmemberを終了させないことを直接検証する。

field/subobject ptr/ref、physical layout、aggregate typed-root install/change、nested aggregate、
occurrence-dependent payload semanticsはこのsliceに含めない。
それらを求めるAPIはsupported flat representationの事実を超えて成功させない。
aggregateのopaque seed/rootやopaque registered-call resultのmintもunsupported。
aggregate declaration、partial construction、nested/rest/renaming pattern、sum occurrence carrierは導入しない。

## Reused semantic transitions / failure / diagnostics

Copy/consume、parameter compatibility、exclusive child reborrowとcall-return終了、
initialize/take/destroy、stale incarnation、BackingRegion access、raw ledgerはP3/P4/R1の経路を使用する。
P5はsource receivingとblock sequencingを接続し、新しいlifetime/raw authorityを追加しない。

syntax error / syntax unsupported、semantic error / semantic unsupported / analysis precision、
host OOM / resource limitを別status/categoryで返す。
first diagnosticはdeterministicなhalf-open byte span。field/receiver/callee rolesをchecked nodesに保持する。
hidden/unknown dependenciesは既存precision rejectionを維持し、dependency launderingを成功させない。

Linux linker fault injectionはtestだけに存在する。
registration/clone、parser tree/list、receiving/binding、aggregate Copy members、checked bufferの
malloc/realloc failuresを列挙し、全public views/identity counters/raw stateとoutput slotsの不変を比較する。

## Bounded implementation / non-goals

- type/field/value tablesは既存4096 entry budget、depthは128、aggregate fields/receiversは16。
  実装予算はNewLangのlanguage limitではなくhost resource statusで報告する。
- 現在のoperation結果は最大2責任。general multi-result user-function signatureは未対応。
- aggregate fieldsはflat nominal/byte/u8/usize/addr。nested/authority/ref/domain fieldsや
  hidden dependencies、nominal Copy/Discardable restriction declarationは将来のimplementation work。
- general expressions/operators/numerals、field access、sum/match、fn/callback declarations、
  source loan body、full-file frontend、MIR/LLVM/FFI、F1.5/relocation/M9/Red Teamはscope外。
- context cloningとbounded linear lookupを継続。incremental solver/allocator frameworkは導入しない。
- CLIのcompile pathは引き続きunsupported。P5のsource pathは上記module APIsとtestsで検証する。
