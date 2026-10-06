# P7 — Ref-valued match-result join report

Status: P7 reviewed/merged / CLOSED。Git historyを変更履歴の正とする。

## Base / authority

- repository: https://github.com/wakairo/NewLang_Compiler
- branch: `p7-ref-result-join`
- base main: `fb67bd68990b40093adb1d33cd7fbce2ee3321aa`
- canonical: `CURRENT_SPEC.md` → Draft 17.10、変更なし。
- [Issue #41](https://github.com/wakairo/NewLang_Compiler/issues/41) / [Phase A Track: P](https://github.com/wakairo/NewLang_Compiler/issues/41#issuecomment-6006550788)。
- exact review head、PR URL、PR-triggered run/job evidenceはIssueの最終Track: P handoffとPR Checksに記録する。
  自己参照するcommit SHAをこのfileへ埋め込まない。

## Implementation / invariants

`NLSemanticValueView` が最大16のinline complete ref alternativesを所有する。
concrete factとjoined may-setを明示的に区別し、joined resultのsingular factはabsent。
resultのplace/incarnation/scope/provenance/access/occurrenceを一候補として保持する。
clone/Copyは全候補を保存し、endは当該packageだけを終了する。

arm-owned resultはpublic existing-origin proofまたはborrowed payload structural proofでrebaseする。
known Noneのinfeasible payload候補をpruneし、hypothetical scope/place/occurrenceをpublic stateへ輸出しない。
known Someのresultはcurrent payload occurrenceとfallbackのmay-setを保持する。
arm lexical binding名が終わってもparent ref scopeが生存するpayload resultは使用可能。
全alternativeのsafe-use検査とconflict検査を行う。
詳細なownership / supported matrix / failure contractは[P7 contract](P7_REF_VALUED_MATCH_RESULT_JOIN_CONTRACT.md)。

## PW1–PW5

| Witness | Result / evidence |
| --- | --- |
| PW1 R3-01 | `let chosen=match r {Some(v)=>{v},None=>{fallback},}; observe(chosen);` のbindingとcommon continuationを同一contextで検査。known Some / None、両arm orderで成功。Someはparent scope + public occurrenceとfallback、Noneはfallbackだけ |
| PW2 different refs | two pre-existing refsを保持。arm reorderでもsorted complete factsが同じ。どちらの候補scopeが終了してもsafe useをreject |
| PW3 invalidation | Some由来候補が残るresult/Copyはwhole store/replaceを阻止。fallback scope終了後もpayload側conflictを保持。Noneのfallback-onlyならroot transition後も使用可 |
| PW4 scope exit | 実際のincoming ordinary scope終了後のsource useとhost rejoinをrejectし、失敗stateは不変。arm binding名の終了だけではPW1をrejectしない |
| PW5 wildcard | actual returned external refsのみのdependency。payloadをignoreしたresultにoccurrenceを付加せず、whole transitionとresult useが成功 |

## Direct unit / source integration evidence

`ref_join_test` はpublic semantic module APIを使い、lifecycle/Copy/view ownership、
16/17境界、stale/unknown/invalid/access facts、occurrence laundering rejection、
owned hypothetical arm snapshotとpublic resultのID分離、fault injectionを直接検査する。
`ref_join_frontend_test` はsource → parser → checkerでPW1–PW5とnegative continuationを検査する。
parserやprivate `.c` のtest-only公開は追加していない。

source/APIの全malloc/realloc indexを最初のsuccessful operationまでfault injectionする。
alternativeの所有storageはvalue tableなので、そのclone/growthとchecked arm/artifact/result/bindingの
allocation pathも対象。failure時にはtype/value/binding/place/scope/occurrence/raw stateと
全alternativeをpublic getterで比較し、output artifact/IDが不変であることを確認した。
late unresolved arm / exact result type mismatch / branch mutation precisionでもrollbackを確認。
arm内だけで作られたcompound TypeIdもpublic IDとして扱わずprecision rejectする。

P0–P6の全53 CTestsを残し、P7の5 unit groups + source integrationを追加して計59。
P6の旧escaping-ref negative witnessはpayload-refとsum-refの型が異なるため、
現在もrejectし、理由を正しい `P6-RESULT-JOIN` に更新した。

## Validation / reproduction

bootstrap checksum/TLS/signed provenance/exact LLVM 23.1.2 pinは変更なし。
local Linux x86_64でGCC 14.2.0、Clang 23.1.2、CMake 3.31.6、Python 3.12.14を使用。
CIは既存Ubuntu 24.04のGCC 13 / pinned Clang 23.1.2、既存5-job workflowを使用する。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build-p7-gcc -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p7-gcc --parallel 2
ctest --test-dir build-p7-gcc --output-on-failure
```

他のconfigurationはbuild directoryを分け、以下を指定する。

| Configuration | Configure options | Required result |
| --- | --- | --- |
| GCC Debug | `-DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug` | 59/59 |
| GCC Release/NDEBUG | `-DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release` | 59/59 |
| Clang | `-DCMAKE_C_COMPILER=clang-23 -DCMAKE_BUILD_TYPE=Debug` | 59/59 |
| ASan + leaks | Clang Debug + `-DNEWLANG_SANITIZER=address` | 59/59 |
| UBSan | Clang Debug + `-DNEWLANG_SANITIZER=undefined` | 59/59 |
| Format | `bash scripts/check-format.sh` | pass |

local全configurationを検証し、PR-triggered CIも同じ59 tests（LLVM smoke/oracle/artifact integrity含む）を
5 jobsで実行する。最終READY判定ではPR current headとのSHA一致と全job successを
GitHub APIで確認し、run URL/jobsをIssueとPR本文に記録する。過去headのgreenで代替しない。

## Findings / limitations / gate

- `COMPILER-COMPLETENESS` R3-01: 指定bounded witnessとdownstream floorを実装済み。closure裁定はCoordination/reviewerに委ねる。
- `COMPILER-PRECISION`: joined write mutation / ptr conversion / memory-state phi / core/exclusive/aggregate/sum-target ref join /
  live external occurrence下の同一root再matchは保守的に拒否。
- `COMPILER-IMPLEMENTATION-LIMIT`: 16 distinct alternatives、既存table/depth/arm budgets。
- `COMPILER-SPEC-AMBIGUITY` / spec hole: 発見なし。Draftを変更していない。
- `COMPILER-IMPLEMENTATION`: common joinでarm-local compound TypeIdをpublic type tableへindexする経路にbounds guardを追加。
  ptr-valued matchはsupportを増やさずprecision rejectし、同じ数値IDを二つのcloneが生成するnegative witnessを追加した。
- 未解決の `COMPILER-IMPLEMENTATION` failureなし。OOM/resource/semantic failureのatomicityを維持。

PR #42 exact head `fdf547378a6d7a81a87554a232a6c7f0049d7791` はCoordination reviewをPASSし、mainへmerge済みである。
**P7 CLOSED**。このclosure自体ではM9.3/F2/R4/LLVM/relocationを開始しない。
