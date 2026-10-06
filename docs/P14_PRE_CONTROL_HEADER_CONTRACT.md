# P14-pre — control-exit / header substrate contract

Track: P。Issue [#95](https://github.com/wakairo/NewLang_Compiler/issues/95)だけのproduction prerequisite。
main `0cbc171c621334bf4ca7301ddfefb9293d7bb75a` の運用方針を先に確認した。
CURRENT_SPEC → Draft 17.17、Backend Contract、merged evidenceの順でauthorityを保持する。
F2 CLOSED（FormalProof `e591e53f295d4ab76d00d3bc1a2cdc443e06b7b4`）はproof evidence。
Git historyを変更履歴の正とする。本書はnormative specificationではない。

## Exit / normal outcome

内部module `src/control.h` / `control.c` が、normal continuationと独立したowned finite exit集合を提供する。
`NLControlExitView` はReturn / Continue / Break、nominal target、result配列、owned post-stateを保持する。
Returnはfunction target、Continue/Breakはloop targetにしか接続できない。
NULL集合はemptyで、`NLControlOutcome {normal=false, exits=NULL}` はzero-normal-exit。
Return unitはcount=1の実在するReturn edgeであり、empty outcomeとは異なる。

`nl_control_outcome_join` はIF/MATCH alternativesを全てunionし、normalをORする。
`nl_control_outcome_then` はlexical sequenceで、左にnormal continuationがあるときだけ右を含める。
左がterminatingなら後続のexitやnormal状態を捏造しない。
結果を一つのarmへ代表化せず、重複も勝手にdedupしない。

既存production `source_return` はresult transfer / function-exit checkの後でowned snapshotを記録する。
lexical blockは同じartifactへ集約し、IFは両arm、function MATCHは全guarded armのexit集合を保存する。
sole-normal IFの再適用はpublic normal stateを構築するためで、既に保存したexit集合を再度追加しない。
`normal == 0`だけではReturnと判定せず、両armのexplicit Return/target evidenceを要求する。
`body_result`は全Return snapshotのtype / target / dependency exitを検査し、unconsumed loop exitをinternal errorにする。
no-normal + empty exitsはfake Returnなしで許可する。zero-exit calleeはnormal call valueを生成しない。

`Check.terminated` / checked `terminates`は**normal continuationの不在**を表す互換fieldとして残す。
`returned`は既存return-only result/state joinで作る互換summaryであり、exit classificationには使わない。
P13の有限result/current-state join、P7のref alternatives、P9のactual-variant MATCH interpretationは維持する。
control集合は全static evidenceを持つが、full conditional memory/result join frameworkではない。
混在controlのsource admissionとloop内での消費はP14の仕事である。

## Nominal target / origin certificate

targetはopaque owned handle。semantic ValueId、fact number、source spellingから作らない。
生成ごとに別handle、evidenceはretainするため生存中のallocator address再利用によるaliasはない。
function checkingはfresh function targetを作り、finite armだけがそれを継承する。
calleeは別function targetを持つ。nearest-loop source stackは追加していない。

`NLControlState` はcontextのowned cloneとroot-origin certificateを持つ。
root作成は独立origin、forkは同じoriginをretainする。
certificateのValueId / current-fact prefixだけが共有identityの証拠であり、post-fork数値の一致は証拠ではない。
既存semantic APIをowned stateに適用できるが、同じoriginでもroot-prefix外のfresh identityを共有としない。
semantic IDを他のsnapshot contextへ直接使ってはならない。

## Hとinclusion

`NLLoopHeader` はowned entry certificate、nominal loop target、固定slot数/type、Copy abstraction方針を持つ。
slot配列のindexがsymbolic correspondenceであり、symbolic slotのためのruntime packageを生成しない。
carried inputはloose responsibility。non-Copy slotの重複、carrier不整合、type不一致はrejectする。

- flat dependency-free Copy: wide modeで任意の同型content/current valueを包含する。known scalarを選ばない。
- flat unchanged non-Copy: root/public prefix由来の同じpackageだけを一責任として保持する。
  static type一致だけのtransformed/fork-local packageはprecision reject。
- captured outer non-Copy: availabilityはentryとexact一致。MaybeConsumedへwidenしない。
- outer flat Copy place: wide modeではcontent/current factをunknownに包含する。
  incarnation、place/root/domain/backing frameはexact。
- external ref blocker: root-prefix packageのcomplete alternative facts / carrier / scope frameが全て一致するときだけ保持する。
  refはcarried slotとしてはまだprecision reject。fresh ref alternative、hidden dependency、Unknown dependencyは独立扱いにせずprecision reject。
- scope/domain/rich occupancy/correlation: exact frameまたはprecision reject。raw regions/conditional occurrencesはこのHでは未対応。

strict modeはentryのexact scalar/shared-origin/current-fact情報だけを表す。
changed Copy current factはstrict Hから外れる。post-fork current-fact/value IDの偶然一致をstrict certificateにしない。
wide modeはより大きなHで、entryと変更後successorの両方を包含できる。

`nl_loop_header_create`はentry inclusionを検査してからpublishする。
`nl_loop_header_closure`は**提供された全Continue edge**のtarget / arity / type / responsibility / availability / inclusionを検査する。
Return/Breakはrecurrenceへ入れず、別loopのContinueを同じtargetとしない。
1本目のsuccessで終了せず、後続failureは全体failureになる。

### Proof boundary

本substrateのclosureは**供給されたsuccessor evidenceに対する包含検査**である。
source bodyをH上で解析して全statically reachable edgeを供給すること、transferがHの任意のrepresented inputにsoundであることはcallerの義務。
fixture entryから得た一回の実行をfull inductiveness proofと呼ばない。
将来P14はabstract header入力・iteration-local projectionをこのcontractへ接続し、全edge coverageを確立する必要がある。
F2のentry + every continue closure → arbitrary finite reachabilityという原理に対応するが、C testsはLean proofの代替ではない。
exact least fixed point、termination、general CFG、nested fixpointは実装しない。

## Ownership / failure / budgets

exitはtargetをretainし、stateをforkして独立post-state cloneを所有する。result IDsはそのclone内だけのborrowed責任参照。
artifact解放はsemantic discard/cleanupではない。snapshotをdestroyしてからtargetをreleaseする。
artifactのexit集合、arm/body evidence、body planはいずれもowned。通常source lifetimeとP11 durable registered-body lifetimeは従来どおり。

appendはclone/retain/growth完了後にだけpublish。union/outcome compositionはfresh candidateを構築してからswapする。
headerもclone/entry inclusion後にだけpublish。OOM/precision/resource/semantic failureでpublic contextは変更しない。
existing source checkerはwhole-context clone/check/commitを維持し、新record/unionのfailureもwhole candidateをrollbackする。

boundsはcompiler budgetでlanguage limitではない。

| Budget | Limit / failure |
| --- | --- |
| exits per artifact/outcome | 64、resource rejection。edgeを捨てない |
| header/carried slots | 16、resource rejection |
| state tables / source depth / finite work | 既存4096 / 128 / P13 replay boundを維持 |
| ref alternative storage | 既存16、全factsを保持できなければreject |
| worklist/fixpoint iterations | 使用しない。finite supplied edgeごとのinclusion walk |

inclusionはslot、binding、place、scope、domain、valueを有限順序で検査する。
unionはsource/入力順の全edgeを保持し、meaningはset union。edge順を入れ替えてもaccept/rejectは同じ。
一般hidden/Unknown dependency、transformed non-Copy、rich cyclic state、fresh private originsは`NL_CHECK_ANALYSIS_PRECISION_LIMIT`。
型/availability/責任不整合はsemantic failure、不正なtarget categoryや未消費loop exitのfunction boundaryはinternal error。
allocation/cap failureはOOM/resource statusであり、successに変換しない。

## P14へ残す作業 / non-goals

source loop/continue/break grammar、structural-name extension、initializer/parameter receiving、nearest-loop source context、
H上のbody transfer、iteration-local binding/scope projection、continue recurrenceとbreak finite join、source workload matrixは未実装。
#94を再開しない。新しいM/F/R、LLVM、relocation、FFI、modules、concurrency、general CFG/SSAやrecursive SCCへ進まない。
