# P4 — Raw Storage / Occupancy / Byte Semantics contract

基準main: `aacb53b3cc599276125e7420d7cb4a5dbae19b5c`。
`docs/reference/CURRENT_SPEC.md`が指すDraft 17.6がcanonical authority。
Draft > Backend Contract > merged evidence > P4 prompt > conversation history。
Draft/history/oracleを書き換えない。新しいlanguage designは行わない。

## 対象と境界

Draft §§3.1–3.7、4.4、5.1a/5.6、9.3、14.1–3、23.1、24.0–24.2を実装する。
source parserは変更せず、owned checked artifactを返すprogrammatic semantic
operation APIを追加する。P3 fragment checkingと同じcandidate/commit境界を使う。
Allocation、Storage、slot、byte、必要なu8/usize/addr scalar identityを保持するが、
source-visible BackingRegion、RawRange、DefinedStorage、汎用integer arithmetic、
FFI、MMIO API、relocation、M8.4 grammar、LLVM loweringは追加しない。

## Identity / authority / occupancy

BackingRegionIdはcontext-local nominal identityであり、address、binding、place、
incarnation、ptr provenanceと別物。成功allocateだけがfresh explicit regionと
その唯一のAllocation authority、full-range Storage responsibilityを作る。
Allocation/Storageはnon-Copy・non-Discardable。transferはregion identityを保存する。

Storage/slotのregion-relative rangeはvalue-owned、live rootのplacementはplace-owned。
slotはempty typed occupancyでありlive Tではない。into_slotはexact size/alignmentを
必要とし、tailを暗黙分割しない。erase_slotは同じrangeをrawへ戻す。
initialize/take/destroyが同じrange上でresponsibilityを移す。
P3のcompiler-managed abstract fixtureはregion=0として保持し、explicit raw regionと
同一視しない。legacy slotにexplicit range factsがなければerase_slotはimplementation
unsupportedと明示し、BackingRegionやStorageを捏造しない。P4 explicit regionsでは全rangeが一意のStorage/slot/live-root
responsibilityにより保存され、overlap、欠落、dead-region occupancyを検査する。

layoutはcompiler/targetが与えるsize/alignment factsであり、nominal aggregateの
field/C ABI layoutは公開しない。byteはsize=1/alignment=1、exactly 256 values。
host quantity/address bookkeepingは現行Linux x86_64 native profileのsize_t幅に限定する。
他のlayoutが未知ならprecision limit。size=0 storable typeは§23.1により拒否する。

## Raw state / scalar / access

contextがregionごとのsorted interval partitionを所有する。各intervalは
Unspecified、Defined-known octet、Defined-unknownを保持する。
unknown constantとUnspecifiedを混同しない。これはcompiler summaryでありruntime
shadow bitmapではない。Defined-unknown intervalは各byteがDefinedという近似であり、
未知byte値同士のequalityを主張しない。

raw read/write/copyはref<read,Storage>からoperation entryのcurrent Storageを取得し、
その時点のbounds/access/validityを再確認する。Storage placeのcurrent-value factや
claim/rangeをraw writeで変更しない。ordinary raw read/write propertyはBackingRegion
側で別に保持する。特殊memoryをordinary transferへpromotionしない。

readはDefinedだけからinline semantic byte scalarを返す。scalar result bookkeepingは
Storage/Allocation/ptr/refやtyped T ValuePackageを生成しない。argument evaluationによる
既存ordinary refのCopy / operation-local childはP3同様に別途記録し、call returnで終了する。byte/u8 explicit
conversionは全256値でtotal、unknown Defined scalarでもtotalである。
RawDefined inspectionはcompiler factのviewでありauthority minting APIではない。

copyはsourceのpre-operation interval sequenceをsnapshotし、Defined-known/unknownと
Unspecifiedをpointwiseにdestinationへ移す。同一claim内partial overlapもsnapshotを
使う。§24.2のzero count/end-anchorは明示的に支持する。

## Failure / precision / scope

全operationと新しいregistration pathはtransactional。semantic error、unsupported、
precision、OOM、resource、internal errorを区別し、first diagnosticのrole spanを保持する。
programmatic requestはoptional source/spansをborrowする。sourceなしのdiagnosticは
placeholder spanを用い、renderにはsourceが必要。

constant singleton writes、constant-range copies、known Defined readsが基準。
未知dynamic bounds/alignmentやpath-sensitive proofはCOMPILER-PRECISIONとして拒否する。
context tableごと4096 entries、全live region合計4096 raw intervalsの実装budgetを設け、
limit/OOMをsemantic invalidityに
変換しない。candidateのpartial mutations/fresh historiesをcommitしない。

非空allocate、splitのinterior cut、nonempty adjacent mergeを支持する。
§3.2–3.3はempty allocation / split端点のaffine responsibilityを明示していないため、
これらはCOMPILER-SPEC-AMBIGUITY（deferred corner、非空sliceのblockerではない）として
unsupportedにする。emptyを新しい仕様として合法/違法と決めない。canonical Draftの
変更が必要な矛盾を発見した場合は実装で解決せず停止する。

loan body/nonescape、function dependency solver、platform allocator runtime、physical
memory allocation failure / Result surfaceは対象外。P4 completion後はreview gateで停止し、
PR merge、P5、M8.4 frontend、relocation、LLVM workへ進まない。

## 検証

P3の27 CTestsを維持し、38 CTestsでoccupancy cycle、claim duplication/identity/range/alignment拒否、
Defined/Unspecified差、overlap copy、current Storage replacement、raw-vs-typed boundary、
deallocation/stale ptr、dynamic precision、determinism、OOM/rollbackを追加する。
GCC/Clang/ASan/UBSan、formatとfinal-head PR CIを確認する。
