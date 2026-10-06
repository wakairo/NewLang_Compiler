# R4-01 targeted argument compatibility fix

Track: P。R4-01 / compiler soundness / Mediumへのproduction fixのみ。
開始main: `7eb7c71d83b7771a6dcf51f0879057f9d2c489a1`。
mainの開発運用方針を先に確認した。`CURRENT_SPEC.md` → **Draft 17.10**。
[Issue #53](https://github.com/wakairo/NewLang_Compiler/issues/53) と
[shared-path audit](https://github.com/wakairo/NewLang_Compiler/issues/53#issuecomment-6008652124)
を参照する。canonical Draftは変更しない。branch: `r4-01-argument-compatibility`。
PR/exact head/current-head CIはIssue #53の最終 `Track: P` handoffとPR本文を正とする。

## Central correction

`ref_compatible` にexclusivity一致を含め、write→read compatibilityをordinary同士に限定。
`binding_argument` はselected ordinary-ref expectedに対するexclusive actualを、child生成や
non-Copy consumeより前に `P3-TYPE-MISMATCH` でrejectする。
既存exclusive mode-change判定は先に適用し、write→ordinary/exclusive readの
`P3-EXCLUSIVE-MODE-UNSUPPORTED` を維持する。

§11.4 ordinary weakening、§12.1 selected exclusive parameterへのcompatible exclusive child、
§18.2/§18.4のtransfer/type compatibilityを区別した。§12のindependently constructed ordinary
childの可能性を禁止するruleではない。callがexclusive actualをordinaryへ自動adaptしない。

signature-only / P8 body-backed calls、initialize stability、replace/store/swapのordinary operands、
take/destroyのexclusive operands、programmatic raw binding operandsが同じpathを使う。
`parameter_match` のordinary両者限定weakeningは維持。expression/loose operandsの既存
no-adaptation fenceも変更しない。各callee/primitiveへのspecial caseは追加していない。

## Regression evidence

新規 `r4_01_argument_compatibility.unit` は修正前にN1の誤受理を再現してfailし、修正後にpass。
全rejectionはpublic snapshot比較でscope/value/binding/place/current fact/occurrence/raw stateを
検査し、parent Available、child非残存、diagnostic repeat determinismを確認する。
N6はslot/value両non-Copy argumentsのtentative consume後のfailureもrollbackする。

| Control | 結果 / test |
|---|---|
| P1 exclusive→exclusive same-mode | 新unitでread/writeを各2回。childはfresh shorter scope、call終了後inactive、parent Available。既存P3 suspension testsもpass |
| P2 ordinary write→read | 新unitでordinary childをCopy/contextually weakened、reborrow scope無し。既存P3とP8 body call weakening testsもpass |
| P3 take/destroy sequential reuse | 既存P3 ending testsでtake / take / destroyの間同じouter authorityがAvailable。scope/local child semanticsを維持 |
| N1/N2 signature-only same-mode exclusive→ordinary read/write | P3-TYPE-MISMATCH |
| N3/N4 body-backed ordinary read/write | P3-TYPE-MISMATCH。bodyは事前に独立validation済み |
| N5 replace/store/swap ordinary write operands | P3-TYPE-MISMATCH。swap第2operandと先行argument評価後のfailureも検査 |
| N6 initialize ordinary stability | P3-TYPE-MISMATCH。ordinary evidenceへ切替後のinitializeはpass |
| N7 exclusive write→ordinary read | existing mode unsupported維持 |
| N8 exclusive write→exclusive read | existing mode unsupported維持 |
| raw Storage len/addr/read/write/copy | exclusive actualは同じtype mismatch。independently constructed ordinary childはCopyとして利用可能 |

## Corrected test fallout

- P3 ending unit: `initialize(vacant,recovered,ending)` の旧成功期待はordinary stability operandへ
  exclusiveをadaptしていた。negativeへ変更し、ending scope終了→ordinary stable供給で成功を検査。
  それ以前のsequential take/take/destroy reuseは同じparentのまま維持。
- P5 source cycle: single block内でendingをinitializeへ渡す旧期待は無効。hostでordinary stability /
  exclusive ending scopesを明示的に交替し、各receiving/initialize/take/destroyをparsed source
  fragmentsから実行。backing range、fresh incarnation、stale ptr、final deallocation検査は維持。
  source grammarやsource loan closureを追加しない。initializeのreborrow child数は0、takeは2。
- P6 sum lifetime: ordinary stability scopesでinitializeし、exclusive ending scopesでtake/destroy。
  payload occurrenceの終了/fresheningとstale pointer検査を維持。
- raw reborrow unit: Storage observation/raw accessのordinary expectedへexclusiveを自動adaptする
  旧期待も同じfallout。direct-exclusive negativesとexplicit host-constructed ordinary child positivesへ
  変更。two operandsでもexclusive actualはtype mismatch、ended ordinary childはdead-scope rejection。

この修正はinvalid期待を保つためcanonicalを変えない。既存64 CTestsを削除/skipせず、1追加して65。

## Validation / review boundary

locked bootstrap verification pass。LLVM/Clang/format 23.1.2、GCC 14.2.0、CMake 3.31.6、
Python 3.12.14、Debian 13。GCC Debug / GCC Release-NDEBUG / Clang Debug / ASan-leak / UBSanは
各 **65/65**。CLI/diagnostics、artifact integrity、LLVM C API smoke、frozen oracleも全構成でpass。
formatは既存pinned non-modifying checkerを使用。
PR-triggered CIは既存5 Ubuntu jobsでexact current headを検証し、final handoffへrun/jobsを記録する。
普通の再現手順はREADME通り。focused regressionは:

```sh
. .deps/activate.sh
CC=gcc cmake -S . -B build-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-gcc --parallel 2
ctest --test-dir build-gcc -R 'r4_01|semantic_value_use|semantic_transition|raw_reborrow' --output-on-failure
ctest --test-dir build-gcc --output-on-failure
bash scripts/check-format.sh
```

新しいspec ambiguity/holeやprecision limitは発見していない。
no toolchain/workflow/pin変更。R4独立revalidationやclosureは本作業の成果とはしない。
PRをmergeせず、M9.4/F2/R4 revalidation/LLVM/relocationを開始しない。

**R4-01 FIX READY FOR REVIEW**
