# NewLang C-Replacement Coverage Map

> Status: Track V strategic coverage ledger
>
> この文書はNewLangの **practical C-replacement envelope** を長期的に監査するためのproduct/strategy ledgerである。
> language semanticsのnormative authorityは `docs/reference/CURRENT_SPEC.md` が指すDraftであり、本書はそれを上書きしない。
> current implementation stateは変化するため、個別milestoneのexact evidenceはIssue / PR / report / Git historyを参照する。

## 1. 目的

North Starを狭く保ちながら、NewLangが将来Cの実用領域を置換できなくなる設計上の穴を見落とさないために使う。

このmapはroadmapでもv1 feature checklistでもない。

特に:

> **Deferred from the current slice != Rejected from the strategic envelope.**

を明示する。

## 2. Coverage status

- **BEACHHEAD-VERIFIED** — bounded actual-source product evidenceがある。
- **SEMANTIC-BASIS** — canonical semantics / production semantic machineryに基礎があるが、real workload/product coverageは未完了。
- **DEFERRED-ADDABLE** — 今はdeferするが、既存設計から後付けできるevidence / researchがある。
- **STRATEGIC-GAP** — C replacement上重要だが、十分なaddability / implementation evidenceがまだない。
- **OPEN** — 必要性または解法自体を追加調査する。
- **OUTSIDE-ENVELOPE** — C互換のためだけに存在し、NewLangの実用C replacementには原則不要。

statusはlanguage validityを意味しない。

## 3. Coverage ledger

| Capability area | Current strategic status | C replacement上の意味 / 現在の見方 |
| --- | --- | --- |
| scalar / lexical value flow / function body | BEACHHEAD-VERIFIED | bounded actual-source executable spineで基礎を検証済み。一般numeric family等は別問題。 |
| nominal aggregate value | BEACHHEAD-VERIFIED | bounded aggregate construction / whole destructuringをactual sourceで検証。layout保証とは分離。 |
| persistent ptr + scoped ref | BEACHHEAD-VERIFIED | local-root gateでlocatorがscopeを越え、safe accessはscope/livenessに従うことを検証。 |
| stale ptr after EndRoot | BEACHHEAD-VERIFIED | ptr token自体は残り得るがsafe reacquisitionはrejectする境界を検証。 |
| lifetime-preserving mutation / replace | SEMANTIC-BASIS | `replace`はcurrent value/factを変えincarnationを維持するcanonical basisあり。actual-source write acquisitionのproduct gateは別途必要。 |
| ordinary write loan source mapping | SEMANTIC-BASIS | Draft 17.19でNorth-Star-onlyのdirect lexical-local `u8` mappingをsemantic delta 0でcanonical化。actual-source production/product evidenceはstable-root mutation gateで未検証。 |
| field projection | SEMANTIC-BASIS | safe typed projectionはcanonical。general source profile / product evidenceは未完了。 |
| field mutation | SEMANTIC-BASIS | field-level Change / replace-storeのsemantic basisはある。actual-source projection + write pathは未検証。 |
| recursive nominal / intrusive graph shape | STRATEGIC-GAP | cJSON等へ重要。declarationだけではfalse progressになり得るため、construction/mutation/topologyを伴うwitnessが必要。 |
| sum / optional persistent link | SEMANTIC-BASIS | closed sum / conditional occurrenceのsemantic machineryあり。`Option<ptr<Node>>`型のreal topology evidenceは今後。 |
| root lifetime start/end | SEMANTIC-BASIS | initialize/take/destroyとincarnation modelあり。North-Star real-source full lifecycleはまだ限定的。 |
| Allocation / Storage / slot | SEMANTIC-BASIS | raw/typed occupancy semanticsとproduction testsあり。一般user-facing source pathは未完成。 |
| dynamic container raw-vacant bridge | DEFERRED-ADDABLE | runtime metadataをSSOTにするlocalized privileged bridge方針。一般化しすぎないことが重要。 |
| allocator / deallocator integration | STRATEGIC-GAP | C-class workloadへ重要。language semantics、runtime/library、source APIの境界を段階的に検証する必要。 |
| function pointer / indirect call | DEFERRED-ADDABLE | direct-call body-sensitive modelを壊さずknown-target / unknown-summaryへ拡張できるかを継続監査。 |
| captureless callback + context pointer | DEFERRED-ADDABLE | C FFIの実用最小subsetとして重要。escaping closureとは分離する。 |
| C ABI / FFI | DEFERRED-ADDABLE | C scalar / pointer-like / callback / external global等のminimal boundary候補あり。full C++ ABIやvariadicは別扱い。 |
| external backing / mmap / shared memory | STRATEGIC-GAP | C replacementには重要。safe BackingRegion nonalias invariantを壊さないimport contractが必要。 |
| MMIO / BAR / volatile | STRATEGIC-GAP | platform codeで重要。ordinary memory semanticsとvolatile/device effectを分離する必要。 |
| DMA / physical-vs-virtual alias | OPEN | safe independent backing identityとの整合が難しい。one region + views等を追加検証する。 |
| in-place / caller-allocated / out-pointer construction | DEFERRED-ADDABLE | whole-value + initializeをbaselineとし、FFI/opaque objectで必要なlocalized boundaryを後付け可能か監査。 |
| exact / packed / offset layout | OPEN | ordinary aggregate layoutはopaqueを維持。real workloadで必要ならboundary-specific mechanismを優先し、coreへ安易に入れない。 |
| freestanding / embedded execution | STRATEGIC-GAP | C replacementの重要領域。runtime assumptions、toolchain、startup、platform interfaceの実証が必要。 |
| atomics / concurrency | STRATEGIC-GAP | v0外だがC replacementでは避けにくい。memory modelとownership/lifetime interactionを将来独立設計する。 |
| separate compilation / modules / linkage | STRATEGIC-GAP | one semantic compilation unitのv0から実用scaleへ進む際の重要境界。summary/identity transportが必要。 |
| debugger / profiler / sanitizer interoperability | OPEN | language adoption上重要。semantic modelとは別にtooling coverageを追う必要。 |
| stable library / package / build ecosystem | OPEN | C replacementはlanguage featureだけでは達成できない。build/link/dependency distributionもproduct risk。 |
| C source compatibility | OUTSIDE-ENVELOPE | 目標ではない。migration toolingは将来あり得るがlanguage design authorityにはしない。 |
| C undefined-behavior compatibility | OUTSIDE-ENVELOPE | 継承しないことがNewLangの目的に含まれる。 |
| every compiler extension / preprocessor trick | OUTSIDE-ENVELOPE | practical capabilityが別の安全なmechanismで満たせれば互換は不要。 |

## 4. Coverage reviewで見ること

各capabilityについて、単にfeatureの有無ではなく次を確認する。

1. **Real workload necessity** — Cを選ぶ現実の理由になっているか。
2. **Core fit** — 現在のNewLang semanticsへ自然に足せるか。
3. **No foreclosure** — 既存decisionが追加経路を閉ざしていないか。
4. **Boundary localization** — unsafe / platform-specific要素を小さなboundaryへ閉じ込められるか。
5. **Topology preservation** — C workloadのarchitectureを保ったまま扱えるか。
6. **Toolchain reality** — compiler semanticsだけでなくABI/link/debug/build等が実用可能か。
7. **Residual C** — このcapabilityが無いと、どれだけ重要なC islandが残るか。

## 5. Prioritization rule

coverage gapがあるからといって、直ちに実装priorityが高いとは限らない。

優先順位は概ね:

```text
current North Starをblockingする
  > adjacent workloadで繰り返し現れる
  > C replacementに大きなresidual islandを残す
  > addability riskが高く早期検証が必要
  > convenience / compatibility-only
```

とする。

implementation costがAI等で低下しても、semantic interaction、diagnostic burden、compatibility burden、toolchain burdenは別に評価する。

## 6. Early researchを行う条件

current sliceへfeatureを入れなくても、次の場合はearly research / destructive testを行ってよい。

- 後付け時に基礎semantic invariantを壊す恐れがある
- source-visible lifetime/effect等の大きなmechanismを後から要求しそう
- ABI / memory model / external stateと深く結合する
- Java genericsのような恒久的互換制約を早期decisionが作り得る

FFI、function pointer、external backing、in-place construction等の事前調査はこの目的に位置づける。

## 7. North Starとの分離

`NewLang_North_Star_Product_Validation.md` のPASS/FAILを通すために、このmapの全gapを閉じる必要はない。

反対にNorth StarがPASSしても、このmapのStrategic Gapが残る限りC replacement達成とは言わない。

```text
North Star = first-value validation
Coverage Map = long-horizon adoption / replacement risk
```

## 8. 更新方針

新しいDeep Research、M/F/P/R finding、North Star experiment、adjacent workload testによりstatusが変わったときに更新する。

変更理由はGit history / linked Issueを正とし、この文書内にchange logを複製しない。

大きなcapabilityを `OUTSIDE-ENVELOPE` と判定する場合は、単なるv0 deferではなくstrategic decisionなのでCoordinationで明示的に裁定する。

## 9. 関連文書

- `NewLang_Product_Value_Strategy.md` — target ladder / practical C-replacement envelope / no-foreclosure
- `NewLang_North_Star_Product_Validation.md` — current beachhead experiment
- `NewLang_Project_Development_Process.md` — track運用
- `reference/CURRENT_SPEC.md` — canonical language semantics
