# P8 registered function body slice report

Track: P。**P8 reviewed/merged / CLOSED.**

## Authority / artifacts

開始main: `e8d9bf31353174062de3347e0b77c7e8e5b70851`。
`CURRENT_SPEC.md` → canonical **Draft 17.10**。運用方針を先に読んだ。
[Issue #47](https://github.com/wakairo/NewLang_Compiler/issues/47) と
[Phase A audit](https://github.com/wakairo/NewLang_Compiler/issues/47#issuecomment-6007426183)
がscope/coordination記録。
branch: `p8-registered-function-bodies`。PR URL、exact head、PR-triggered CI run/jobsの
最終証拠はIssue #47の `Track: P` handoffとPR本文へ記録する。
自己参照するcommit SHAをreportへ埋め込まず、Git/PRを正とする。

canonical Draft、Backend Contract、P0 lock/provenance、workflowは変更していない。
契約は [P8 contract](P8_REGISTERED_FUNCTION_BODY_SLICE_CONTRACT.md)。

## Implementation

- `semantic.h`: named parameters + transactional body registration API。
- `function_body.c`: durable owned source/syntax/name plan、explicit retain/release、shared exit check。
- `semantic.c` / internal header: registry clone/destroy ownership integration。
- `semantic_check.c`: caller-independent definition validation、actual-context body execution、
  fresh callee parameter bindings、namespace isolation、exact tail result forwarding、exit checking。
- `checked.h` / `checked.c`: owned child body evidence getter、plan lifetimeとrecursive cleanup。
- CMake: plan source追加、existing parserへのdependency、P8 test targets。LLVMとはリンクしない。
- tests: registration/exit/ownership/failureの4 unit groupsとsource→checked integration。

owned syntax planをactual checkerへ適用する最小方式を採用した。formal contextは登録時の
names/type/use/result/exit義務を検査するが、actual aliasing proofの代用にはしない。
formal IDsは捨て、callではactual complete factsを用いる。resultはbody packageをforwardする。

## Workload / destructive evidence

| Workload | 結果 / evidence |
|---|---|
| W1 Linear identity | original caller value ID = returned package ID。callerとfresh parameterはそれぞれconsume。destination incarnationはfresh。uncalled unused Linear parameterはreject |
| W2 unit / tail | empty unit bodyと合法statement-only bodyはpass。no-tail non-unit、exact tail mismatch、non-Discardable discardはdefinition時reject |
| W3 distinct / alias swap | actual referenced root values交換。same-placeはincarnation/current value/current factを含むPlaceView exact no-op。direct swapと同じprimitive経路。複数ordered swapとnested argument replaceでsource orderを検査 |
| W3 late failure | `{store(a,v);swap(a,b);}` のsecond actualがjoined writeなら、store後のprecision failureでも全state rollback |
| W4 unsafe | real local-scope refをcaller-visible carrierへinstallするとexit reject。loose result capability、global+local alternativesもreject。scope endだけではdependencyを消さない |
| W4 safe | temporary local-dependent packageをendしglobal capabilityへrestoreすると同じexit helperがaccept。source構築不可のためinternal fixtureでありfull loan surfaceではない |
| P7 joined actual | readonly bodyがcomplete alternativesを保持。fallback scopeがdeadならreject。sum occurrence conflictをcall後も保持。joined writeはsingular-operation precision rejection |
| ptr result | `ptr_from_ref` bodyのtail ID/factsがcall resultへ直接forward。write actual→read parameter compatibilityでunderlying place/incarnation/provenanceを保持。implicit borrowはreject |
| definition negatives | duplicate function/parameter、unknown name、caller capture、parameter rebind、non-block、unknown effects/dependencies、self recursion、body call chainをreject |
| lifetime ownership | 登録source/tree/parameter nameを破棄・書換え後もcall可能。clone/commit後も古いbody evidence有効。contextを先にdestroyしたartifact cleanupを検査 |
| OOM / limits | malloc/realloc sweepでregistration、Linear argument consume後のcall、caller-visible write後のcallの各失敗でpublic snapshot不変。65 body calls / 129 parametersはresource reject |
| grammar boundaries | source `fn f() {}` と `{return x;}` はparser rejection。source grammar/token rulesへの変更なし |

W3のsum-root variantはP6 sum mutation fenceを維持してdeferred。
同一place no-opのconditional occurrenceをbody-backed sum signatureで新たに通す主張はしない。
required negative controls 1–20は上記と既存P0–P7 suiteで検査する。

## Validation

既存59 CTestsを保持し、5追加して **64 CTests**。local Linux x86_64で以下を全通過:

| Configuration | 結果 |
|---|---|
| GCC 14.2.0 Debug | 64/64 |
| GCC 14.2.0 Release / NDEBUG | 64/64 |
| Clang 23.1.2 Debug | 64/64 |
| Clang 23.1.2 ASan + leak detection | 64/64 |
| Clang 23.1.2 UBSan + halt-on-error | 64/64 |

strict C17 warning policy維持。既存artifact integrity、LLVM C API smoke、frozen M7.5 oracle、
CLI/diagnostic/source/raw/sum/ref-join testsを各構成で実行した。
bootstrapはexisting checksum lock検証を通過。LLVM/format **23.1.2**、CMake **3.31.6**、
Python **3.12.14**、Debian 13。formatは既存非変更checkで検証する。
PR-triggered Ubuntu 24.04 CIは既存5 jobs（GCC Debug/Release、Clang+format、ASan、UBSan）を
使用し、exact current headの全greenをIssue final handoffの前提とする。
run/statusはPR/Issueのlinked GitHub evidenceで確認する。

再現例（全suite、P8のみならCTestに `-R p8` を追加）:

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-p8-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p8-gcc --parallel 2
ctest --test-dir build-p8-gcc --output-on-failure
CC=gcc cmake -S . -B build-p8-release -DCMAKE_BUILD_TYPE=Release
CC=clang-23 cmake -S . -B build-p8-clang -DCMAKE_BUILD_TYPE=Debug
CC=clang-23 cmake -S . -B build-p8-asan -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=address
CC=clang-23 cmake -S . -B build-p8-ubsan -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=undefined
# 各configurationでcmake --buildとctestを同様に実行
bash scripts/check-format.sh
```

## Findings / review boundary

- COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY: 今回scopeにblockerなし。
- COMPILER-COMPLETENESS: body source profileはsingle bindings/statements/name/call/blockのみ。
  source fn/return、general CFG、aggregate/sum body signatureは追加しない。
- COMPILER-PRECISION: core authority/exclusive signatures、ordinary ref function result、
  joined-ref write/general memory-state phi、unknown hidden dependenciesは保守的reject。
- COMPILER-IMPLEMENTATION-LIMIT: body→body chains/self recursionとhost budgets。
- W4 source construction limit: real local scope negative/safe restoreはinternal fixtureで検証。
  canonical semantic gapとして扱わず、source loan closureを開かない。

PR #48 exact head `3b35b5d6103f02308c3bbf395a7b8d23d1736bdc` はCoordination reviewをPASSし、mainへmerge済みである。
**P8 CLOSED**。このclosure自体ではM9.4/F2/R4/LLVM/production relocationを開始しない。
