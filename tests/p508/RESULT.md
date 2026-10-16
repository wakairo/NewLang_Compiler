# P508 finite source-derived Copy IF selection

DRAFT / DO NOT MERGE. Base `eacbd0fd84f3f9ef90813435ac71633a770f3f80`,
tree `8c6501ddff9dfb2b784a77b8259af8d5cbb16b64`; source/oracle freeze
`d07831c` (full SHA in Git history). Canonical CURRENT_SPEC→Draft17.30.
No P503/P506 implementation imported. Public witness is existing checked IF
and both owned arm views; no new public general IF/Pair validator is claimed.

## Before changes: first actual boundary

Original select_true/select_false and copied/renamed variants parse and CHECK
CLI0. Public source→registered unit→main() probe after original AST/input
cleanup finds both arm values7/8 and source bool1/0. Parent joined u8 and its
next Copy are statically UNKNOWN, not scalar-known7/8. Ordinary C17 stops CLI4,
V1-BACKEND-UNSUPPORTED, zero output. baseline.json records exact fixed-main
build and binary SHA; baseline-pr507-reference.json is a separately labeled
same-Production-tree reference run, not substituted authority. Pre-edit owned
logs and every immutable source SHA/oracle are retained.

No grammar, semantic join or bool producer change was required. Existing
source_if already implements the selected flat-Copy result join. Missing path:
ordinary driver accepts no IF initializer in the u8-binding C17 spine.

## Bounded implementation and trust boundary

Exactly one root-block u8 result binding, one IF, two normal pure u8 arms.
Each arm has only connected BLOCK→literal or outer Copy→IF_ARM, no statements,
local declarations, calls, loan/ref, effect, roots/fields/owner/raw allocations,
nested IF/loop or arbitrary CFG. Condition is a source-named Copy of a genuine
root-bound usize equality (bounded Copy aliases allowed). Scalar root prefix,
current facts/incarnations/carriers, scopes, private result IDs, source spans,
original declaration spelling and core types are checked independently against
both actual arm contexts. All active scalar dependency/authority fields must
be empty. No Context replay or new retained world snapshots.

A source pointer alone cannot authenticate an arm from a second call using the
SAME retained plan and identical numeric IDs. Minimal private issuer metadata
(parent artifact, IF node, arm index) records exact membership at existing fork
creation; consumer checks these anchors AND recomputes the source graph and
packages. No approved flag, runtime address/root grant, public API, semantic
rule/guard, ownership registry or allocation is added. Three borrowed fields
introduce no owning cycle; old source_if checks and cleanup remain unchanged.

Driver verifies source literal spelling/cache/package consistency, both arm
ownership and complete scalar prefix before normal C17 emission. Actual output:

```c
const _Bool nl_local_3 = (nl_local_1 == nl_local_2);
const uint8_t nl_local_4 = (nl_local_3 ? 7 : 8);
const uint8_t nl_local_5 = nl_local_4;
```

Renamed case emits `nl_local_6 ? nl_local_4 : nl_local_5`, then copies that real
selected result. No emission from cached bool truth or unknown parent value0.
The previous native-size_t width static_assert is preserved. Only scalar pure
selection is enabled: no original Pair memory-state phi or physical Change.

## Actual source-owned and native observations

Both original sources differ only in b=2 vs3. TRUE observes operands2/2,
bool1, actual selected7 and Copy7. FALSE2/3, bool0, selected8 and Copy8. Both
owned normal arms remain7/8 even when condition is statically known. Renamed
conditions/outer u8 values preserve the same distinction. Parent joined value
remains unknown in public semantic views. Snapshot scopes/domains/backing/
occurrences are0; this does not deny ordinary lexical u8 root identity.

Strict GCC14.2 and Clang23.1.2 generated-C compilation, C17 Wall/Wextra/
Wpedantic/Werror, O0/O2: four sources × two compilers × two optimizations.
Unmodified guest and passive read-only observer:32 native executions exit0.
Separate corrupted-C chosen arm/direction/injected constant/copied carrier:
64 executions exit91 with the SAME observer. Observers add assertions/read
existing local carriers only; never write guest variables or replace algorithm
values in a passing run. Original C digests/bytes recorded unchanged in evidence.
These local runs are not the pinned23.1.3 FULL gate; natural CI reported separately.

## Malicious evidence, OOM and regressions

Driver-private mutations of actual owned artifacts, not host-generated success
worlds: literal families79 rejects each; outer-Copy families77 each,312 across
four programs. Wrong/duplicate/swapped/foreign arms; wrong IF/condition/source
span/result/type/package; private ID/fact/scope; wrong old cached scalar and
coherently changed literal+package versus real source; missing comparison
source; wrong copied value and invented reference/allocation/dependency bits.
Foreign cases include separately loaded source AND second actual check sharing
same retained source plan (fork value-free registered program before main,
no host scalar/root/authority seed). Exact parent anchors reject both.

Each driver rejection leaves observed artifact/node/context bytes unchanged;
mutant restored, original revalidates. Accepted gate has zero malloc/realloc/
calloc even when first wrapped allocation would fail; complete public snapshots
remain unchanged. Actual issuer/registration OOM152 and main()96 failures before
first success: OUT_OF_MEMORY, NULL checked output, public rollback, retry succeeds.
No new allocator work in lineage metadata or C gate.

Frozen source negatives:

- wrong_condition: semantic P13-IF-CONDITION.
- known-true invalid else: semantic P3-WRITE-REF-REQUIRED; other arm still checked.
- ref_escape: semantic P8-EXIT-DEPENDENCY.
- noncopy_arms: unsupported ALLOCATED-SOURCE-PROFILE BEFORE ownership join;
  not a nonCopy safety proof or parser rejection.
- field_roots: SOURCE/CHECK0, C17 CLI4; legal broader Copy reads are outside this
  finite scalar-only emitter profile, not mislabeled canonical-invalid.

Old P497/P504 sources, expected diagnostics, unit-IF backend STOP and original
Pair P6-precision STOP remain untouched. Local full frontend-only297/297 passed,
with final targeted checks after test-only snapshot assertions. Runtime/source
checker and backend tests ran; this is not only a host semantic model.

During test construction a malformed C macro caused a failed test-target build;
stale-binary results were discarded, clean build+tests used. Parser owner-slot
reset and before-main registry fork corrected test harness mistakes; immutable
NewLang source/oracles were never repaired after observation. No such harness
failure is reported as compiler/source failure.

## Reproduce and limits

```
cmake -S . -B /tmp/p508 -DNEWLANG_FRONTEND_ONLY=ON
cmake --build /tmp/p508
ctest --test-dir /tmp/p508 --output-on-failure
/tmp/p508/newlangc --check tests/p508/select_true.nl
/tmp/p508/newlangc tests/p508/select_true.nl > /tmp/p508-true.c
cc -std=c17 -Wall -Wextra -Wpedantic -Werror /tmp/p508-true.c -o /tmp/p508-true
/tmp/p508-true
python3 tests/p508/native_test.py /tmp/p508/newlangc /tmp/p508/p508_public_probe \
  /tmp/p508/p508_gate_test cc tests/p508 none
```

Only one default-on pure-u8 IF result in an otherwise scalar no-arg unit entry;
no inline comparison condition, arm-local statements/bindings, nested IF/call,
Pair/field mutation, ABI, arrays, owner, general CFG/effects or LLVM expansion.
Whitespace-only checked IF gaps; comments there remain outside finite C17 gate.
Borrowed checked views require owning artifacts/context alive and matching the
issued state; old artifacts after mutating their parent context are not proofs.
Canonical, public API, Node validator/normal_frame_unchanged, P497/P504 test bytes,
workflow/toolchain and existing PRs unchanged. One DRAFT and one natural CI pair.
Final CI URLs/results/head/tree are recorded in PR/Issue, no prediction here.

Goal: reusable real source quantity guard→Copy choice, a technical Exploration
rung. Evidence: SOURCE/CHECK/public owned/ordinary C17/native in tested boundary.
Alternatives: C/Zig/Rust already express scalar conditionals; a meaningful
library author+ordinary caller UB-audit/re-audit closure improvement remains
UNMEASURED. STOP: no expansion beyond the validated pure Copy selection;
Coordination owns adoption/R and future investment. P506 public structural Pair
proof remains HOLD and physical original Pair IF is a distinct later gate.
No merge, self-close, other Track or H1/STB/Ring/Product GO claim.
