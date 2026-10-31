# P1055 / C144 local result — DRAFT HOLD

Five P-owned, genuinely source-issued one-CHANGE old-result profiles pass
CHECK, public body/parameters/event current, C17 and strict GCC13/Clang23
O0/O2. Binding tail, Copy-binding RETURN and the renamed/reordered
three-binding tail now reach C17. The two previously accepted RETURN forms
and 36 other parent samples retain their exact C bytes (38 total).

Tested implementation: `277ed621b8f171e40435b333299ad84510726dbe`, tree
`a2224a1e78cfe38bdfa58494f3530b1e3d0560f6`. Fixed parent PR1048:
`ade11ebec808f5a95e8831a7ae6ee6f6da2b0dbe`, tree
`70305183f8951038d296ac9a9ecae1f8e8750e4f`. Publication adds evidence only;
the PRE-RUN source/oracle/harness and Production bytes remain unchanged.

| Local condition | Existing tests | New profiles | Physical guest observations | Reached emitter OOM |
| --- | ---: | ---: | ---: | ---: |
| GCC13 | 578 | 5 (driver) | 30 | 2,023 |
| Clang23.1.3 | 578 | 5 (driver) | 30 | 2,023 |
| ASan/LSan | 115 related | 5 (CTest) | 30 | 2,023 |
| UBSan | 115 related | 5 (CTest) | 30 | 2,023 |

The original 578 CTest names are retained, with five ordinary new tests.
Local frontend inventory is 583; FULL additionally has `llvm.c_api`.
Both sanitizers used the unchanged 90s test policy. Each condition includes
the original P965 494 ownership cases and 502 reached CHECK/definition OOM
positions, P633/P817/P785/P790, historical freeze and P1045 runtime checks.
Local FULL LLVM `c_api` is untested; natural CI is pending at publication.

Every condition rejects all nine frozen source negatives, before native
execution: seven CHECK0/public-current0/backend4, two CHECK3/backend3.
Incoming, constant, alias and another actual CHANGE's old result remain
different origins even when numerically equal. Double CHANGE and the
unsupported after-closed-H boundary remain refused. Named output, inode
and no-temporary-file preservation hold for rejection and reached output
error; healthy retry reproduces the exact C.

Each condition measures both actual caller return bindings, live original
T field addresses/old/new/sibling, and original H pointers, allocation,
domain, LiveTail, terminal and exact cleanup across guest0/1/2 and O0/O2.
Raw exit0 alone is explicitly INCONCLUSIVE. All ten planned nonsanitized
defined-u8 second-helper-return mutants are detected at the actual caller
return check while their field stores/hooks remain correct.

Emitter OOM per profile is 399/403/403/407/411. Each condition has 1,923
OUT_OF_MEMORY and 100 conservative UNSUPPORTED responses; every injected
position is actually reached, publishes no C or length, preserves snapshot,
current/H origin/custody/owner, and retries to identical C. Per condition,
70 actual-current faults, 30 supplied-view mismatches, 20 H-origin faults
and five unrestored A-destroy/B-survive cases pass.

First unexpected local RED: ASan CTest invocation had a missing external
raw directory, so two Python processes failed before any compiler/source
execution. Raw stdout SHA256:
`ce8942be08237294dcf5f3d2d2e04505edc11b0efe0b6774d871bdc61eed6252`.
Only that directory was created; the same frozen candidate passed in a
separate recorded retry. No test, oracle or Production adjustment occurred.
The original parent RED, P1054 RED reference and all old v1-v9 evidence stay
unchanged. The first runtime private predicate in the uninstrumented parent
remains UNKNOWN; static shape exclusion is not presented as runtime proof.

`evidence/local-results-v1.json` records counts and exact command/stream
hashes. `evidence/local-v1.zip` preserves 1,770 raw files losslessly, including
each command's streams, full old/new CTest logs, generated C and the initial
preparation RED. Its internal `INDEX-SHA256.json` hashes every raw member.
ZIP SHA256: `80fc682e4836e0f901ec2405716f27854018849a863ae216b0b6ae3cbcb9ae3e`.

Self-review: one Production header, 83 additions/34 removals (net49).
The borrowed, decreasing-prefix origin walk is O(n²), O(1) auxiliary space,
with no allocation/cache/issuer/ledger/slot, source law/API/ABI/IR or H-world
change. Public current, actual/formal operands and the physical serializer
retain their existing checks. Prospective v10 retains all 2,053 v9 inputs
and original v1-v9 manifests; a 36-line test-only v9 anchor wraps unchanged
older history verifiers. Five new v9 and five forwarded old v8 coherent
rewrite/data-loss counterexamples are refused. This is not protection
against arbitrary replacement of trusted checker code.

VALUE-ALIGNMENT-RISK: origin inspection and frozen-evidence maintenance
increase review/re-audit cost. The author plus ordinary-safe caller's human
UB audit dependency closure versus factored C, performance and product cost
remain UNMEASURED. Alternative A, no change plus the supported-shape matrix,
remains the fallback; larger plan/IR/issuer mechanisms are deferred. KEEP
Draft17.34 / REPLACE NONE; historical design gate N/A faithful implementation.

No R1016/R1051 originals were accessed or reused. Their native result is
unverified. After-closed-H support, a third helper and general control-flow
chains are outside this release. P1036 STOP and PR1048/1050 DRAFT HOLD
remain. One new stacked DRAFT and one natural FULL5/PARTIAL group (six jobs)
are authorized; any CI RED stops further push/rerun. No READY/merge/Product
GO or independent F/R launch is authorized.
