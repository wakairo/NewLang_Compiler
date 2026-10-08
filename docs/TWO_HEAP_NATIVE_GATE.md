# Draft17.26 two-heap native gate

Track: P / [Issue #189](https://github.com/wakairo/NewLang_Compiler/issues/189)

## Authority / scope

Base main: `96f2961b9d27d09b914363cb125af918939b6773`、CURRENT_SPEC → **Draft17.26**。
Development Process §4.2、Design Decision Procedure、Ledger DI-009/010/007、Testing Strategyを確認。
Historical audit **N/A — canonical §§3.2/10.1/17.1/26/27のfaithful lowering**。
semantic delta **0**。source/parser/checker/checked evidence contractは変更しない。

[merged PR #188の独立ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/188#issuecomment-6057036324)
で成立したowned artifactと、#187のancestor-derived captured-prefix closureを再利用する。
primary sourceは既存 `tests/fixtures/two_heap_semantic.nl` の全文verbatimを使用する。
[pre-backend gate report](TWO_HEAP_SOURCE_SEMANTIC_GATE.md) はそのgate時点の証拠。
本gateで初めて同sourceのtwo-H Checked-C/nativeを検証する。

## Backend boundary

actual source → production registration/checker → separately owned fragments → Checked-C/C17 → native。
元ASTを破棄したartifactをprobe/unit testへ渡す。emitterはsource spellingを解析しない。

- 各match armを独立にemitする。numeric IDはartifactとchecked prefixで修飾し、
  ancestor C carrierだけを継承する。sibling suffixを共通carrierへimportしない。
- first mallocはouter match、second mallocはouter Someの内部だけに存在する。
  captured-postはprefix/carrierの検証証拠であり、hidden cleanup命令には変換しない。
- Backend-onlyの二個の有限lifecycle recordを各arm入口へコピーし、各armで
  grant→slot→domain→initialize→EndRoot→raw→finalize→releaseを独立に検査する。
  nested両armがclosedになったことを確認して、共通ancestor carrierだけをprojectする。
  これはsemantic stateの新しいjoin/solverではない。
- Allocation/domain carrierはframe+symbolを追跡し、non-Copy transferで旧carrierを無効化する。
  同じValueIdを持つ消費済みaliasをdeallocation operandへ差し替えても拒否する。
- selected ptr/stabilityのCopy、domain source、root/incarnation/range/provenance/read/write、
  mode-preserving committed field identity、ValueFact/Occurrence Change・Resetを維持する。
  tail reloanはcopied current linkから選ばれたqとD_tを使う。
- heap-headの**実際のf0**をwrite/readする。Copyは現fieldのtag/ptrを物理的に読む。
  old None/Some packagesを返し、unlink後にtail→headの順で明示的にEndRoot/freeする。
- 24/8のH、Option 16/8、opaque field implementationのprivate C layout/static assertionsは
  既存planを再利用する。C ABIやsource-visible offset/permissionを制定しない。
- absent/mismatched evidenceはstaged outputを破棄してunsupported。OOM/resourceも
  callerのNULL outputとlengthを保存する。部分Cやnative executableはpublishしない。

## Independent runtime observation

`two_heap_checked_probe.c` はemitterを呼ばず、owned artifactのinitialize/scalar、selected operands、
root/domain/scope、field identityを抽出する。integrationはそれとgenerated Cのoperand flowを比較する。

`two_heap_observer.h` は別のP-owned read-only oracle。generated Cとは別ファイルで管理し、
heapの初期化・link変更・repair・freeを行わない。自身の有限bookkeepingだけを更新する。
H bytesはlive中だけ読む。EndRoot後のraw/release/free/finishは記録済みinteger identity/orderだけを読む。
NDEBUGでもVERIFYが有効。general tail payload read sourceは追加せず、observerがlive H_tの
payload/linkをread-onlyに読むことはruntime observationとして明示的に区別する。

`two_heap_platform.c` は指定first/second callへのNULL注入だけを行い、その他はreal malloc/freeへ
そのまま委譲する。allocation size/回数を記録するが、principal authorityやNodeを作らない。
observer自体にallocation pathは無い。shimのreal allocation failureはNoneを返し、cleanupしない。

| 同じ生成programのnative outcome | Observer evidence |
|---|---|
| first NULL | second callなし、no initialization/link、0 frees |
| second NULL | headのみinitialize、source cleanup、1 matching free |
| both success | 異なる二個のreal malloc regionが同時live、head.link None→Some(actual tail)→None、Option Copy/Some(q)/D_t ref、tail/headの2 matching frees |

観測はexit 0 aloneではなく、actual malloc identities、head自身のfield address/tag/ptr、
selected q、checked D_h/D_t/root/incarnation/scope、old-values、両payload保存、EndRoot/free順をassertする。
observerなしの同生成CもNDEBUGで同じ三経路を実行する。

## Destructive / failure evidence

- 7 actual-source native variants: verbatim、renamed H/link/payload、changed u8 values、
  selected head ptr/stability+root alias、owner transfer alias、selected tail ptr/stability alias、inner arm order。
  各variantはobserverあり/なし × success/first NULL/second NULL、計42 positive native runs。
- 17 generated-C mutations: wrong head/tail ptr、lexical proxy、wrong field、forced tag/copied ptr、
  missed write/unlink、cross free/swapped release/double free/free early、second-NULL head leak、
  hidden cleanup、wrong domain/range identity、wrong q reloan。
  全てhost Cとしてcompileしてobserverで検出する。単なるcompiler errorを検出証拠にしない。
  arbitrary third regionやtraversal/general allocatorのcorruption coverageとは主張しない。
- `two_heap_backend.unit`: 既存heap-link 32 metadata attacks + selected operand substitution、
  malloc OOM/clean retryをfull two-H artifactへ適用。
- `two_heap_backend_lifecycle.unit`: 44 nested certificate/initialize/domain/EndRoot/cleanup/carrier attacks。
  消費済みsame-ValueId owner aliasを三cleanup siteで拒否する。OOMとbounded resource limitも
  NULL output/unchanged length、restored-artifact clean retryを検査する。
- 41 explicit input-classified source controls、owned three-path ledgers、branch numeric-origin、
  field/Copy/dependency/Change・Reset、registration/checking malloc/realloc sweepを維持する。
  positivesだけbackend expectationをunsupportedからsuccessへ更新する。negativeを弱めない。
- 新gate以外のvalid constructsはexplicit backend unsupported。link-free prerequisite witnessも
  unsupportedを維持する。oracle adapter分類、workflow、frozen cJSON contractは変更しない。

## Validation / disposition

205 baseline + 3 new CTests = **208**。
checksum-locked bootstrap (LLVM/Clang/formatter23.1.2)、format、GCC Debug、GCC Release/NDEBUG、
Clang、ASan、UBSanでfull CTest。native integrationは各sanitizer設定をgenerated Cへも適用する。
exact-head SHA / PR-triggered five-job run URL・statusはIssue/PR handoffを正とする。

`COMPILER-IMPLEMENTATION`: existing one-H backend fenceをaccepted two-H evidenceへ限定的に拡張。
新たなsemantic blocker / spec hole / ambiguityは発見していない。
過去gateのprecision fences・source profile制限を維持する。
既存one-H native、lexical topology、raw/source/semantic、oracle.adapter/smoke、artifacts.integrityを保持する。

候補PRはOPEN/unmergedで独立Coordination reviewへ渡し、
**DRAFT 17.26 TWO-HEAP NATIVE GATE READY FOR REVIEW**で停止する。
third Node、detach/traversal、general allocator、FFI/LLVM/cJSON、後続Trackには進まない。
これはcJSON/North Star PASSや一般owner policyの証明ではない。
