# P412 — unchanged compiler public join boundary: NO-CODE FEASIBILITY HOLD

Track: P. [Issue #412](https://github.com/wakairo/NewLang_Compiler/issues/412).
**Test-only DRAFT / DO NOT MERGE. No native feature implementation.**
Fixed main and prepared GitHub branch `p412-bounded-node-join-c17` initially
both equal `adc63b64b656b3d6e0501f29eaeee5181927dc14`.
CURRENT_SPEC selects Draft17.30; FormalProof/main
`a8de4ebb4099f4742fc161d26867740dcac48414` is not proof of a native lowering.

## First barrier and bounded decision

The existing lexical Node switch emitter is already sufficient to represent
runtime None/Some and scoped writes mechanically. Its first explicit refusal
for the admitted changed match is `checked_c_node.c:490–493`:
`!v->normal_frame_unchanged`. Removing that guard alone would not establish
original-frame mapping/currentness.

The published public contract (`checked.h:116–120`) attaches ancestor C-carrier
mapping to the unchanged normal frame proof. Arm accessor documentation
(`checked.h:278–281`) explicitly qualifies semantic IDs by each separately owned
hypothetical world. The compiler sets `match_binding_prefix` before arm checking
(`semantic_check.c:5789–5794`); its nonzero value alone is not a changed-frame
certificate.

Selected `node_memory_join` proves the original common entry internally, applies
parent-owned fresh unknown Option/field/root facts, and records only normal arm
count/type in the match (`semantic_check.c:5358–5368`). There is no public Node
join-specific entry/post accessor or validator linking that original frame and
its per-arm writes to the abstract continuation. The public packet/producer/
custody/captured certificates serve other profiles. Original parent context
views at the end of the function are not a match-entry snapshot.

Preflight therefore stopped on the Issue's existing-public-proof condition:
**NO-CODE FEASIBILITY HOLD**. This is a consumer evidence-contract gap, not a
semantic-invalid source verdict, unsafe-acceptance finding, formal impossibility
proof, or proof that every possible bounded public-trace validator is impossible.
A new native path cannot be justified here by equal numerical IDs, a prefix,
structural similarity, copied pointer bits or a private checker-state import.
No full common-origin reconstruction/validator was invented in this task.

A possible future smallest target is a separately authorized, bounded read-only
original-Node join certificate/validator exposing or verifying common entry,
arm lineage and field/parent/occurrence post correspondence. It need not select
new semantics or imply general CFG/SSA/heap machinery, but the Issue prohibits
public API changes. Its engineering/soundness cost has **not** been measured.
Coordination decides whether that boundary or another proof route merits work.

## Real unchanged-main reproduction

The test-only probe uses public parse/register/check and checked/value APIs;
original syntax is disposed before owned inspection. It includes no internal
header, host seed, fake authority or mutable checked-artifact control.
The requested six unchanged fixtures show:

- SOURCE/registration/CHECK/source-issued owned bodies and both arm snapshots.
- Two normal changed-match arms, `normal_frame_unchanged=false`, nonzero prefix;
  separately owned worlds, real arm scoped replace with field AND parent facts.
- No packet entry/post world, captured post world, or matching existing packet/
  custody validation for this Node profile. Those false validators are **not**
  Node semantic rejection or exhaustive proof that no public method can exist.
- CLI4 `V1-BACKEND-UNSUPPORTED`, C0; `nl_checked_c_node` unsupported, NULL output
  and unchanged caller length sentinel. Public semantic snapshots are unchanged.
- C17/NATIVE for the new changed join **NOT REACHED**. Runtime arm coverage,
  final physical pointer identity, link mutants and native sanitizer execution
  for that new feature are **NOT TESTED**.

Full source hashes, match byte spans and actual field/parent/occurrence IDs are
in [unchanged-public-boundary.json](evidence/p412/unchanged-public-boundary.json).
It is diagnostic data, not an authority certificate. The Source/CHECK/owned
facts do not replace the missing native-consumer proof.

Local clean GCC Debug: **264/264 PASS, 28.72s**: original 263 plus one diagnostic
reproduction suite. An initial test-only selector incorrectly included the
saved-copy fixture's earlier payload-extraction match; it was narrowed to the
actual two-normal changed match. Production behavior was not repaired/changed.
Existing source negatives, old occurrence/dependency/owner/private-ID controls,
registration384/call235 OOM/rollback and P404 native scalar tests remain original.
Fresh dedicated-branch Actions five configurations are cited in final PR/Issue
metadata; they validate an **unchanged compiler and added test**, not a new native
implementation.

## Reproduce

Use existing README bootstrap/toolchain and normal configuration:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 tests/p412/reproduce.py build/newlangc \
  build/p412_public_boundary tests/fixtures
```

All branch changes are the test-only executable/script, its eight-line CMake
registration and this diagnostic report/evidence. `src/`, `include/`, fixtures,
README, specification, workflow and flags are byte-identical to fixed main.
No unmerged experimental/R-only code imported. Branch fetched through the
configured GitHub connection, not an alternate DNS destination.

## Gates and STOP

Compass → Product Strategy/Coverage Map and Process §4.3.1 Exploration GO checked;
Product GO remains HOLD. Historical §4.2: N/A, read-only assessment and mechanical
test reproduction; selected §§13.5a/27.3 unchanged. No Ledger/semantic/API choice
was made. R402 BLIND INCONCLUSIVE/R405 calibrated owned PASS remain historical
and separate; this is P evidence, no new independent R or native/formal claim.

No merge, Track launch or authority expansion. H1's five fallible original roots,
6+4+2/B.prev=B/nonCopy B+dst/0–5/B1–B7 remain undecided/H1-only deferred; STB six
conditions remain HOLD. Record the first boundary and stop; don't rescue this
example by inventing a certificate, abstract Some, or new ownership mechanism.
