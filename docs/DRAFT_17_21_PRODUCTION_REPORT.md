# Draft 17.21 production catch-up report — Issue #146

Track: P

Base main: `46e07d7f627fc037dc794c3edb28d720815ca196`。
Canonical: CURRENT_SPEC → Draft 17.21。Issue #146のP-A〜P-Jだけを実装した。
Draft §16.3のdeclaration/type-graph deltaを接続し、追加のsemantic inventionはない。
normative snapshots、oracle archive/adapter、toolchain pinsは変更していない。

## P-A〜P-J evidence

| Package | Evidence |
|---|---|
| P-A parser | exact two-field Option<ptr<same nominal>>,u8。distinct labels/trailing comma、arbitrary/malformed type formsをreject。Node/labelsはspecial builtinではない |
| P-B header | stable incomplete nominal TypeIdとcompletion metadata。cloneで保存。public completion-only query、incomplete full shape/property/value/root/layout/signature使用を拒否 |
| P-C ptr | direct target Hだけselected formationを許す。target property/layout不要、ptr Copy/Discardable independent。type-only registrationでpublic values/places/provenance/incarnations/domain/occurrenceを生成しない |
| P-D Option | concrete None/Some(ptr<H>) identityをptr type argumentでintern。repeated exact type source/API resolveでsame TypeId。existing sum validation/copy/occurrence ownershipを保持 |
| P-E cycle | semantic aggregate/sum containment traversal、ptr barrier。selected shape accept、direct/sum-mediated self containment fixture reject。C/backend/layout不使用 |
| P-F completion | same H TypeIdへexactly-once commit、completed fieldsからCopy/Discardable derive。identical/inconsistent repeats、unresolved header reject。failureにpartial public headerなし |
| P-G unit order | recursive categoryをcollect→resolve/complete→signature/body validation。declaration before/after function、physical input permutationでsame properties/type graph/acceptance。旧AVS ordering不変 |
| P-H actual source | None-link Node construction/whole destructuringとordinary main bodyがchecker accept。completed metadata、checked type evidence、sum/aggregate value ownershipを検証。CLIではexplicit backend unsupported、Cなし |
| P-I destructive | unknown self target、known different nominal、duplicate declaration/completion、inconsistent completion、unresolved header、malformed/arbitrary generic、incomplete use、order、allocation failure/rollbackを検証 |
| P-J regression | existing product gates/oracle/integrity/full CTestをGCC Debug/Release、Clang+format、ASan、UBSanで検証。各176/176 pass |

primary sourceにhost-known seed/prelude/helperはない。Some ptr-payload/copy/occurrence testは
trusted internal controlで、actual-source runtime topologyの成功証拠に数えない。
Checked-C emitterは変更せず、Node semanticsの検査をCへ委譲しない。

## Files / ownership

- `src/recursive_type.c`：candidate header/Option/completion、bounded containment、value eligibility。
  strings/type tableはcontext所有、cycle colorsはcall内所有。partial helper stateはprivate
  transactionだけに存在し、complete helperはallocation前にfieldsをcommitしない。
- semantic view/internal metadata/APIと`src/semantic.c`：completion-only query、clone保存、
  incomplete ordinary use guards、Option memberのowned deep Copy。
- syntax/parser/checker：exact node、category-specific collection、same-target resolution、
  existing constructor/aggregate checkerへlowering。Pair field gateは変更しない。
- tests/CMake：6 unit groups + CLI integrationをnormal full CTestに登録。
- READMEとcontract/report：bounded coverageとhandbackを記録。

## Limitations / findings

general recursive types/generics、mutual recursive source declarations、arbitrary containment
completion、Node field source、recursive Checked-C/LLVM、raw allocation/lifecycle、FFI、modules
は未実装。これらのprecision/profile fenceをlanguage-invalid一般則へ昇格しない。
canonical gap/ambiguity、ptr lifetime/provenance変更の必要性は発見していない。
source Option-param/function-result等は既存P8 signature precisionを維持する。
oracle overlap classifierは未変更でexplicit/input-based/fail-closed。

## Validation / handoff

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
bash scripts/check-format.sh
```

GCC Releaseはbuild type Release。Clangは`clang-23`。独立sanitizer buildで
`-DNEWLANG_SANITIZER=address` / `undefined`を指定する。既存required CI matrixは
Ubuntu 24.04、GCC 13 Debug/Release、Clang+format、ASan、UBSan。toolchain lockは不変。
Local baseline: Debian 13 x86_64、GCC 14.2.0、Clang/LLVM/clang-format 23.1.2、
CMake 3.31.6、Python 3.12.14。bootstrap checksum検証、5構成のfull CTest各176件と
format checkはpass。exact-head CI結果、head SHA、PR/run URLはIssue #146とPRの
handoff recordへ記録する。
review-readyはcurrent-head required checksがgreenの場合だけ宣言する。

marker: `DRAFT 17.21 PRODUCTION CATCH-UP READY FOR REVIEW`。
PRをmergeせず、Issueをcloseせず、recursive executable topologyや次sliceへ進まない。
