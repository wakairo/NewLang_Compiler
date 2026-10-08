# Draft 17.25 heap-owned link source-semantic contract

Track: P — Issue #178。**pre-backend限定**。

## Authority / scope

開始時main: `060731a121cb29ba6cfe2559d135a4736c3dc429`。
`CURRENT_SPEC.md` → Draft 17.25 §§10.1 / 17.1。
source裁定はIssue #176 / merged PR #177と独立Coordination ACCEPT。
Process §4.2 / Design Decision Procedure / Ledger DI-001/002/006/007/009/010を照合した。
Historical auditは **N/A — faithful implementation**、semantic deltaは **0**。
DI-010の採用statusだけを同期し、canonical Draftを変更しない。

対象は、既存fallible `try_allocate_one<H>()`のowned None/Some world内で、
1つのallocated Hの自身のlinkを1つのlexical H tailへ接続・読出し・unlinkし、
既存typed lifetime / affine backingを回収するactual-source semantic path。
completed H / link / payloadの宣言名は固定しない。
新しいheap field accessのChecked-C/native loweringは対象外。

## 2つのauthority gate

1. `ref_from_ptr(write,p,stable)`:
   parserはrequested modeと2つのidentifier operandを保持する。
   checkerは既存ptr/stability検査に加え、ptrのvalid provenance・read/write access、
   current independent H root incarnation、live governing D / occupied backing、
   Hのfull typed range / alignmentとbacking ordinary-write propertyを確認する。
   `stable`は同一Dのlive ordinary read refでなければならない。
   dependency不足・stale状態・conflictを成功としない。
   read-only D stabilityからwrite permissionを生成しない。
   read-mode acquisitionは引き続きread-only。
2. ordinary root refからの`r@committed_link`:
   baseのstatic ref kind / completed Hをlabelより先に選択する。
   allocated sliceのcurrent allocated rootに限定し、payload・unknown field・ptr base・
   exclusive ref・nested/general memberへfallbackしない。
   resultはexact ordinary `ref<same mode,Option<ptr<H>>>`。
   selected parent refのscope/provenance/access/dependency evidenceをそのまま継承し、
   referentだけをcurrent fixed childへ導出する。新しい独立root・backing・lifetimeは作らない。

static opaque projection keyは既存`NLCheckedField`のcompleted nominal identity +
semantic declaration index。current child PlaceId/incarnationはそのinstanceのsiteであり、
C offsetやnumeric addressとは別物。checked artifactはparent/childのincarnation、
ValueFact、access、current Option package、conditional occurrenceを保持する。
childのgoverning Dとparentから継承したscopeはroot stabilityを継続して制約する。
ordinary mutable refへ根拠なく`Value(current contents)` dependencyを追加しない。
元refが持つexact dependencies / unknown evidenceを消去しない。

## Copy read / Change / Reset

`read(r@link)`だけをbounded builtinとし、general `read(ref<T>)`を導入しない。
write→read weakeningは既存selected parameter compatibilityを使う。
current Optionの既存deep Copyはpayload ptrのprovenance、incarnation、hidden dependenciesを保存する。
temporary projected refと返るCopy value packageは別物。
Copy package自身にescaping dependencyがなければloan normal-resultでforwardできるが、
root/field refのscope escapeは拒否する。

`replace(w@link,v)`は既存write-ref primitive / fixed-field Changeへ接続する。
linkとancestor ValueFactを更新し、root/link incarnation・disjoint payload siblingを保持する。
old Option packageは返り値となり、Some Resetはold occurrenceを終了して新しいoccurrenceを作る。
surviving Value(link/root)・conditional Occurrence blockerは既存validatorsで拒否する。
ordinary write refに新しいunique/exclusive/noalias規則を与えない。

## Ownership / failure boundary

None/Someは独立owned checked fragmentsで検査し、SomeのBackingRegion/D/Place IDを
callerのcommon stateへ持ち出さない。Noneはallocation authorityをmintしない。
root/field refsのscope終了後に既存exclusive endingでdestroyし、slot→Storage、
finalize_domain、matching full-range deallocationを完遂する。
persistent ptr tokenやold Copy OptionはEndRoot authorityを持たない。

既存context clone / commit boundaryを維持する。
registration/body/evidence構築のsemantic error、malloc/realloc OOMはcaller stateとoutputを
変更せず、clean retryできることを検証する。
backendは新しいchecked kindsをunsupportedとし、partial Cをpublishしない。

## Bounded limitations / stop

- projection frontendはこのallocated-H gate内だけ。general ref projectionやlexical H root-ref
  projectionの追加実装は行わず、profile外はstructured unsupported。
- function body内のborrowed matchは既存`P9-MATCH-PRECISION`でrejectする。
  conditional payload blockerは別の**SUPPORTING programmatic** testで直接検証する。
- scoped root/field refのloan-result escapeは既存loan-exit validationでrejectするが、
  現診断は`P3-INTERNAL`。このgateでdiagnostic frameworkを再設計しない。
- non-writable backing/ptr、unknown provenance、Value/Occurrence blockerの内部claim controlsは
  sourceにないauthorityをseedしたactual-source証拠とは扱わない。

既存reverse-orientation lexical-head / heap-tail native executionは保持する。
heap-owned field native実装、一般allocator、複数heap roots、detach、recursive delete、
cJSON、FFI、LLVM、仕様変更は実施しない。
候補PRはOPEN / unmergedで独立Coordination reviewへ引き渡し、
`DRAFT 17.25 HEAP-OWNED LINK SOURCE-SEMANTIC GATE READY FOR REVIEW`で停止する。
