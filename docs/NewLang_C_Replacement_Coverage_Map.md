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

statusはlanguage validityを意味しない。以下の行の `SPEC`／`SOURCE/CHECK`／`OWNED`／`C17/native` は**有限の技術的証拠段階**を示し、`BEACHHEAD-VERIFIED`（bounded actual-source **product** evidence）や製品成功への自動昇格ではない。一般価値仮説は[Product / Value Strategy §1.1](NewLang_Product_Value_Strategy.md#11-ub-prevention-audit-locality-value-hypothesis)を参照。

## 3. Coverage ledger

| Capability area | Current strategic status | C replacement上の意味 / 現在の見方 |
| --- | --- | --- |
| scalar / lexical value flow / function body | BEACHHEAD-VERIFIED | bounded actual-source executable scalar spineとCopy `u8→u8` direct/nested callはSOURCE/CHECK→OWNED→C17/nativeの有限実証。**名前付きNode書込関数のsum/ref署名・二正常分岐のpost-stateはmain未対応**（[P475](https://github.com/wakairo/NewLang_Compiler/issues/475)はCHECK/P8停止、未統合[P478/PR479](https://github.com/wakairo/NewLang_Compiler/pull/479)でもCHECK/P9停止）。一般numeric familyやeffect/ABIは別。 |
| nominal aggregate value | BEACHHEAD-VERIFIED | bounded aggregate construction / whole destructuringをactual sourceで検証。layout保証とは分離。 |
| persistent ptr + scoped ref | BEACHHEAD-VERIFIED | local-root gateでscoped accessを実証。さらに[PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)では**元lexical Node**のread-loan由来ptrを別の初期`Some`へ保持し、SOURCE/CHECK→OWNED→C17/nativeの元C addressを有限確認。heap locator／一般安全access APIとは別。 |
| stale ptr after EndRoot | BEACHHEAD-VERIFIED | ptr token自体は残り得るがsafe reacquisitionはrejectする境界を検証。 |
| lifetime-preserving mutation / replace | BEACHHEAD-VERIFIED | stable-root `u8` に加え、[PR #438](https://github.com/wakairo/NewLang_Compiler/pull/438)・[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)の元lexical `Node@next` でscoped write/old Copy・field/root currentness・sibling維持をSOURCE/CHECK→OWNED→C17/nativeで有限実証。一般CFG/heap ownerは未検証。 |
| ordinary write loan source mapping | BEACHHEAD-VERIFIED | 当初のdirect lexical `u8` profileに加え、`loan_write(node@next){|w|replace(...)}` の**元lexical Node固定field**が[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)でsource-issued公開owned/C17/native。名前付きcalleeへこのrefを渡す二分岐合流は未実証。 |
| field projection | SEMANTIC-BASIS | `local@field` 一般則はSPEC。実sourceでは固定`Node@next:Option<ptr<Node>>`をCOPY readし、scoped write投影と併せて[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)のSOURCE/CHECK→OWNED→C17/nativeを有限実測。任意のfield/aggregateのproduct coverageは未検証。 |
| field mutation | SEMANTIC-BASIS | **旧「actual-source未検証」は解消（有限）**。元lexical `Node@next` のwrite-loan+`replace`、旧`Option` Copy、`Change/Reset`と二正常分岐の公開証明、元物理NodeのC17/nativeを[PR #438](https://github.com/wakairo/NewLang_Compiler/pull/438)・[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)で確認。**SEMANTIC-BASISのまま**：一般field・named-call効果・heap mutation・普通caller製品証拠はない。 |
| recursive nominal / intrusive graph shape | STRATEGIC-GAP | SPECの再帰nominal宣言だけでなく、元lexical `Node` の`None/Some(ptr)`初期化・元pointer topology・matched`Some(q)`別root書込をSOURCE/CHECK→OWNED→C17/nativeで有限実測（[PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)）。**STRATEGIC-GAPを維持**：real heap graph／非Copy custody／一般intrusive ownership・cJSON detach/adoptionは未達。 |
| sum / optional persistent link | SEMANTIC-BASIS | `Option<ptr<Node>>::None/Some`を元lexical `Node@next`に実際に構築し、Copy read、match `Some(q)` の元ptr保持、二正常分岐で別rootに作用する有限SOURCE/CHECK→公開OWNED/validator→strict C17/nativeを[PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)・[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)で実測。**SEMANTIC-BASIS**維持：一般graph/unsafe-free ordinary callerへ未一般化。 |
| root lifetime start/end | SEMANTIC-BASIS | root/incarnationモデルとsource検査に加え、元lexical Nodeのlive original carrier／後続scoped accessをC17/nativeで有限観測。別の5-root heap pre-detach/0–5 failure worldの実行証拠はあるが、元B＋dstの非Copy custody、end/releaseとcJSON H1全体の製品到達は未証明。 |
| Allocation / Storage / slot | SEMANTIC-BASIS | raw/typed occupancyのSPEC＋Productionテスト、固定5-root **pre-detach** ／0–5失敗世界の限定native証拠あり。Node `Some(q)` / write-joinはlexical rootsでありAllocationやLifetimeDomainを生成・検証する証拠ではない。一般user-facing source alloc/teardown・owner custodyは未実装。 |
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
