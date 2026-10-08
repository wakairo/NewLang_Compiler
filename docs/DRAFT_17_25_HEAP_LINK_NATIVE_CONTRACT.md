# Draft 17.25 heap-owned link native contract

Track: P — Issue #180。
Base main: `fd6941568598bba70900ff140303299d0a3bdd5a`。
Canonical: CURRENT_SPEC → Draft 17.25 §§10.1 / 17.1と既存§3.2 lifecycle。
Issue #178 / PR #179の[独立Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/179#issuecomment-6053585849)で成立したowned checked evidenceを使う。
Process §4.2、Design Decision Procedure、Ledger DI-001/002/006/007/009/010を照合済み。
Historical audit: **N/A — faithful implementation**。semantic delta **0**。

## Exact executable boundary

unchanged `tests/fixtures/heap_link_semantic.nl`のactual-source→checker→owned checked
None/Some arms→Checked-C/C17→native経路を成立させる。
1 allocated Hが自身のlinkを所有し、distinct lexical H tailへ接続する。
completed nominalとcommitted linkをsemantic identityで選択し、宣言名を特別扱いしない。
H size 24 / alignment 8は既存Linux x86_64 compiler-private target plan。
generated `_Static_assert`でH/Option size、alignment、member offsets/typesを検証するが、
NewLang ABI/layout guaranteeにはしない。

## Checked evidence → carrier

- root取得は`NL_CHECKED_REF_FROM_PTR`のselected ptr/stability identifiersをlowerする。
  identifierのCopy factsをselected bindingと照合し、current initialized root incarnation / D、
  scope、provenance、requested read/write mode、pointer write permissionとbacking ordinary-write /
  range / alignmentを保持する。read stabilityからwriteをmintしない。
- `NL_CHECKED_FIELD_REF`はそのselected root-ref operandをlowerしてからprivate `f0`へ写す。
  opaque key (completed nominal + declaration index) / parent-child incarnation / historical child-parent
  relation / D / range / scope / access / dependency evidenceを照合する。
  field baseを「最後のmalloc結果」やsource文字列から選ばない。
- Copy readはnative current Optionを別carrierへコピーする。
  checked Copy packageとptrのprovenance/current incarnationを照合し、Some armが実tagで選ばれた
  pointerからexisting local reloanを行う。
- 2つのreplaceは実heap fieldへ書き、native old Optionを返す。
  checked pre/post ValueFacts、old/new packageとSome occurrence Resetの順を照合する。
  root/link incarnationとdisjoint siblingを変更しない。
- scoped refsはgenerated C lexical loan scope内に置く。すべてのfield access後に
  same-D EndRoot→slot/raw→domain finalize→matching Allocation handleのfreeを1回実行する。
  real NULL armではsuccess-only claims、field操作、EndRoot、freeを実行しない。

backendの一時stateはこの有限checked traceのcarrier/fact correspondenceだけであり、
新しいsemantic checkerやruntime authority solverではない。
final contextではroot/child/scopeが終了済みでよい。point-in-time checked証拠と
immutable historical relationshipsを使い、final-state livenessを実行順に逆輸入しない。
branch-local IDsはowned artifactとprefix certificate内でのみ解決する。

## Observation / failures

test-only observerとplatform shimを分離する。
shimのsuccessはgenerated malloc/freeをそのままdelegateし、failure injectionはNULLだけ。
observerは実tag/address/payload/operation orderを読み、Node/root/linkを作成・修復・解放しない。
NDEBUGでも明示VERIFYが有効。EndRoot後は保存済みinteger addressだけを記録する。
observer無しのgenerated Cもstrict C17でcompile/runする。

不適切なchecked key、site、scope/D、access、selected operand、Copy/Change/Reset証拠は
unsupportedとし、staged Cをpublishしない。OOM/semantic failureでもcaller output slotsを
変更しない。metadata不足なら勝手にartifactをrepairせずCoordinationへBLOCKする。
既存static negatives、supporting claimsとregistration/body fault sweepsを弱めない。
source-validなricher routeはbackend unsupportedとし、language-invalidへ分類し直さない。

## Non-goals / handoff

一般ref/member/ptr projection、複数heap roots、detach、traversal、一般allocator、
recursive delete、FFI、LLVM、cJSON、North Star PASS、新しいtrackを含まない。
borrowed-match precisionやscope-escape診断の既存制限も変更しない。
candidate PRはexact-head 5-job CI後もOPEN / unmerged、IssueもOPENとし、
独立Coordination reviewへ渡す。
`DRAFT 17.25 HEAP-OWNED LINK NATIVE GATE READY FOR REVIEW`で停止する。
