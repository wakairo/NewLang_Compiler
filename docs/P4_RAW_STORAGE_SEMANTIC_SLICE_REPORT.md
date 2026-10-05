# P4 — Raw Storage / Occupancy / Byte Semantics report

## Authority / scope

開始時にremote mainを確認し、`aacb53b3cc599276125e7420d7cb4a5dbae19b5c`から
`p4-raw-storage-semantic-slice`を作成した。canonicalは
`docs/reference/CURRENT_SPEC.md` → `NewLang_v0_spec_Draft17_6.md`。
Draft > Backend Contract v0.4 > merged M/P/F evidence > task prompt > history。
reference snapshots、CURRENT_SPEC、Python oracle、Draftを変更していない。

M8.3/M8.3RをP3のproduction checkerへ接続するsemantic pressure testであり、
parser/lexer、M8.4 receiving、aggregate/sum/match、MIR、relocation、LLVM lowering、
MMIO/volatile/FFI、runtime allocator/shadow bitmap、P5は実装していない。
M7 backendのnoalias/capture/lifetime/opaque-layout/conservative-fallback制約を維持する。

## 実装 / conservation

Cのpublic `raw_storage.h`にprogrammatic semantic entryを追加し、
既存owned checked artifactへresolved operands、constant selections、別々のresult
responsibilitiesまたはinline scalar resultを保存する。source/requestsはborrow、
artifactはcaller-owned、region/interval summariesはcontext-owned。
source grammarやmulti-result source receivingは追加していない。

| Semantic identity / fact | 扱い |
|---|---|
| BackingRegionId | fresh context-local nominal ID。address/binding/place/incarnationとは別 |
| Allocation | non-Copy/non-Discardable、regionへの唯一のfinal deallocation authority |
| Storage | non-Copy/non-Discardable、value-owned raw region/range claim |
| slot<T> | non-Copy/non-Discardable、empty typed claim、T/layout requirement |
| live root placement | place-owned region/range。ValuePackageに混入しない |
| byte | 256値、Copy/Discardable、native octet size/alignment=1、u8と別identity |
| RawDefined | compiler fact。typed lifetimeやauthorityを生成しない |

全explicit regionについて、commit前にalive Allocationが一つ、
Storage/slot/live-root rangesがdisjointかつfull regionを過不足なくcoverすることを検査する。
raw occupancyの重複、欠落、dead-region responsibilityを許さない。
Allocation/Storage transferはregion identityを保存する。
P3のabstract fixtureはregion=0で分離し、実physical backingの証拠にしない。

| Operation | 結果 / preconditions |
|---|---|
| successful allocate | fresh region + matching Allocation + full Storage、fresh Unspecified |
| deallocate | same region/full-range raw claim、両value consume、backing end |
| split | source consume、same-region exact nonempty partition |
| merge | same region/disjoint/adjacent/contiguous、unionへconsume |
| into_slot<T> | exact sizeof(T)/alignment、same range、typed lifetimeを開始しない |
| erase_slot<T> | explicit slotのsame rangeをStorageへ戻すtotal operation |
| initialize | slotを終了、same placementにfresh root incarnation |
| take / destroy | rootを終了、same rangeのempty slot。takeはpayloadもforward |
| storage_len / storage_addr | current Storageのinline usize/addr、claimは不変 |
| storage_read_byte | bounds/read permission/Defined、inline byte |
| storage_write_byte | bounds/write permission、singleton Definedを設定、neighbors保存 |
| copy_raw_bytes | pre-state source sequenceのpointwise transfer、overlap safe |
| u8 ↔ byte | explicit total conversion、全256値とunknown scalarを支持 |

cycle integrationはallocate → split → into_slot → initialize → take →
再initialize → destroy → erase_slot → merge → deallocateを通過する。
rootのphysical placementとStorage payloadが別regionに属するnestedケースも通過し、
rootをtakeしてもpayload自身のclaim/regionを失わないことを確認した。
non-Discardable Storageのdestroy/storeは拒否する。

## Raw representation / access / boundary

regionごとのowned sorted interval partitionを使い、Unspecified、Defined-known
byte、Defined-unknownを区別する。unknown Defined byte同士のequalityは仮定しない。
raw bytes全体に比例するphysical/shadow allocationは行わず、SIZE_MAXのrangeも
小さいsummaryで扱う。constant singleton writes / constant-range copiesがbaseline。

copyはsource intervalsのselectionを先にsnapshotする。Defined-known/unknownと
Unspecifiedをpointwiseにdestinationへ移す。異なるregions、split claims、同一claim内の
両overlap方向、self-copy、zero count/end-anchorを検証した。
独立した8-byte sequence modelと全285合法constant selectionsを比較した。
Unspecifiedを含むraw copyは成功するが、scalar read/write loopのreadは拒否する。

BackingRegionのordinary raw read/write permissionをrefのread/write modeから分離した。
`ref<read,Storage>`でもauthorized raw writesが可能で、read-only/device相当のprofileは
ordinary accessへ昇格しない。platform/MMIO APIsは作っていない。

Storageへのrefはplaceを保持し、各entryでcurrent Storageのidentity/rangeを再取得する。
replace/swap後に新しいboundsを見ること、raw mutationがStorage placeのcurrent factや
claimを変更しないことを検証した。numeric addressが等しくてもregion/Allocation一致や
merge legalityを導かない。

raw writeでvalid byte representationを書いてもtyped byte rootやValuePackageを生成しない。
raw scalar resultsはinline bookkeepingであり、argument evaluationの既存ref Copy/reborrow
だけが別途記録される。into_slot/erase_slotだけでもtyped incarnationは始まらない。
take/destroyではraw rangeをUnspecifiedへ保守的に戻す。旧ptrのincarnationは復活せず、
同じplaceのfresh initialization、deallocation、numeric address reuseの後もsafe ref取得を拒否する。

## Failure / diagnostics

P3 candidate clone / validate / commitを共有する。binding ownership、selected-parameter
compatibility、ref liveness、call-local exclusive childも既存処理を共有する。
失敗時はartifactを返さず、contextのnames/types/packages/places/domains/scopes/history/
BackingRegion/access/全fixture byte stateを保存する。

Linux linker-wrapによるtest-only malloc/realloc fault injectionを全13 raw operations、
initialize/take/destroy、layout/scalar registrationへ適用した。
各operationで最初のallocationから成功直前まで全allocation pointを一つずつ失敗させ、
clone、operand evaluation、region creation、interval replacement、snapshot、commit前validatorの
OOM rollbackとsuccess cleanupを検証した。raw operationsは27–33、root transitionsは
36/38/38 allocation failuresを個別にrollbackした。
4095→4097 intervalsとなるwriteが4096のbudgetを超える場合もresource limitとしてatomicに拒否する。

semantic error、unsupported、precision、OOM/resource、internal statusを区別する。
first diagnosticのstatic code/category/messageとoperand role spanを持ち、同じ失敗を
二回実行して一致することを確認した。shared P3 failure pathsは既存P3 codeを保持する。
無効API argumentsはowner/outputを上書きしない。sourceなしはplaceholder span。

## Validation / reproducibility

P3 baselineは27/27 PASS。追加は10 raw unit groupsとcycle integrationの11 CTests、
計38 CTests。CLI、LLVM C API、artifact integrity、frozen M7.5 oracle smokeも毎回実行する。
M7.5 smokeは旧oracleの起動/identity確認であり、M8.3 raw semanticsのoracle agreementを
主張しない。formal proof / Leanはbuild dependencyでもP4検証の代替でもない。

baselineはLinux x86_64、C17、CMake/CTest、strict `-Wall -Wextra -Wpedantic -Werror`、
checksum-locked LLVM/Clang/clang-format 23.1.2。localはDebian 13、GCC 14.2.0、
CMake 3.31.6、Python 3.12.14。既存bootstrap/lock/provenance/workflowは維持する。

| Local configuration | Result |
|---|---|
| GCC Debug | 38/38 PASS |
| Clang Debug | 38/38 PASS |
| Clang ASan + leak detection | 38/38 PASS |
| Clang UBSan + halt-on-error | 38/38 PASS |
| GCC Release / NDEBUG | 38/38 PASS |
| clang-format-23 check / diff whitespace | PASS |
| bootstrap / frozen artifact integrity | PASS |

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-gcc --parallel 2
ctest --test-dir build-gcc --output-on-failure

# CC=clang-23 + a separate build directory for Clang/ASan/UBSan.
# Sanitizers: -DNEWLANG_SANITIZER=address or undefined at configure time.
# Release: -DCMAKE_BUILD_TYPE=Release in a separate GCC build directory.
bash scripts/check-format.sh
```

PR CIはfresh Ubuntu 24.04でGCC/Clang/ASan/UBSanの同じ38 CTestsを実行し、
Clang jobがformatを検査する。current-head CI run URL/statusとexact SHAはPR本文と
最終handoffに記録する。古いgreen runをcurrent-headの代わりにしない。

## Findings / bounded limits

- **COMPILER-IMPLEMENTATION（修正済み）**: `replace(w,newStorage)` / `swap`後、
  bindingがhistorical ValueIdを保持し、後のidentifier consumeが古いclaimを参照していた。
  Draft §3.3のcurrent Storage、§13.5a/§17.4のcurrent-value factに従い、Change(place)時に
  bindingをcurrent packageへ同期した。raw current-value/replace/swap testsで固定した。
- **COMPILER-SPEC-AMBIGUITY（nonempty sliceのblockerではない）**:
  最小例 `allocate(0,1)` / `split(s,0)` / `split(s,storage_len(s))`。
  Draft §§3.2–3.3はempty rangeのaffine responsibilityとsplit端点の細部を明示していない。
  Backend Contractのlayout/authority規則もempty claimのsurfaceを裁定していない。
  M7.5はM8.3より前でありこの意味論のoracleにはしない。formal evidenceを代替裁定にしない。
  現状はexplicit unsupportedとしてdeferし、emptyを勝手に合法/違法と定義しない。
  将来のM adjudication対象候補だが、今回のnonempty sliceにDraft変更は不要。
- **COMPILER-PRECISION**: unknown dynamic bounds、unknown size/layout/alignment、
  path-sensitive/whole-program definedness proofsは不足を成功扱いせず拒否する。
  Definedだがconstant value未知の場合はUnspecifiedとせず、soundなDefined-unknownを維持する。
- **COMPILER-IMPLEMENTATION-LIMIT**: 各context table 4096 entries、全regionのraw intervals
  4096、host size_t/native x86_64 profile。legacy abstract slotにはexplicit backingがないため
  erase_slotはunsupported。P3のref/slot/domain payload nesting、exclusive mode-changing
  compatibility、loose exclusive contextual conversion、loan-body checkingのdeferralsを維持する。
- **COMPILER-PERFORMANCE**: candidateはcontext全体をcloneし、occupancy disjointnessは
  bounded pairwise check。P4はwhole-program range solverやscalable incremental engineを主張しない。

新しいCOMPILER-SPEC-HOLE、blocking ambiguity、COMPILER-LOWERING conflictは発見していない。
canonical Draft変更が必要な矛盾も発見していない。仕様を実装都合で書き換えていない。

## Review handoff

P4 IMPLEMENTATION COMPLETE。P4 READY FOR REVIEWはrequired current-head CI greenを確認して
handoffする。review gateで停止し、CodexはPRをmergeせず、P5 / M8.4 frontend / relocation /
LLVM workを開始しない。commit/runの正はGit history / PR checksであり、文書に別のchange logを持たない。
