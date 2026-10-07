# Draft 17.23 Node @link production report — Issue #159

Track: P

Base main: `457bc4eea79441f12880daf808d916a7ae52e80d`。
Branch: `draft-17-23-node-link-semantic`。canonical: CURRENT_SPEC → Draft 17.23。
Issue: <https://github.com/wakairo/NewLang_Compiler/issues/159>。
Exact review head / PR / PR-triggered CI URLはIssueとPRのfinal handoffに記録する。

Historical design audit: N/A — faithful implementation of Draft17.23 §§16.3, 17.1, 13.7/13.8, 26.

Process、Design Decision Procedure、DI-001/002/007を確認。新しいsource/semantic designはない。
core semantic delta = 0。FormalProofへの変更なし。

## Rule → implementation → evidence

| Selected rule | Implementation | Evidence |
|---|---|---|
| §16.3 completed H / §17.1 link selection | `nl_recursive_local_type` + `fixed_selection`がheader/completion/type/indexを照合 | metadata名をCell/link/dataへ変更したactual source、wrong/incomplete nominal、payload/unknown/category negatives |
| §17.1 current link Copy | `field_read` → existing recursive `nl_sem_copy_value` | `node_link_evidence` / `transitions`で独立Option/payload IDs、exact ptr provenance保存、checked field/read type |
| §13.7/13.8 ordinary field loan | existing `source_local_loan`のfixed-child selection / stability / scope-exit | exact ordinary ref target/access、non-exclusive、exactly-once/forwarding/nonescape evidence、unused/alias pass、escape/misuse reject |
| Change(root/link) / sibling preservation | `nl_fixed_change`がaggregate ownerを更新しroot/link factをfresh化 | root/link incarnation維持、siblingのplace/fact/value/incarnation完全保存、owner移行、ancestor/link dependency reject・sibling dependency pass |
| §26.7 whole-Option occurrence transition | fixed attach/detach/changeとsum validatorを接続 | None→Some / Some→Some / Some→None、old occurrence End、fresh Some occurrence、old returned OptionとCopy観測のptr保持、store discard、live payload ref conflict |
| §13.7/13.8 Node local read / ptr reloan | exact completed recursive rootをread guardへ追加し既存reference/current/visibility proofを維持 | primary source、stale lexical target、same-place fresh incarnation、hidden governing root、stale Some payload再取得reject |
| finite match continuation | Node Option unit-only exact unchanged prefix proof、全arm検査 | primary Some armのsafe reloan、二つのnormal unit arm、branch-owned artifact context。arm mutationは`P9-CONTINUATION-PRECISION` reject |
| transactional / owned evidence | existing candidate clone/commit、recursive owned-value End、new attach allocationsはcandidate-owned | registrationとbody/read/Some→Someの全malloc/realloc fault positionsでOOM rollback / no artifact、fact/incarnation/occurrence-history resource fences、duplicate-owner invariant reject |

Primary witnessは`tests/fixtures/node_link_semantic.nl`そのものをunit registrationとCLI integration
へ渡す。initial tail ptrはactual `loan_read` / `ptr_from_ref`から取得する。
registration後にoriginal source/treeをdestroyしてもretained source planとchecked call bodyが動作する。
Some-arm pointer reloanのevidenceはそのarm context内に保持し、public contextへbranch-local IDを移さない。

## Actual source / diagnostics matrix

| Input | Result |
|---|---|
| Issueのtwo-root witness / same-variant replacement / renamed nominal+fields | checker accept → explicit deterministic `V1-BACKEND-UNSUPPORTED`、stdout empty、C/object/executableなし |
| `head@payload` | selected Node category → `NODE-LINK-FIELD-PROFILE` (unsupported bounded surface) |
| `head@absent` | selected nominal → `FIELD-UNKNOWN-FIELD` (semantic error) |
| ptr/ref/wrong nominal / same-name scalar base | `FIELD-PROFILE`、sum/receiver fallbackなし |
| `head::next` / legacy dot | `P6-SUM-QUALIFIER` / `SOURCE-DOT-RESERVED`、field fallbackなし |
| nested/arbitrary expression base | existing parser unsupported/syntax diagnostics、general projection grammarなし |
| whole Node write | `LOCAL-LOAN-PROFILE`、read拡張がwriteへ漏れない |
| wrong incoming Option/value / escaped ordinary write ref | `P3-TYPE-MISMATCH` / `P8-EXIT-DEPENDENCY` |
| ptr to ended or replaced incarnation | `P3-STALE-POINTER`、Someやaddress coincidenceからauthorityを生成しない |
| Unknown/hidden facts / richer match state | structured precision rejection、language-invalidやsuccessへ置換しない |

Exact dependency / stale restart / live conditional ref / resource controlsの一部はtrusted internal
fixturesである。primary source acceptance / backend gateの代用には数えない。

## Validation

Local full CTest: GCC Debug、GCC Release/NDEBUG、Clang Debug、ASan、UBSan各184/184 PASS。
既存Pair native、AVS/u8/local-root、function/control/sum/generic/dependency suites、
`oracle.adapter` / `oracle.smoke` / `artifacts.integrity`を含む。formatも必須。
Exact-head PR-triggered five-job successはfinal handoffのlive runとSHAで検証する。

Locked rootless bootstrapは既存package URL/version/SHA-256/provenance/pinをそのまま検証する。
Local host Debian 13 x86_64、GCC 14.2.0、Clang/LLVM/clang-format 23.1.2、CMake 3.31.6、
Python 3.12.14。CIは既存Ubuntu 24.04 / GCC 13 / locked Clang 23.1.2 matrix。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
bash scripts/check-format.sh
CC=gcc cmake -S . -B build-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-gcc --parallel 2
ctest --test-dir build-gcc --output-on-failure
```

Releaseは`-DCMAKE_BUILD_TYPE=Release`、Clangは`CC=clang-23`、sanitizersはClang Debugに
`-DNEWLANG_SANITIZER=address` / `undefined`を指定する。READMEの再現手順を維持する。

## Findings / scope audit / handoff

- COMPILER-IMPLEMENTATION: aggregate-owned Optionのconditional occurrenceとrecursive field Endを
  productionへ接続した。これは既存所有/occurrence規則の実装であり新Reset/lifetime semanticsではない。
- COMPILER-IMPLEMENTATION-LIMIT: Node executable backendは未許可・unsupportedのまま。
- COMPILER-PRECISION: function-body Node Option matchの新joinはunit結果・exact unchanged frameのみ。
  current correlated memoryを更新する複数normal arm等は既存precision fenceで停止する。
  aggregate-owned linkへのborrowed matchはfield-aware hypothetical occurrence guardsがこのslice外なので
  `NODE-LINK-MATCH-PRECISION`で停止する。primary witnessはCopyしたOption valueをmatchする。
- COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY: このbounded mappingで新規blockerなし。

canonical Draft/CURRENT_SPEC、Ledger、FormalProof、frozen oracle/cJSON/corpus/adapter、toolchain lock、
provenance、bootstrap implementation、CI workflow、Checked-C backendを変更していない。
parser/lexer grammarも変更しない。allocation/lifecycle/topology executionや次のgateへ進まない。
PRは独立Coordination reviewを待つOPEN / unmerged状態で停止する。
