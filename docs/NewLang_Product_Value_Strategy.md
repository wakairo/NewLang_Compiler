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

## 1.1 UB-prevention audit locality (value hypothesis)

**製品価値仮説**は、C-classのoriginal pointer topology / allocation・lifetime policy /
physical architectureを維持しながら、**未定義動作（UB）を防ぐために人間が
監査・再監査しなければならない意味論的範囲を小さくすること**である。
compilerが現実的に検査できるlifetime、currentness、provenance、access、
responsibility等の条件はchecked側で拒否・保証し、証明不能でも実際に真で
ある必要がある事前条件だけを明示的・局所的な `unchecked` に残す。
`unchecked` の前提違反はUBであり、明示した人間／library authorが証明責任を持つ。

**Audit locus（監査の実際の対象）**は `unchecked` の行数ではない。
各privileged operationと**実際のauthority issuer**、元の
Storage / BackingRegion / Allocation / LifetimeDomain、
前提となる封じ込め済み不変条件、それに影響するlegal mutationと
normal / error / teardown / reentrant callback / FFI経路、
および変更時にそれらの成立性を再検討すべき**影響依存関係**を含む。
raw addressやmetadata（例：Ringの `head/len`）は責任・権限を無から
生成できない。例えば `unchecked buf[i]` では、元の `buf` の領域・
寿命・アクセス権限と `i` の範囲だけでなく、それらを生成・更新する
writer、呼出し、状態遷移まで監査対象が広がり得る。これは
`unchecked` の行以外を確認する必要があるというだけで、局所化の失敗ではない。
**実際の前提からの依存関係を特定し、その閉包だけを監査・再監査すれば
よく、無関係なsafeコードの再監査を避けられる**なら、Cより監査範囲を
絞れた価値がある。逆に公開ordinary-safe APIの合法な操作が
前提を破壊できるなら、安全な抽象化は成立していない。実装内部の
safe helperやwriterを変更するときには、その変更が影響する
不変条件の依存閉包を再監査し、compilerが保護できる部分は委ねる。
「uncheckedの行だけ見ればよい」は、この依存閉包の確認を
省略してよいという意味ではない。

**ordinary-safe library/APIの成立条件**は、APIから合法的に到達できる
**すべてのsafe-reachable state**とその状態遷移に対し、内部uncheckedの
前提が維持されることである。通常処理だけでなく、失敗時の責任の返却／
rollback／commit、途中状態での公開、破棄、借用中の変更、callbackによる
再入、FFI境界を含む。これはstaticに強制するか、健全なruntime guardと
アクセス制御を伴う内部不変条件として成立させる必要がある。
FULL / EMPTY等の予期される操作結果は `Option` / `Result` 等へtotalize／
明示的に返し、UBの「前提」に押し込まない。representation hidingや
safe mutationの制限を実際に強制できないなら、監査の局所化はまだ成立しない。

**責務の分離**：library authorは、原権限を保ったclaim transfer、
その不変条件とすべての合法な変更・失敗経路、内部uncheckedの正当性を負う。
ordinary callerは公開された型付きsafe APIの使用とapplication policyを
負うが、隠れたpointer hopごとのlifetime／owner証明を要求されない。
明示的な `unchecked`／外部契約をcaller自身が選ぶ場合は、その境界の
人間による証明責任が残る。

**公平な比較基準**は、同等の元のtopologyと実際のAPIを揃えた
*library author + ordinary caller* の合計である。

- **十分にAPI化したC**：callerからraw rewiringと局所証明を隠せる。
  static analysisやruntime checksも使えるが、通常は内部のlifetime /
  provenance / mutationに関するUB監査・再監査をlibrary authorが持つ。
- **Zig**：明示的なlow-level control、unsafe境界とbuild modeに応じた
  runtime safety checksが使える。比較は「unsafe行数」ではなく、
  覆われないalias/lifetime/owner前提を誰が保守するかで行う。
- **同等topologyのRust**：localized `unsafe` + safe wrapper、
  privacy、型／借用制約によってordinary callerを守れる。
  既にaudit localityを実現する有力な比較対象であり、NewLangの優位は
  前提としない。同じ構造を維持するためのauthor側証明、safe API設計、
  保守変更の再監査半径、ceremonyを実測して初めて差を判定する。

**成功の観測対象**は、人間が維持するL/R/P/V/C等のsemantic obligation、
author/callerの分担、unchecked前提をsafe mutationで破壊できるかという
negative evidence、変更時の相対的な再監査範囲・費用、および
topology維持である（分類は[North Star §8](NewLang_North_Star_Product_Validation.md#8-human-obligation-ledger)を再利用する）。
zero unchecked lines / zero bugsは目標指標ではない。

**保証と現状の線引き**：健全なcompiler・backend・runtime・FFI・platform
契約と、実際に正しい局所unchecked前提が成立した*将来の条件付き到達点*
として、公開ordinary-safe APIを合法的に使うcallerや監査済み依存閉包の
外側のsafeな変更が、内部uncheckedの前提を破壊しないことを目指す。
依存閉包の内側を変更する際には、真の前提の維持について
人間の再監査が残り得るが、compilerが機械的に保証する部分は
再監査対象から外せることを目指す。
logic / application policyの欠陥、C互換UB、未カバーのplatform effect、
v0範囲外のconcurrency、誤証明されたunchecked前提をなくす約束ではない。
現行Productionにgeneral source `unchecked` / `requires fn`、
module-level representation hidingの強制、general Ring safe APIが
実装済みだとは主張しない。現在の有限source → checked-owned →
C17/nativeのPASSやCHECK0／CI greenは、soundness theoremでもProduct GOでもない。

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
- human obligation、ceremony、topology preservationの比較（§1.1のauthor/caller別監査・再監査範囲とsafe mutationによる前提破壊リスクを含む）
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
- **Anti-false-progress**: host fixture、toy rewrite、unchecked、backend trickでuser-visible gapを隠していないか。safe mutationによるunchecked前提破壊の反例・未検証範囲を隠していないか。
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

最終的には、Cを選ばざるを得ない重要workloadがどれだけ残るか、NewLangがsemantic bookkeepingとauthor/caller双方のUB予防監査・再監査範囲を実務上どれだけ減らすか（safe mutationの負の証拠と相対的な再監査半径を含む）、language/toolchain追加コストに見合うかで評価する。

## 12. 関連文書

- `NewLang_North_Star_Product_Validation.md` — current beachhead / North Starのproduct validation contract
- `NewLang_C_Replacement_Coverage_Map.md` — practical C-replacement envelopeのcoverage ledger
- `NewLang_Project_Development_Process.md` — track運用・handoff・authority
- `reference/CURRENT_SPEC.md` — canonical language semanticsを指すauthority
