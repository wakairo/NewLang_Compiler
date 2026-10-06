# P13 — value-producing if production contract

Track: P。これはbounded production coverage / ownership contractでありnormative仕様ではない。
正本は`CURRENT_SPEC.md` → Draft 17.15（§19.1 / §21.8 / §27.3 / §27.4）。
対象は[Issue #84](https://github.com/wakairo/NewLang_Compiler/issues/84)。

## Core bool / source

`NL_TYPE_BOOL`はcanonical core identity、Copy + Discardable。
`nl_semantic_core_type(context, NL_TYPE_BOOL)`とsource type `bool`は同じidentityを返す。
host nominalで代用せず、physical size/alignmentを仮定しない。
既存seed value/root、abstract slot initialize/take、flat aggregate/sum payload、
ordinary body signature、signature-only pure result、flat Copy/ref-target joinで扱う。
P13はknown scalar bool encodingを導入しない。boolはparameter / semantic seed / pure host fixtureから供給する。

```text
if_expression := 'if' '(' expression ')' lexical_block 'else' lexical_block
```

parentheses/elseは必須。armは既存lexical block。
`if`自体にsemicolonはなく、non-tail expression itemだけ既存block規則で`;`を要求する。
no-else、direct else-if、general grouping、bool literal/operator/truthinessは追加しない。
condition/then/elseはdedicated `NL_SYNTAX_IF`のchildrenで、treeが所有しsource spansをborrowする。
condition delimiter / arm blockの内部ではmatch-scrutineeのbrace fenceを解除し、aggregate bracesを通常のexpressionとして読む。

中央P12 classifierのstructural setはexact/case-sensitive
`fn / let / return / match / if / else`。
`unit`は別core category。diagnosticは既存`P12-RESERVED-STRUCTURAL-NAME`を維持する。
source/hostのordinary function/parameter/local、multi receiver、aggregate shorthand、match payloadに適用する。
member/field/variant labelsは予約しない。loop/break/continueも予約しない。
malformed if/elseはordinary same-spelling lookupへfallbackしない。

## Finite outcomes / common state

conditionを一度value-useし、一つのnormal resultかつexact core boolを要求する。
そのpost-condition stateから両armを独立cloneして検査する。
conditionの値が既知でもarmを省略しない。これはchecker evidenceでありruntime execution/constant foldingではない。

| normal arm数 | disposition |
| --- | --- |
| 0 | 両function return edgeを検査してfunction-exit result/effectsを保守的に合流。checked IFはtype 0 / normal results 0 / terminates=true。unit/bottom/neverのnormal resultを作らない |
| 1 | terminated edgeをlocal normal joinから除外。唯一のnormal blockをcandidate IDsに対して再適用しresult/post-stateをforward。enclosing conditionは再評価しない |
| 2 | exact single-result type、outer non-Copy availability一致、bounded result/dependency/frame joinを要求し、common stateを構築。arm snapshot自体はcommitしない |

単一normal blockの再適用は、仮定付きsnapshotの新しいIDをpublicへimportしないためのchecker処理。
静的arm proofとcandidateへの適用は別interpretationであり、runtimeで両armを実行する意味ではない。
有限checking/replayの共通work counterは4096、既存depth 128 / table 4096 / owned evidence 64も維持する。
limitはhost resource diagnostic、language restrictionではない。

P6の`arm_frame` / availability / flat-Copy current-value widening / owned `save_arm`と
P7のcomplete-fact sorting / rebasingを再利用する。
unchanged frameは保持、ordinary flat Copy current-valueの更新はdependency-free unknownへwidenする。
non-Copy current-value mutation、raw/domain/scope/occurrence等の表現できない相関はprecision reject。
unit spellingのnormal resultは既存unit/no-responsibility conventionへ正規化する。terminating armへnormal unitは追加しない。

## Result identity / dependency

- same-type dependency-free flat Copy resultはfresh abstract unknown Copy packageへ合流できる。
  ref/ptr/aggregate/authorityをtypeだけでplain Copyとみなさない。
- same-origin flat non-Copy resultは、両armで同じ**pre-existing prefix ValueId**をtransferする証拠がある場合だけそのidentityを保持する。
  common candidateでouter bindingを一度consumeし、returned responsibilityを終了させない。
- distinct non-Copy identitiesは`NL_CHECK_ANALYSIS_PRECISION_LIMIT` / `P13-JOIN-PRECISION`。
  typeだけからfresh packageを捏造せず、一方のbranchも選ばない。
- richer sum/aggregate/core-authority identity algebraはprecision boundary。sole-normal forwardは既存body profile内のpackage transferを維持する。
- ordinary ref resultはP7のmay-setでplace/incarnation/scope/provenance/read-write/occurrence dependencyを全て保持。
  実在するpublic package originとunchanged prefixを照合してrebaseする。
  private place/scope/typeのnumeric ID一致だけではimportしない。
- terminated armはnormal ref alternativeを増やさない。
- safe consumersは全alternativeを検査する。joined write/ptr conversionは既存P7 precision fence。
  richer hidden dependencies / ref+memory mutation / unrepresented correlationsはprecision reject。

## Function boundaries / nesting

P11 source-declared / host-registered definitionsとactual callsは同じIF engineを使う。
formal validationだけでactual alias factsを免除しない。P8 ordinary-ref result fenceは維持する。
P9 matchのzero/one-normal-arm / concrete variant / nested-match precision boundariesも維持する。
IF内match、match arm内IF、nested IF、arm returnを既存lexical descendantsとして合成する。

return edgeは**local IF normal continuation**へ混ぜないが、function callerにはそのreturnがresult/effectsの可能性として残る。
function completion時、owned IF exit evidenceとnormal tailを比較してcaller prefixのflat Copy effects / resultを合流する。
non-Copy return-vs-tailが別identityならprecision rejectする。
非終端armの内部return evidenceも辿り、祖先branchのValueId / current-fact prefixの最小値を使ってhypothetical identityを輸出しない。
fork後に同じ番号が割り当てられたcurrent-value/factは同一事実とみなさず、Copyはwiden、非flat stateはprecision rejectする。
P9 matchは独自のconcrete-variant interpretationを持つため、そのguarded armをIFの一般return phiへ混ぜない。
body-backed callのterminating callee resultはcallerにとって通常call resultである。
non-Discardable scope obligation / function-exit dependency checkを維持し、implicit cleanup/restore/dropを生成しない。

## Owned evidence / failure

`NL_CHECKED_IF`はcondition、normal_arms、normal resultまたはterminates/returnedを保持する。
`nl_checked_if_arm(parent, id, 0/1)`はthen/elseのborrowed `NL_CHECKED_IF_ARM` artifactを返す。
各arm artifactは独立semantic snapshotとnested artifactsを所有する。
そのIDは`nl_checked_context(arm)`専用。public contextのIDとして解釈しない。
parent destructionは全nested arm/context/body evidenceを一度解放し、semantic responsibilityをdiscardしない。
P11 retained body planがsource/treeのdurable lifetimeを保証し、registration入力owner破棄後もcallできる。
通常fragment sourceはartifactをoutliveする必要がある。

public check/registerはclone → check → sum/raw invariant validation → commit。
semantic/precision/OOM/resource failureでpublic state / output owner slotは変わらない。
syntax nodes、context clone、arm evidence、inline ref alternativesを含むvalue table、common join、retained body/commit前allocationをfault-injectする。
first diagnostic/category/spanはdeterministic。

## Diagnostic / review boundary

| condition | code/class |
| --- | --- |
| parentheses / then / else / else block | P13-IF-OPEN / CLOSE / THEN / ELSE / ELSE-BLOCK、parse error |
| misplaced else | P13-ELSE-CONTEXT、parse error |
| non-bool / zero or multiple normal condition result | P13-IF-CONDITION、semantic error |
| two normal result mismatch | P13-IF-RESULT、semantic error |
| non-Copy availability mismatch | existing P6-AVAILABILITY-JOIN、semantic error |
| distinct / correlated unrepresented result-state | P13-JOIN-PRECISION、analysis precision |
| richer persistent frame / ref origin | existing P6-JOIN-PRECISION / P7-REF-JOIN-PRECISION |
| bounded work exhausted | P13-BRANCH-WORK-LIMIT、resource limit |

Draft files、toolchain/lock、LLVM/CLI compilationは変更しない。
loop/backedge/general CFG/fixpoint、F/R、backend/relocation/FFI/modules/concurrency、次P milestoneを開始しない。
open/unmerged PRとexact-head validationをreviewへ渡し、P13 READY FOR REVIEWで停止する。
