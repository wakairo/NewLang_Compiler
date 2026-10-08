# Draft 17.25 heap-owned link native report

Track: P — [Issue #180](https://github.com/wakairo/NewLang_Compiler/issues/180)。
[Contract](DRAFT_17_25_HEAP_LINK_NATIVE_CONTRACT.md)。
Base main `fd6941568598bba70900ff140303299d0a3bdd5a`、Draft 17.25。
Historical design audit **N/A faithful implementation** / semantic delta **0**。

## Actual-source / native evidence

unchanged `heap_link_semantic.nl`がactual sourceからproduction checkerとowned checked artifactsを
経てC17へlowerされる。source frontend、semantic rules、checked metadataの形式は変更していない。
既存`checked_c_node.c`へbounded root/field/Copy/replace carrier照合を追加した。
selected symbols / projection identityとruntime carrierを結び、意味をsource textから再解釈しない。
private layout/type assertions以外にC offsetをsemantic proofとして使わない。

`heap_link_native.integration`はcanonical / renamed / payload変更 / selected ptr・stability alias /
root-ref aliasの5 actual-source variantsを検査し、別checked-only probeから期待scalar/operand/site情報を取る。
生成Cのptr/stability Copy→root ref、selected root-ref Copy→field carrierをその情報と照合する。
instrumented **NDEBUG** nativeとobserver無しnativeをstrict C17でcompile/runする。
applicable ASan/UBSanはgenerated executablesにも適用する。

read-only observerが検査するphysical facts:

- real mallocの1 allocation address = heap H owner address、distinct lexical H address。
- heap link初期None→Some(ptr == lexical H address)→Noneと不変payload sibling。
- mode W / R / Wで3 root refs、3 field projections、same semantic root/child identity。
- separate current Option Copyと実Some tagによるselected ptr、lexical tailのsafe reloan/read。
- first replaceのold None、second replaceのold Some、3 scoped ref extents終了後のEndRoot。
- slot/raw/domain回収後、original allocated addressへのmatching freeがちょうど1回。
- forced real malloc NULLではNone armのみ。domain/root/field/change/Copy/freeがすべて0。

local canonical observation例（addressはASLR依存、値の一致をgolden化しない）:

```text
OBSERVED heap=557ee45bf2a0 lexical=7ffe4081ef00 roots=3 fields=3 changes=2 copies=3 scopes=3 free=1
OBSERVED heap=0 lexical=0 roots=0 fields=0 changes=0 copies=0 scopes=0 free=0
```

observerはNodeをwrite/initialize/freeせず、platform shimはsuccessのgenerated callをdelegateし、
NULL injection以外にallocation resultを変更しない。EndRoot以降のNode memoryを読まない。
10 generated-C corruption controlsはwrong tag/ptr、lexical proxy write、wrong projected field、
missing write/unlink/free、double/early/wrong-identity freeをNDEBUG observerで検出した。
これらはcompilerによるstatic rejectionとは区別したruntime observation evidenceである。

## Static / checked / failure evidence

既存31 actual-source controlsの全semantic negativesを保持する。
semantic positivesのexact native shapeはCを生成し、richer positivesはbackend unsupportedを許す。
static rejectionでは引き続きempty C/stdout/no object/executable。
既存owned-arm evidence / nonCopy responsibilities / malloc-realloc registration/body sweepsと
**SUPPORTING programmatic** permission・Value/Occurrence blocker controlsも保持する。

`heap_link_backend.unit`は32 checked-node corruption controlsとwrong selected ptr symbolを検査。
missing/malformed root/ref mode、provenance、scope/D/range、projection key/site/fact/operand、
Copy/Change/Reset/owning Some grantをunsupportedとし、output NULL / length unchangedを確認する。
各repair後はclean retryで成功する。emitter malloc OOMでも同じoutput atomicityを確認する。
artifactの不足を補う修正やruntime安全性判定は導入していない。

## Reproduction / validation

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
ctest --test-dir build -V -R heap_link_native
bash scripts/check-format.sh
```

Clang: `-DCMAKE_C_COMPILER=clang-23`、GCC Release: `-DCMAKE_BUILD_TYPE=Release`。
ASan/UBSan: Clangで`-DNEWLANG_SANITIZER=address` / `undefined`、それぞれ別build directory。
Pinned LLVM/Clang/formatter 23.1.2 / checksum bootstrapを再利用。
local Debian 13 x86_64 / GCC 14.2.0 / CMake 3.31.6 / Python 3.12.14。
Full CTestは**198 tests**。旧#174 reverse-orientation allocated native、lexical topology、AVS、
raw/lifetime/Option、oracle.adapter/smoke、artifacts.integrityを含む。
local GCC Debug / GCC Release(NDEBUG) / Clang / ASan / UBSanは各**198/198 PASS**。
formatとbootstrap checksum verificationもPASS。
exact-head PR-triggered 5-job CIのSHA/run/job結果はIssue #180と候補PRに記録する。

canonical gap / new semantic blockerは確認していない。
一般member/allocatorやborrowed match等の既存precision制限は残す。
compilerのstatic受理とnative観測を別々の証拠として記録し、full memory-safety proof / cJSON /
North Star product PASSとは主張しない。
候補PRはOPEN / unmerged、独立Coordination ACCEPT/BLOCK待ちで停止する。
