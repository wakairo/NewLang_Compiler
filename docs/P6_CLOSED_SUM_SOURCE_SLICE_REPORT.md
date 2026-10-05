# P6 — Closed-sum source slice report

Status: P6 implementation review packet。Git historyを変更履歴の正とする。

## Base / authority / Phase A

- repository: https://github.com/wakairo/NewLang_Compiler
- branch: `p6-closed-sum-slice`
- base main: `2b327edc000a913a49adb7cfd5f78a95c1e931b8`
- canonical: `CURRENT_SPEC.md` → Draft 17.10。参照snapshotsを変更していない。
- [Issue #35](https://github.com/wakairo/NewLang_Compiler/issues/35) の Phase Aを実装前に
  [Track: Pとして記録](https://github.com/wakairo/NewLang_Compiler/issues/35#issuecomment-5995873897)した。
- F1.2 reportをFormalProof `a3a8c70becb3da6a9c2fc6f91ca578f49b19df32` で確認。
  formalizationをnormative authority / build dependencyにしていない。

surface auditで仕様上の不足は見つからなかった。`=>` adjacency、constructor shape、3種類のpattern、comma/trailing commaを既存lexerのtoken/spanで実装できる。
P5にはreturn/function frontendがないため、Issue指定の非終端W1 equivalentを採用した。

## Implementation

- nominal closed-sum registry、owned variant names/types、static Copy/Discardable derivation。
- independent sum/payload ValuePackages、deep Copy ownership、returned old packageのoccurrence非移送。
- root-owned append-only conditional OccurrenceId、conditional child site、explicit ref occurrence dependency + parent scope。
- qualified constructor、match/pattern syntax、semantic resolution/shape/exhaustiveness。
- consuming / borrowed match、全armの独立candidate検査、owned arm checked evidence。
- exact result / affine availability join、Copy unknown widening、bounded identity-preserving sum forwarding。
- whole replace/store occurrence freshening、payload replace/store preservation、live capability conflict rejection。
- P4 Storage occupancyと既存initialize/take/destroyのroot boundaryを保持。sum invariant validationを追加。

詳しいsupported/deferred matrix、ownership / branch-state contractは
[P6 contract](P6_CLOSED_SUM_SOURCE_SLICE_CONTRACT.md)を参照する。

## W1–W4 / destructive evidence

| Witness | 結果 / evidence |
| --- | --- |
| W1 Result-like | `ResultHeaderError` / `Result` のnon-Copy whole consumption、fresh payload binding、Ok/ErrまたはSuccess/Failure handlerへの移送、同じCopy result typeへのjoin。known variantでも他armのerror/type/availabilityを検査。真のreturnは未実装 |
| W2 borrowed payload | parent modeを継承したpayload ref、parent scope + occurrence dependency、payload replace/storeでoccurrence/root incarnation維持。live payload ref下のwhole transition reject。borrowed wildcardならcapabilityを作らず全arm共通のwhole updateを許可 |
| W3 Storage | successful allocateのStorageをOptionStorage.Someへ移し、consuming Some(s)の全armidentity re-packで同一raw claimを保持。consuming Some(_) reject、borrowed Some(_) accept。runtime Noneでもstatic whole-type store / wildcard guardを弱めない |
| W4 freshness | Some(old)→Some(new)でroot incarnation維持、old occurrence dead、distinct fresh occurrence。returned old sumはloose payload valueだけを保持。payload-onlyとの対比を直接gettersで検査 |
| ptr pressure | payload ref→ptrからscope/occurrence blocking dependencyを除き、whole updateを許可。同型fresh payloadでもold ptrのsafe loanをstaleとしてreject |

その他: unknown/wrong qualifier/variantとlookup fallback禁止、payload arity/type、malformed arrow/comma、general pattern unsupported、empty/missing/duplicate arm、wrong-sum pattern、shape、Copy binding availability、non-Copy use-after-consume、partial arm binding nonescape、unsupported ref result、mixed branch-state precision、call preservation summary不足、conditional payload take拒否。

initialize→take→initialize→destroyのsum lifetime cycleも直接検査した。old occurrenceはtakeで終了し、reinitialize時にはroot incarnation / occurrenceともfreshになる。operation-local ending authorityはAvailableのまま再利用できる。

## Ownership / failure verification

`sum_slice_test` はregistration/name ownership、Copy package independence、root/occurrence views、all-arm validation、Storage conservationを直接検査する。
`sum_frontend_test` はsource→syntax→checked pipelineでactual Failure/None/Someを跨ぐworkloadを検証する。
variant registration、parser node、context clone、constructor attach、whole replace、hypothetical arm context、owned arm evidence、common-state constructionの各malloc/realloc indexをtest-only fault injectionで失敗させ、public state不変・output NULL・cleanupを検証する。
state snapshotにvariant/value ownership/occurrence/ref dependencyを加え、既存failure testsもこれらを比較する。
resource budgetのvariant数 / owned arm数もfailure atomicityを検査する。

旧45 CTestsを保持し、P5で旧unsupportedだったmatch parse exampleをcanonical P6 validへ変更した。
P6の7 unit groupsとsource→checked integrationを加え **53 CTests**。

## Validation / reproducibility

既存toolchain/pin/workflowを使用し、upgradeしていない。
Linux x86_64 Debian 13、C17、GCC 14.2.0、Clang/LLVM/clang-format 23.1.2、CMake 3.31.6、Python 3.12.14。
`python3 scripts/bootstrap.py` はTLS/exact package/checksum lockを再確認して成功した。

| Local configuration | Result |
| --- | --- |
| GCC Debug | 53/53 PASS |
| GCC Release / NDEBUG | 53/53 PASS |
| Clang Debug | 53/53 PASS |
| ASan / leak detection | 53/53 PASS |
| UBSan | 53/53 PASS |
| pinned clang-format | PASS |

PR CIも同じ全test setを実行し、exact headの最終resultは下記review recordへ記録する。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build-p6-gcc -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p6-gcc --parallel 2
ctest --test-dir build-p6-gcc --output-on-failure
```

GCC Release/NDEBUGは別build directoryで`-DCMAKE_BUILD_TYPE=Release`。
Clangは`-DCMAKE_C_COMPILER=clang-23`。
ASan/UBSanはそれぞれ`-DNEWLANG_SANITIZER=address` / `undefined`。
すべて同じ53 tests（CLI/diagnostics、LLVM C API、artifact integrity、oracle smokeを含む）を実行する。
ASan leak detection / UBSan fail-on-first設定は既存CTest設定を使用。
formatは`bash scripts/check-format.sh`。

current-head PR-triggered CIのURL/head/jobsは、このbranchのPRとIssue #35最終Track: P commentに記録する。
CIはUbuntu 24.04 / GCC 13、pinned Clang 23.1.2の既存5-job matrixを使う。
reportへのself-referential SHA埋め込みはせず、PR headのexact evidenceをreview recordとする。

## Findings / limits / disposition

- COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY: なし。Draft変更不要。
- COMPILER-IMPLEMENTATION-LIMIT: function/return、nested sum/aggregate/ref/ptr payload、nested match、sum swap、escaping payload ref、live conditional ptr acquisition body planは未実装。
- COMPILER-PRECISION: general branch-state/result join、pre-existing external payload refを持つborrowed matchのrelational guard、core authorityの一般join、whole/no-write混在、variant-varying whole update、non-Copy payload mutation join、occurrence preservation summaryのないwhole-sum write callは保守的にreject。
- COMPILER-IMPLEMENTATION / COMPILER-DIAGNOSTIC: bounded grammar / ownership / deterministic diagnosticsを実装してtest。新たな仕様上のblockerなし。

P6はbounded source→checked evidenceであり、runtime execution / machine layout / whole-program correctnessを主張しない。
R3/F2/M9.2/LLVM/relocationは開始していない。PRはopenのままreviewを待ち、自身ではmergeしない。

**P6 IMPLEMENTATION COMPLETE / P6 READY FOR REVIEW** は全validationとexact-head CI確認後のIssue/PR記録により確定する。
