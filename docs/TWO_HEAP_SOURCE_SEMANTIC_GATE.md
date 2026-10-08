# Draft17.26 two-heap actual-source semantic gate

Track: P / [Issue #185](https://github.com/wakairo/NewLang_Compiler/issues/185)

開始時main `189aa59e939cf8a8bb77a7a5f42b6e73086bca66`、CURRENT_SPEC → **Draft17.26**。
Process §4.2、Design Decision Procedure、DI-009/010/007/006/001/002を確認。
Historical audit: **N/A — canonical §3.2/§10.1/§17.1/§26/§27への忠実な実装・検証**。
source/core semantic delta **0**。Canonical Draftを変更しない。

## 前提と今回の差分

旧findingはouter non-Copy責任が変化するnested allocation joinを表現できないことだった。
[Issue #186 / merged PR #187](https://github.com/wakairo/NewLang_Compiler/pull/187)
の[独立ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/187#issuecomment-6055133754)
により解決済み。ancestorだけからclosed comparison prefixを構成し、両armで一致を証明して
callerへ反映する処理、`normal_frame_unchanged=false`、owned immutable post snapshotをそのまま再利用する。
DI-009のsource ADOPTED/BOUNDED同期も#187で完了している。

今回のprimary fixture `tests/fixtures/two_heap_semantic.nl` はDraft17.26 §3.2の**全文verbatim**。
parser → registered `main` → checker → owned checked artifactまで通す。
元source/ASTを破棄してからevidenceを読む。host seed/claimやC runtimeによる安全性判断は使わない。

必要なproduction変更はprofileのcoverage fenceだけ。
loop内trialや、有限body walkで複数のouter trialを繰り返すsourceが従来checkerを通っていたため、
one outer + optional one nested Some trial、same completed H、loop外に制限する。
別targetも`ALLOCATED-TARGET-PROFILE`というunsupported diagnosticにする。
これらはcore-invalid/UBとは主張しない。branch/loop solver・field/Option semanticsやbackendは変更しない。

## 三経路の責任とidentity

| Owned path | Grants / EndRoot / deallocate | Authority / common caller |
|---|---|---|
| outer None | 0 / 0 / 0 | R/D/rootなし、inner trialなし |
| outer Some → inner None | head 1 / 1 / 1 | tail grantなし、headのA/D/root/Rを明示終了 |
| outer Some → inner Some | head+tail 2 / 2 / 2 | 別R_h/R_t、D_h/D_t、root/incarnation。tail→head順で独立終了 |

SomeのOneBacking、Allocation、Storage、slot、LifetimeDomainは非Copy・non-Discardable責任。
各applicable normal pathで消費し、Noneにprincipal ownerを作らない。
inner両armのhead終了結果は#186のprefix証明でcallerへ反映する。
head ptr tokenは残れてもsafe reloanは不可。tailのarm-local claimをcallerへ持ち出さない。

IDは**owned fragment path + numeric ID**で読む。
parent prefixだけがancestor由来として同一視できる。
inner None/Someで一致するsuffix数値は別identityであり、common stateへimportしない。
同一H typeとroot/backing/domain identityを分離する。
Option armのowned snapshotでは両region/domain/rootが同時にliveであることも検査する。
fresh distinct BackingRegionとoccupancy validationがabstract backingのnonalias evidenceであり、
numeric machine address equalityからauthorityを推論しない。

## Link / Copy / safe reloan / Change・Reset

- `ref_from_ptr(write,ptr_h,stable_h)`の**selected source operands**とprovenance/current incarnation、
  D_h、scope、write permissionをchecked nodeで検査。
- head自身のcommitted fixed childへのmode-preserving `@next` projectionを検査。
  ProjectionId相当のnominal/index、parent/child incarnation、opaque site、selected root bindingを維持する。
- None→Some(ptr_t)で新しいconditional occurrence、Some→NoneでReset。
  root/child incarnationは同じまま、ValueFactはChangeで変化し、payload sibling packageは保存される。
- `read(head_r@next)`はOptionとpayload ptrの**deep Copy**。
  original/copyのpackage IDは異なり、ptr_tのroot/incarnation/provenance/dependencyは保存される。
- `Some(q)`でのread reloanはselected qとstable_tを使用し、D_tに結び付く。
  D_hへの差替えは拒否。ptr link/CopyはtailのAllocation/domain責任を移動しない。
- すべてscopeが終了してからtail/headの独立EndRoot・erase_slot・finalize・full matching deallocate。

**観測の限界:** これはsafe tail scoped ref acquisitionのsource/checked evidence。
一般`read(ref<H>)`やlanguage-level tail payload load、実heap pointer identity/native topologyの証明ではない。
新しい2-Node Checked-C/native/observerは実装しない。

## Tests / failure boundary

- `two_heap_fields.unit`: AST teardown後のfull field/Copy/selected q+stability/Change/Reset/lifecycle evidence、
  read-only→write禁止とtype/access、live owned Option snapshots、source semantic-error rollback/retry。
- `two_heap_lifecycle.unit`: #186の検査器をfull sourceへ適用、三経路のledger、captured closureと
  branch-local numeric collision、corrupted prefix/result/certificateのfail-closedを維持。
- `two_heap_source.integration`:明示的input-based source controls。
  rename/scalars/binding aliases/reversed arms/continuationのpositiveでもowned evidenceを再検査。
  wrong D/region、責任漏れ・重複・再使用、stale q、active root/field ref、scope escape、
  missing Option arm、pattern、3rd/repeated/loop trial、unsupported target/member/readを拒否。
  syntax unsupported、semantic invalid、precision、backend unsupportedをcode/statusで区別する。
  借用Option matchのChangeは既存`P9-MATCH-PRECISION`で保守的に拒否し、成功にはしない。
- `two_heap_failures.unit`: existing malloc/realloc sweepにfull sourceを追加。
  registration、second trial、nested clone、Option arm/interpretation、checked creation、
  prefix/continuation snapshot allocationを順に失敗させ、caller不変・NULL output・clean retryを検査。
- **SUPPORTINGのみ:** actual owned live snapshotのcloneにunknown provenance、write permission loss、
  duplicated occupancy、Value(link)/Value(sibling) blocker、Some conditional refのReset conflictを加えて検査。
  これらはsource positive authorityではない。

正しいfull sourceのCLI結果も **exit 4 / V1-BACKEND-UNSUPPORTED / empty stdout**。
C/object/executableは生成しない。失敗結果からoverlapを推測するfallbackは無い。

## Findings / validation / stop

`COMPILER-IMPLEMENTATION` coverage finding: loop/repeated outer trialのprofile fence不足を局所修正。
`COMPILER-DIAGNOSTIC`: target unsupportedのgeneric internal風codeを明示profile diagnosticへ修正。
`COMPILER-PRECISION`: richer borrowed Option/conditional Changeなどは既存precision fenceを維持。
new semantic blocker、spec hole/ambiguityは発見していない。

baseline201 + 新4 = **205 CTests**。
pinned bootstrap LLVM/Clang/formatter23.1.2、local GCC14.2.0。
GCC Debug / GCC Release(NDEBUG) / Clang+format / ASan / UBSanでfull CTestを実行。
exact-head PR-triggered五構成のSHA/run URLはIssue/PR handoffに記録する。
既存one-H native、oracle.adapter/smoke、artifacts.integrityを維持する。

OPEN/unmerged候補PRを独立Coordination reviewへ引き渡し、
**DRAFT 17.26 TWO-HEAP SOURCE-SEMANTIC GATE READY FOR REVIEW**で停止する。
3つ以上のNode、native two-H、detach/traversal、general allocator、FFI/LLVM/cJSONや後続trackに進まない。
