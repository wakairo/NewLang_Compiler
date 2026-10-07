# NewLang Product / Value Strategy

> Status: Track V product/value strategy
>
> この文書はNewLangのproduct strategy、targeting、value validationの基本原則を定める。
> language semanticsのnormative authorityは `docs/reference/CURRENT_SPEC.md` が指すDraftであり、本書はそれを上書きしない。
> 変更履歴はGit commit historyを正とし、文書内に別のVersion historyを持たない。

## 1. Product thesis

NewLangはC後継のsystems programming languageを目指す。

> **Preserve the architecture, replace the bookkeeping.**

Cで合理的に採用されている低水準architecture / topology / explicit mechanismを維持しつつ、lifetime、ownership、current-state、dependency、provenance等のsemantic bookkeepingのうち、compilerが静的に検査できる部分をcompilerへ移す。

> **Policy-neutral, mechanism-strict.**

application / data-structure policyを過度にlanguage coreへ取り込まず、低水準mechanismの意味を厳密にする。

## 2. 多段階ターゲティング

NewLangはcurrent North Starと最終市場を同一視しない。

> **North Star scope is an experimental beachhead, not a definition of NewLang's ultimate addressable workload.**

### Stage 1 — Beachhead / North Star

最初に価値差を最も鮮明に検証する。主対象はskilled systems programmer、explicit pointer topologyを持つsingle-threadedまたはexternally synchronizedな低水準component、custom / intrusive structure、allocator/runtime/VM/GC、DB/storage内部、low-level library等。

Stage 1は最初に勝つ場所であり、NewLangの最終用途一覧ではない。

### Stage 2 — Adjacent systems workloads

Beachheadと同じsemantic foundationがOS component、embedded / firmware、storage/networking internals、game/runtime、allocator、custom container等へ自然に伸びるかを検証する。

### Stage 3 — Practical C-replacement envelope

新規にCを選ぶことが合理的な実用workloadについて、NewLangを選択不能にする大きなcapability holeを残さない状態を目指す。

ここではC ABI / FFI、function pointer / callback、external backing、mmap/shared memory/DMA、MMIO/volatile、raw storage、freestanding、atomics/concurrency、separate compilation/modules/linkage、platform ABI boundary等がcoverage問題になる。

### Stage 4 — Strategic destination

組織がNewLangを採用したとき、実用上重要なresidual C islandを十分小さくできる状態を目指す。

## 3. なぜC replacementまで目指すのか

実務上の大まかな選択を:

```text
GC language -> Rust -> C
```

と捉えたとき、NewLangがCとRustの間のごく狭い用途だけを担うなら:

```text
GC language -> Rust -> NewLang -> C
```

となり、language/toolchainが一段増える組織コストを生み得る。

NewLangの長期的な狙いは単に一層追加することではなく:

```text
GC language -> Rust -> NewLang
```

とし、Rustが自然に解けないため現在Cへ落ちている実用領域をNewLangで担えるようにすることである。Rustを置き換えることは目的ではない。

## 4. C replacementの意味

長期目標は:

> **Cが合理的に選択されている実用的なsystems-programming領域を、Cのunsafe semantic modelを引き継がずにNewLangで担えるようにする。**

英語では:

> **NewLang should eventually cover the practical capability envelope for which C is rationally chosen, without inheriting C's unsafe semantic model.**

これはC source compatibility、Cのundefined behavior互換、全compiler extension、C preprocessor programming model、C++ ABIの包括的再現を意味しない。置換対象はCのsource semanticsそのものではなく、Cが実務で担っている役割である。

## 5. Beachhead != Final target

Track Vは次を区別する。

```text
current validation scope
!= v1 product boundary
!= ultimate addressable workload
```

North Starからfeatureを外すことは、現在の価値仮説には不要という判断であって、将来もNewLangに不要という判断ではない。

逆に、将来C replacementに必要そうだからという理由だけでcurrent North Starへfeatureを追加しない。

## 6. No-foreclosure rule

Deferred capabilityについて必要に応じて次を確認する。

> **今のcore semantics / source decisionsが、将来そのC-class capabilityを自然に追加する道を不必要に閉ざしていないか。**

これを **no-foreclosure rule** とする。FFI、function pointer、external backing、in-place initialization等を今実装しなくても、後付け時に基礎意味論の破壊を要求しないかは先にpressure-testしてよい。

no-foreclosure reviewはfuture featureの実装authorizationではない。

## 7. Track V — Value / Product Validation

Track Vは **誰のどの問題を、どのevidenceで解けたと言うか** を担当する。

主な責務:

- target user / workload / workflowの定義
- value hypothesisの明文化
- North Star / representative workloadの選択
- PASS / FAIL criteriaとanti-gaming ruleのpre-registration
- human obligation、ceremony、topology preservationの比較
- intermediate product gateの設計とfalse progressの検出
- implementation後のevidenceがproduct claimを本当に支持するかの評価
- practical C-replacement envelopeのcoverage risk管理
- Deferred capabilityへのno-foreclosure確認

Track Vはcanonical language semantics、Draft revision、implementation architecture、formal proof strategy、merge / milestone sequencingを単独では決定しない。

## 8. 他trackとの問いの分離

```text
V: 誰の何が改善されれば価値があるか？
M: その価値を成立させる最小で整合したlanguage ruleは何か？
F: そのruleは期待する性質を形式化できるか？
P: production compilerで忠実にcheck / diagnose / lowerできるか？
R: preferred interpretationを外して読むと何が壊れるか？
Coordination: どのfindingを、いつ、どのtrackへ渡すか？
```

## 9. Slice選択原則

新しいfeature / sliceでは少なくとも次を見る。

- **User value**: 誰のどのreal workloadが改善するか。
- **Beachhead necessity**: current gateに本当に必要か。
- **Failure attribution**: 複数の未確定mechanismを一つのgateへ混ぜていないか。
- **Anti-false-progress**: host fixture、toy rewrite、unchecked、backend trickでuser-visible gapを隠していないか。
- **Topology preservation**: NewLangを通すために元のC architectureを本質的に別物へ変えていないか。
- **No foreclosure**: 小さな現在判断が将来のC replacementを不必要に閉ざしていないか。
- **Lifecycle cost**: implementation costだけでなくsemantic/test/diagnostic/compatibility/documentation/future-interaction costを数える。

AIによりimplementation costが下がっても、semantic complexityやcompatibility costは自動的には下がらない。

## 10. Deferredのproduct分類

- **Not needed for current gate** — 現在のvalue hypothesisに不要。将来必要性は未判定。
- **Deferred but addable** — 将来必要になり得て、現在設計から自然に追加できるevidenceがある。
- **Strategic coverage gap** — practical C-replacement envelopeに重要だが、追加可能性のevidenceが不足。
- **Outside strategic envelope** — C互換のためだけで、NewLangの実用C replacementには不要と判断したもの。

これはproduct/strategy分類であり、language validity分類ではない。

## 11. Product success

成功はfeature countではなく、次の順で積み上げる。

```text
one sharp value claim
  -> actual-source vertical evidence
  -> adjacent workload reuse
  -> capability coverage expansion
  -> residual-C reduction
```

最終的には、Cを選ばざるを得ない重要workloadがどれだけ残るか、NewLangがsemantic bookkeepingを実務上どれだけ減らすか、language/toolchain追加コストに見合うかで評価する。

## 12. 関連文書

- `NewLang_North_Star_Product_Validation.md` — current beachhead / North Starのproduct validation contract
- `NewLang_C_Replacement_Coverage_Map.md` — practical C-replacement envelopeのcoverage ledger
- `NewLang_Project_Development_Process.md` — track運用・handoff・authority
- `reference/CURRENT_SPEC.md` — canonical language semanticsを指すauthority
