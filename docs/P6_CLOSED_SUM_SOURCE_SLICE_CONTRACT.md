# P6 — Registered closed-sum source slice contract

Status: production review contract。言語仕様の代替ではない。

## Authority / scope

開始時 main は `2b327edc000a913a49adb7cfd5f78a95c1e931b8`。
`docs/reference/CURRENT_SPEC.md` が指す Draft 17.10 の §26、§17.4、§19.1 を正本とする。
Backend Contract v0.4、review済み運用方針、merged P/F/R evidence が続く。
[Issue #35](https://github.com/wakairo/NewLang_Compiler/issues/35) と
[Phase A audit](https://github.com/wakairo/NewLang_Compiler/issues/35#issuecomment-5995873897)
は implementation handoff / evidence であり、仕様を変更しない。
F1.2 は FormalProof `a3a8c70becb3da6a9c2fc6f91ca578f49b19df32` の report を確認した。
Lean の ghost history / Finset 等は採用要件にしていない。

P6 は concrete registered sum と constructor / match の source→checked vertical sliceだけを扱う。
C17 / toolchain / parser旧4 entry / CLI / raw-memory semantics は既存基盤を使用する。
一般function frontend、R3、F2、M9.2、LLVM lowering、relocationを開始しない。

## Supported / deferred matrix

| 領域 | Supported | Deferred / failure class |
| --- | --- | --- |
| 登録 | nominal type、1–16 variants、payloadless / unary、同一sum内の名前一意、全payload型によるCopy/Discardableのconjunction | source sum declaration、generic instantiation、nested sum、aggregate/ref/ptr payload はsemantic unsupported。16はhost budget |
| payload | flat nominal/unit/scalar、既存Storage/Allocation/slot/LifetimeDomainのvalue package | core authorityの一般user-call移送 / 合流はsummary不足としてunsupported / precision |
| constructor | `Sum.Variant` / `Sum.Variant(expr)`、exact qualification、variant resolution後のshape/arity、ordinary value use | inference / associated/member lookupへのfallbackなし。invalid shapeはsemantic error |
| match syntax | `match expr { Variant => block, Variant(name) => block, Variant(_) => block, }` | standalone wildcard、qualified/nested/guard/OR/literal/general pattern はsyntax unsupported |
| punctuation | `=>`のbyte adjacency、comma、optional trailing comma、newlineはwhitespace | missing/non-adjacent arrow、missing separator、non-block bodyはsyntax error |
| patterns | static sum自身のvariant resolution、exactly-once exhaustiveness、payload shape | empty arm listはparseしてsemantic exhaustiveness error。known current variantでも全armを検査 |
| consuming | Copyは独立package copy、non-Copyはwhole consume、payloadのfresh arm bindingへの移送、wildcardのstatic Discardable検査 | partial moveなし |
| borrowed | ordinary read/write parent ref、同modeのpayload ref、parent scope + explicit occurrence dependency。wildcardはcapabilityなし | ptr/exclusive-ref scrutineeにimplicit deref/borrowなし。escaping ref resultはsemantic unsupported |
| transitions | whole replace/storeのroot incarnation維持とoccurrence freshening、payload replace/storeのoccurrence維持 | sum / conditional payload swapはsemantic unsupported。既存scalar swapは維持 |
| normal join | exact result type / result count、outer non-Copy availability一致、unit / flat Copy result widening、sum identity forwarding | general CFG、variant-varying affine result、複雑なpersistent stateはprecision rejection |
| terminating arms | W1の非終端equivalent | true return/function frontendはCOMPILER-IMPLEMENTATION-LIMIT。Draftのterminating ruleは未実装 |
| ptr pressure | payload ref→ptrはnonblocking。whole transition後のstale ptrはsafe acquisition前にreject | live conditional ptr→refのP3 loan body planはsemantic unsupported。一般occurrence-aware acquisition frontendは今後のtargeted work |

constructorはzero/multiple argumentsも構造として保持してからsemantic shapeを診断する。
valid unary formにtrailing commaはない。旧P2 expression entryは拡張せず、P5 source fragment entryを拡張する。
match scrutineeの後ろの`{`はarm delimiterとして処理し、call argument内のaggregate/blockとは区別する。

## Identity / ownership

`NLSemanticContext` はtypes、variant names、bindings、values、places、scopes、occurrence historyを所有する。
登録入力の名前・variant arrayはcall中だけborrowし、成功時に名前をcopyする。
context cloneはowned names / raw intervals / occurrence tableを独立copyする。

| State | 意味 / owner |
| --- | --- |
| TypeId + variant index | exact nominal sumのclosed shape。別sumの同名variantを混同しない |
| ValueId / sum_payload | sum semantic valueとactive payload semantic value。loose memberは`NL_CARRIER_SUM` + sum_ownerで所有関係を示す |
| PlaceId / root incarnation | compiler-managed memory site / typed root incarnation。whole replaceでは維持 |
| current_fact | current semantic value fact。payload更新でもparent/payloadのfactを更新 |
| OccurrenceId | rootに条件付きで存在するmemory payload occurrence。append-only historical tableの別identity category |
| conditional child place | occurrenceのcurrent payloadを保持する内部site。parent_sumを保持しindependent_root=false、BackingRangeなし |
| ref facts | parent scopeのchild scopeとoccurrence_dependencyを同時に保持。provenanceとは別 |
| ptr facts | historical child place/incarnationへ向くprovenance。occurrenceを維持するdependency / ref scopeは持たない |
| BackingRegion / occupancy | P4のvalue-owned raw responsibility / root-owned physical placement。occurrenceからmintしない |

Cで各IDをsize_tとしても、category/contextを跨ぐ交換は禁止する。
conditional childのincarnation番号は内部provenance handleであり、独立してtake/destroyできるtyped rootを追加しない。
同physical address・同variant・同payload typeでも、whole transitionはold occurrenceを再利用しない。
loose sum / returned old sumはpayload valueを所有し、source occurrenceを所有・移送しない。
Copy sumはpayloadを含め別ValueIdを持ち、original ownership bookkeepingをaliasしない。

`src/sum.c` はattach/detach、payload fact propagation、core authority判定、structural invariantを担当する。
install / source binding / initializeはattach、move / lexical discard / take / destroyはdetachを使用する。
whole storeはstatic whole-type Discardableを先に検査し、old packageをreturned resultとして公開せず終了する。
payload-only updateは同じchild place/occurrenceを保持しparent current package/factだけを更新する。
live scopeのoccurrence-dependent refがあるwhole updateはrejectする。
whole-sum writeを持つregistered user-callはoccurrence preservation summaryがないためprecision rejectionとする。

## Bounded arm checking / common post-state

1. public candidateでscrutineeを一度だけvalue-useする。
2. 全patternをstatic closed shapeに照合する。
3. 各armはpost-scrutinee candidateを独立cloneし、該当variantを**そのarm snapshotだけ**で仮定する。
   異なるvariantのpayloadはtype-correct unknown packageを使う。未確定raw claimをpublic stateへmintしない。
4. pattern bindingをarm lexical blockのbinding floorに含め、scope-exit obligationsを検査する。
   borrowed payload scopeはparent ref scopeに依存し、arm終了時に閉じる。
5. 全armのexact normal result typeとouter affine availabilityを照合する。
6. persistent frameを比較し、下記でcommon stateを**新たに構築**する。arm contextはcommitしない。

Common-state rules:

- unit / flat Copy result: fresh dependency-free unknown packageへwidenする。constant knowledgeは保存を主張しない。
- 全armで同じouter flat affine bindingをconsume: common candidateでもconsumeする。core authority / sum / backing transitionの一般joinはprecision rejection。
- ordinary flat Copy placeの更新: unknown current valueへwidenしincarnationを保つ。
- borrowed payload-only update: 全armでwhole transitionがなくoccurrenceが維持されたとき、actual active payloadをunknown Copy valueへwidenする。
  actual variantがpayloadlessならpublic payload/occurrenceを作らない。
- borrowed whole update: 全armがwhole updateし、incoming variantが同一でstatic Copyの場合だけ、unknown incoming payloadでcommon transitionを作る。
  whole/no-write混在、incoming variant不一致、non-Copy payload update等はprecision rejection。
- consuming sum result: 各armでown variant・own payloadを同じsumへ再構築するidentity transformerを確認した場合だけ、held input packageをfresh resultへforwardする。
  non-Copy payloadは同じValueId、Copy payloadは直接pattern identifierのcopyというbounded evidenceを使う。
  **known current armを選ぶ最適化ではない**。全caseでidentityが証明できた結果として元のactual value/Storage claimを保持する。

existing domain / scope / raw region stateを変える一般的なbranch effectは合流しない。
pre-existing external payload refはactive variantとのrelational factを含むので、その状態で別variantを仮定するborrowed matchは`P6-EXTERNAL-OCCURRENCE` precision rejectionとする。
不整合なhypothetical scope/dependencyを作ってsemantic errorやsuccessを導かない。
知らないpost-stateを成功として扱わず `P6-JOIN-PRECISION` を返す。

## Checked artifact lifetime

parent checked artifactは通常public context/sourceをborrowする。
match arm evidenceはparent artifactがarrayを所有し、各arm artifactは独立したhypothetical semantic contextを所有する。
`nl_checked_match_arm(parent, match_id, index)` はsource-orderでborrowed evidenceを返す。
armのsemantic IDsは **`nl_checked_context(arm)` だけ**で解釈し、public contextのIDsとして使用しない。
arm pattern / body / qualifier / variantのsource spanとresolved identitiesを保持する。
sourceはparent/arm artifactより長く生存させる。parent destroyはarm artifact、arm context、arraysを一度ずつ解放する。
semantic valuesのdiscardはartifact destructionでは行わない。
static libraryの循環linkを作らないため、armに限定したconcrete context destructor callbackを保持する。
allocator/framework/callback-based semantic engineは追加しない。

## Failure / budgets / diagnostics

register/checkはclone→check→validate→commit。syntax、semantic、unsupported、precision、OOM、resource-limitの失敗ではpublic context・output owner slotを変更しない。
partial owned tableはNULL-initして既存destroyでcleanupする。append-only occurrence allocationがcandidate内で途中失敗してもpublic historyへ残さない。
variant/name clone、constructor/root attach、whole replace、arm snapshots/evidence、common joinのmalloc/realloc pathをtest-only fault injectionで網羅する。
全successful committed stateでsum ownership/occurrence構造とP4 raw occupancy invariantを検査する。

budgets: variants/type 16、owned arm evidence/parent artifact 64、各semantic table/node 4096、depth 128。
これらはlanguage limitsではなくhost resource diagnostics。
first diagnosticはdeterministicでbyte span/category/codeを保持する。
syntax error / syntax unsupported / semantic error / semantic unsupported / precision / hostを区別する。

## Review gate

P6 contractはこのbounded implementationをreviewするための記録であり、canonical Draftの保証範囲を縮小しない。
W1–W4、negative controls、OOM/rollback、旧P0–P5 testsとcurrent-head CIをreport/PRへ記録する。
P6 READY FOR REVIEWで停止し、自身ではmergeしない。
