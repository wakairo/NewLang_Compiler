# NewLang North Star Product Validation

> Status: Track V North Star product validation contract
>
> この文書はcurrent beachhead / North Star experimentのproduct-level contractを定める。
> language semanticsのnormative authorityは `docs/reference/CURRENT_SPEC.md` が指すDraftであり、本書はそれを上書きしない。
> implementation slice固有の詳細は各Issue / contract / reportを参照する。
> 変更履歴はGit commit historyを正とする。

## 1. North Starの目的

North StarはNewLangの最終市場を定義するためではなく、最初のsharpなvalue hypothesisをreal workloadで検証するために置く。

中心仮説:

> **Cの低水準architecture / pointer topologyを維持したまま、lifetime・ownership・responsibility bookkeepingの重要部分をcompilerへ移せるなら、Cが現在合理的に選ばれている一部のsystems workloadで実用価値が生まれる。**

North Starの成功はNewLang全体のC replacement達成を意味しない。失敗した場合も、直ちにfeature追加で救済せず、product hypothesis自体を再検討する。

## 2. Primary target user

- skilled systems programmer
- Cを選ぶ理由を理解し、manual ownership / pointer topologyを意図的に扱っている人
- GCを使えない、または使いたくない低水準componentを実装する人
- Rustの所有権modelへarchitectureを大きく合わせ直すことに価値を感じないworkloadを持つ人

最初のbeachheadはsingle-threadedまたはexternally synchronizedなmemory-owning low-level library / subsystemを中心とする。

## 3. Representative workload — cJSON

North Starのfrozen upstream workload:

- repository: `DaveGamble/cJSON`
- upstream commit: `6d9f2443ab071f86e5d9b43025a40929ec41c46c`
- scope files: `cJSON.h`, `cJSON.c`, `tests/misc_tests.c`, `tests/common.h`

対象scope:

- allocation / creation
- suffix / add / linking
- reference creation / add
- detach
- replace
- recursive delete
- owning / reference distinction

Parser / printer / string / number handlingはcurrent North Starのprimary scope外とする。

cJSONをtoy linked-listへ縮小してはならない。real ownership / detach / recursive destruction / reference distinctionを残す。

## 4. なぜcJSONか

このworkloadは、NewLangが価値を主張したい複数の要素を小さなC library内に同時に含む。

- node-per-allocation
- persistent pointer links
- explicit detach / responsibility transfer
- replacement
- recursive destruction
- owning objectとreference objectの区別

また、graph invariant全般をlanguage safetyへ過剰に取り込まず、memory/lifetime/responsibility bookkeepingだけを比較しやすい。

## 5. Expected NewLang mapping

product-levelの期待対応は概念的に次である。

- links -> `Option<ptr<Node>>`等のpersistent locator
- access -> scoped `ref`
- allocation -> `Allocation` / `Storage` / `slot` / `initialize`系mechanism
- release -> `take` / `destroy` / `Storage` / deallocate系mechanism
- mutation -> write ref + `replace` / `store`
- traversal -> loop-carried state

ここでのspellingは実装authorizationではない。canonical Draftが最終authorityである。

## 6. Topology preservation

North StarではNewLangを通すために元workloadを別architectureへ書き換えて成功扱いにしない。

Topology scoreは次の6項目を追う。

1. node-per-allocation
2. persistent ptr links
3. explicit detach
4. explicit replace
5. recursive destroy
6. owning / reference distinction

PASSには原則 **5/6以上** を要求し、critical topologyを0にしてはならない。

per-hop `unchecked` budgetはallocator / platform boundary等のlocalized boundaryを除き **0** とする。

## 7. Bug corpus

少なくとも次のbug classをpre-registerする。

- **B1** — scoped accessがliveなままdestroy
- **B2** — destroy後のstale ptrからsafe accessを再取得
- **B3** — double release
- **B4** — consumed responsibilityの再利用
- **B5** — branchでresponsibilityを失う
- **B6** — invalid dependencyをloopへcarry
- **B7** — link transitionを越えて`Option<ptr>` payloadを不正保持
- **B8** — reference childをowning childとして誤destroy

B8はstatic safetyではなくpolicy/modeling問題として残る可能性を許す。

## 8. Human Obligation Ledger

比較時には単なるline countではなく、人間が正しく維持する必要のあるobligationを分類する。

- **L — Lifetime**: object/rootの開始・終了・stale access
- **R — Responsibility**: owner / detach / release / transfer
- **P — Pointer / access**: locatorとsafe access authorityの区別
- **V — Value**: current semantic value / consume / replace
- **C — Control**: branch / loop / early exitでobligationがどう流れるか

各obligationのoutcomeは例えば次へ分類する。

- statically eliminated
- checked but explicit
- moved into localized unsafe abstraction
- runtime checked
- convention
- unchanged
- out-of-scope

重要なのはsyntax量ではなく、bugを防ぐために人間が保持し続けるsemantic burdenがどう変わるかである。

## 9. PASS criteria

North Starは少なくとも次を満たしたときにPASS候補とする。

- scoped behaviorが正しい
- topology score >= 5/6かつcritical topologyを保持
- per-hop uncheckedなし
- B1-B7のうち少なくとも5 classをstatic rejectionできる
- C / Zig / topology-preserving Rust等と比べてmeaningfulな差がある
- single assignment等のceremonyがmanageable
- broad-feature rescueを必要としない
- diagnosticsが実用的

## 10. FAIL / stop conditions

次はFAILまたはproduct hypothesis再評価の強いsignalとする。

- topologyを大きく書き換えないと成立しない
- pointer traversalごとにuncheckedが必要
- bug corpusに対するstatic deltaが弱い
- semantic ceremonyがCのmanual bookkeepingを別表現で再生産する
- 一つのwitnessを通すために無関係なlarge feature setを追加する
- diagnosticsが原因箇所・obligationを実務的に示せない

FAIL時はまずproduct thesis / target / workflowを疑い、feature追加だけでPASSへ寄せない。

## 11. Anti-gaming rules

次をNorth Star成功の代用にしない。

- host-seeded semantic factだけでpositive witnessを作る
- toy structureへ縮小する
- Checked-C / runtime / C UBにNewLang safety judgmentを委ねる
- forbidden inputをparserで雑に落としてsemantic safetyと数える
- backend unsupportedをlanguage-invalidへ読み替える
- unsafe wrapper内へobligationを隠し、outside budgetとして数えない
- exact workloadに特化したspecial caseをgeneral capability evidenceとして扱う

primary evidenceは可能な限り **actual source -> production checker -> checked artifact -> executable / static rejection** のvertical pathを要求する。

## 12. Intermediate product gates

frozen cJSON experimentを実行可能にするまでの間、failure attributionを高めるためbounded product gateを挟んでよい。

これまでの代表例:

- V0 — minimal executable spine
- V1 — typed scalar
- AVS — aggregate value slice
- local-root ptr/ref gate — persistent locatorとscoped accessの分離

intermediate gateはNorth Starの代替ではない。目的は、一度に多くのmissing surfaceを混ぜず、real-source evidenceを段階的に増やすことである。

各gateは:

- semantic deltaの有無
- actual-source primary witness
- positive / negative acceptance
- Checked-C等のbounded lowering boundary
- stop condition
- next sliceを自動authorizeしないhandoff

を明示する。

## 13. Current sequencing principle

次sliceは前sliceの完了だけでは自動的に始めない。

Track Vがvalue hypothesisを再査定し、Coordinationがscopeを裁定し、必要ならMでsource/semantic gapを閉じ、その後にbounded P implementationをauthorizeする。

```text
V value question
  -> Coordination scope adjudication
  -> targeted M if needed
  -> bounded P
  -> V evidence assessment
  -> independent review / closure
```

## 14. Explicit out-of-scope invariants

current North Starでは、少なくとも次をlanguage safetyとして当然には要求しない。

- list membership correctness
- `next.prev` consistency
- cycle freedom
- same-node insertion prohibition
- JSON-kind application policy

これらがreal product value上重要になれば別のhypothesisとして扱う。

## 15. North Starと長期戦略の関係

North Starから外れたcapabilityはNewLangのultimate envelopeから除外されたことを意味しない。

長期的なtarget ladder / practical C-replacement envelope / no-foreclosure ruleは `NewLang_Product_Value_Strategy.md` を参照する。

C replacement coverageの継続的な不足管理は `NewLang_C_Replacement_Coverage_Map.md` を参照する。
