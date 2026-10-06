# P7 — Ref-valued match-result join contract

Status: bounded production contract / review対象。Git historyを変更履歴の正とする。

## Authority / Phase A

current main `fb67bd68990b40093adb1d33cd7fbce2ee3321aa` の
[プロジェクト運用方針](NewLang_Project_Development_Process.md)と
[testing policy](NewLang_Compiler_Testing_Strategy.md)に従う。
`reference/CURRENT_SPEC.md` → Draft 17.10 §13.5a / §26.16 / §27.3 がnormative authority。
Backend Contractはlowering contract、M/P/F/R reportはevidence。
[Issue #41](https://github.com/wakairo/NewLang_Compiler/issues/41) の
[Phase A audit](https://github.com/wakairo/NewLang_Compiler/issues/41#issuecomment-6006550788)
を実装前に `Track: P` で記録した。新しいsemantics/surfaceは不要。

## ValuePackage representation / ownership

`NLSemanticValueView.reference_count == 0` は従来のconcrete `reference`。
nonzeroなら `references[0..reference_count)` がjoined refのmay-setであり、
`reference` はゼロのまま。singular consumerはこのfieldを読まずprecision rejectする。
各alternativeは place / incarnation / scope / provenance / read-write facts /
occurrence dependencyを一組として保持する。first/last armの選択やdependencyのゼロ化は行わない。
visible refの可能なreferentとhidden dependencyを同じalternativeに保持する。

最大16 alternativesはcompiler budgetでありlanguage limitではない。
field順のlexicographic sortとexact-fact dedupでarm順に依存しない。
coarse `NLDependencyKnowledge` は未実装の一般hidden facts用の既存分類であり、
refのexact factsを置き換えない。runtime phi/tag/bitmapは作らない。

inline alternativesはcontextのvalue tableが所有する。view getterはコピーを返す。
context cloneは独立したvalue tableへ全factsをコピーし、Copy value-useは同じfactsを保持する。
endは当該packageのcarrierを終了し、他のCopyのdependencyを終了しない。
context destroyでtableを解放する。追加のalternative専用heap/pointer/tableはなく、
value-table malloc/reallocがalternative storageのallocation pathとなる。
checked armはそれぞれhypothetical contextを所有し、parent artifactのdestroyで全armを破棄する。
source/public contextはartifactがborrowするため、その所有者がartifactより長く生存する。

## Public rebase proof

全armを別contextでtype/availability/effect検査し、arm contextをpublic contextへcommitしない。
ref resultのrebaseには以下のどちらかの証拠が必要。

1. **既存public package origin:** resultのcomplete factが実在するpublic packageのfactと一致し、
   cloneされたpublic prefixのplace incarnation/frame、scope chain、occurrenceが維持されている。
   同じ数値IDがあるだけでは足りない。branchのfresh local place/scopeはこの経路を通れない。
2. **Borrowed payload derivation:** pattern child scope、guardのroot-owned occurrence、child place /
   incarnation / access / provenanceを照合し、public rootのcurrent occurrenceとpayload place、
   親sum refのordinary scopeからfresh public result factを構築する。
   arm implementation用child scopeを延長/輸出しない。§26.16に従う構造的写像であり、
   任意のshort refのscopeを延長する規則ではない。

ref-valued joinではguard後のpublic-prefix memory/availability/scopeの変更を許可しない。
一般memory-state phiが必要な場合は `P7-REF-JOIN-PRECISION`。
既存P6のunit/flat Copy state joinはそのまま残る。

public current variantがexactかつrootがunchangedなら、そのvariantと異なるpayload
alternativeはunreachableと証明してpruneする。known Noneにhypothetical occurrenceは作らない。
known Someではpayload + fallbackの保守的may-setを保持してよい。
これはlanguageのpath correlationの消去ではなくcompiler approximation。
将来unknown-variant stateが導入された場合、このexact-variant pruneを証拠なしで適用してはならない。

## Supported / precision-limited matrix

| Use | P7 disposition |
| --- | --- |
| ordinary `ref<read,T>` / `ref<write,T>` result | exact static typeのflat nominal（LifetimeDomain除外）/ byte/u8/usize/addr targetをsupport |
| borrowed payloadがarm binding終了後survive | public parent scope + current payload occurrenceを保持 |
| ordinary binding / Copy use / block result | 全alternative保持、safe useで全factsを検証 |
| read-only/no-effect registered argument | type compatibility + 全alternativeのliveness/access/provenanceを検証。write→read context weakeningは既存規則、read→writeは禁止 |
| whole-sum store/replace/root-ending | liveな各alternativeのoccurrence/aliasをconflict検査。別alternativeのdead scopeで全may-setを消さない |
| joined write-through / swap / replace / store | `P7-SINGULAR-REF-PRECISION`。memory-state phiなしで任意referentを選ばない |
| joined ptr conversion | `P7-SINGULAR-REF-PRECISION`。provenanceやoccurrenceを落としてptrを生成しない |
| joined refをwrite parameterへ渡す | `P7-CALL-PRECISION`。preservation summaryを追加しない |
| exclusive / core authority / aggregate / sum-target ref result | `P7-REF-JOIN-PRECISION` |
| nested match/general CFG/function-return | 既存unsupported/precisionのまま |
| pre-existing external payload refがある同一rootの再match | `P6-EXTERNAL-OCCURRENCE`。既存guard制限をmay-set全体へ適用 |

`binding_use` / registered call expression argumentは `reference_live` で全候補を検査する。
各候補のscope chain、incarnation、conditional occurrence、provenance、referent type、accessが必要。
一候補でもdead/staleなら既存semantic diagnostic、証拠unknownならprecision diagnostic。
`conflicts` は全候補を列挙する。
exclusive reborrow / domain reference / raw observation / write / ptr conversionなどの
singular pathsはexplicit guardまたは生成可能typeの制限でjoined fieldを読まない。
ptr自体にalternativeを導入していない。

## Module API / failure contract

`nl_semantic_join_references(context, values, count, out)` は同じpublic contextにある
ordinary ref packagesをborrowし、全候補を検証してowned loose Copy resultを返す。
exact static typesが必要。inputsをconsumeしない。IDsの別context間混用はhost contract違反。
このAPIは一般CFGやsource-visible phi primitiveではない。

source checkもhost joinもclone → check → result construction → commit。
semantic failure / precision / resource limit / OOMではpublic stateとoutput IDsを変更しない。
local join bufferはstack-owned。candidate destructionで新value/arm allocationをすべて解放する。
new alternativeの上限超過は `NL_CHECK_RESOURCE_LIMIT`、全候補を保持できない場合にsuccessしない。

## Required evidence / non-goals

PW1〜PW5、first/last/reorder、Copy dependency、dead scope/stale facts、known None、wildcard、
private/hypothetical IDsのnonescape、access/type mismatch、write/ptr precision、late-arm failure、
common join failure、全allocation indexでのOOM rollback、16/17 alternatives境界を検査する。
PW4の真のscope終了は既存host `nl_semantic_end_scope` とsource safe-useを接続する。
sourceにscope-ending/function frontendを追加しない。

canonical Draft、parser/lexer、LLVM/backend、toolchain/lock/workflowは変更しない。
M9.3/F2/R4/relocation/FFI/一般CFG/新source annotationsへ進まない。
P7 READY FOR REVIEWで停止し、人間/Coordination review前にmergeしない。
