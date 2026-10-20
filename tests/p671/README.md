# P671: two-u8 nominal fields — EXPERIMENTAL NON-NORMATIVE / DO NOT MERGE

This is a discardable global B trial authorized only by Issue671. Canonical
Draft17.32 still selects Pair-only direct lexical two-u8 field source. A separate
reviewed source amendment, Ledger synchronization and Coordination adoption are
required before any merge. No specification, public API, backend, workflow,
toolchain or main branch is changed here.

## Evidence boundaries and reproduction

Base: `a0de7a86d04d00d75a2484dfed4184ccb177e06c`; local P Work/Codex checkout.
See `PREFLIGHT.md`, `freeze.json`, `cross_nominal_freeze.json` and `evidence/`.
`../p670/**` is copied byte-for-byte from
`59b01a2afafc17061eda9ecb997beedd8e04ba84`. Its sources, expectations, CLI3
failure logs and historical Stage0 HOLD remain valid historical evidence;
P658 is untouched. This candidate does not relabel either prior task as PASS.

The two historical unit files are archived verbatim as `.historical.txt` files,
with SHA256s in freeze.json. Both full historical `negatives` groups exited0
before any compiler or oracle edit. The only old assertions migrated are
Other field reads in those two files. `fragment.h` checks actual source-issued
parent/children, nominal, indices, incarnation, facts, Copy value and unchanged
sibling. The shared context's later unrelated rejection oracles remain unchanged.
The new fragment probe additionally rejects whole destructuring into a distinct,
separately source-registered nominal with the same layout (`P5-DESTRUCTURE-TYPE`),
checking rollback. That is not multi-nominal normal-CLI evidence.

The compiler change is confined to `nl_fixed_type` in src/fixed_field.c:
complete nonrecursive nominal, exactly2 distinct declared core-u8 fields,
Copy+Discardable. Neither name nor label spelling grants authority. Existing
Node and Array handling stays ahead of this predicate; Array remains excluded
from direct named-field selection. No new source-mode flag, authority, ID,
registry, lifetime or visibility rule is introduced. Existing attach/selection/
Change/dependency/loan/end and backend predicates are reused without weakening.
Compiler diff: 6 additions / 5 deletions (net+1), no emitter changes.

Ordinary `newlangc --check` and `newlangc SOURCE` are exercised on the exact
frozen P670 PacketStamp, reversed RouteStamp and Pair control sources. Each
`public_probe` loads actual source, registers it, destroys its AST, checks actual
`main()`, then observes the retained owned body through public APIs. No fixture
semantic object, pointer, authority or value is seeded by the host.
The existing private `v0_validate_call`/bounded-field gate is exercised by normal
CLI C17 emission without bypass. This is public-view observation plus existing
compiler validation, **not a new independent public fixed-field validator or a
proof of generalized integrity**. No such public validator is added.

```sh
cmake -S . -B build-p671 -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p671 -j2
ctest --test-dir build-p671 -R '^p671_' -V
ctest --test-dir build-p671 --output-on-failure -j2
```

`NEWLANG_FRONTEND_ONLY` is local PARTIAL evidence; the natural PR FULL5 uses the
unchanged curated LLVM23.1.3 lock and a separate PARTIAL workflow. Local retained
GCC13.3.0 and Clang23.1.2 are explicitly distinguished from CI pinned Clang23.1.3.
`native.py` compiles unmodified generated C and a separate passive observer using
strict C17, warnings-as-errors, O0/O2. It observes actual original carrier and
field addresses, actual replace temporary/old Copy result and current/sibling
values. Observer C only captures addresses/reads/asserts; it never initializes
or writes guest state. Plain exit0 alone is not counted as value observation.

## Actual dependency / audit-locus map

| Locus | Actual issuer and dependencies | Checked legal transitions / obligation |
| --- | --- | --- |
| Source nominal | Real declaration registry, actual nominal TypeId10 separately in each program | Labels resolve declaration indices; layout equivalence does not equate nominal identity. |
| Original lexical root | Ordinary source receiving/binding, root Place1/incarnation1, implicit local identity | No heap Allocation or explicit LifetimeDomain is issued or claimed. Root end clears current facts and dependent children. |
| Fixed children | Existing nl_fixed_attach, child2/incarnation2/index0, sibling3/incarnation3/index1 | Both nonindependent under original parent/incarnation. Source-issued evidence and currentness, never C offsets, justify access. |
| Write scope | Source loan_write on actual child2, scope1/ref binding2, write access/nonescape | replace through the scoped ref is a legal checked writer. Scope must close before exit; escaped ref is rejected. |
| Change / Copy result | Existing fixed replace, field fact2→5, ancestor fact1→6, returned actual old Copy binding3 | No incarnation change. Both later field reads use new parent fact6; changed field uses5, untouched sibling retains3. A legal sibling writer can also change parent fact; future authors must not assume a protected aggregate invariant from a Copy value. |
| Teardown | Source normal lexical end and existing detach/end | Final root/children are dead; final facts and child list are cleared. Retained checked nodes hold point-in-time evidence, not currently live authority. |
| Failure | Actual registration semantic failures and injected malloc/calloc/realloc failures, main() issuance | Transactional snapshots remain unchanged; failed artifact is NULL; CLI emits zero C on refusal. |

PacketStamp observed old/new/sibling **115 /126 /43**, RouteStamp **54 /97 /133**;
constructor order differs from declaration order in both. Pair **212 /6 /89**,
and its C17 SHA remains
`d5709a9524e6bd7e7faa71cb615def82f00d92737d2ec2b12374848d890f5058`.
All source hashes stay in original P670 freeze.json. Generated C files and new
results are separate evidence in this directory.

Unknown label, wrong initializer, wrong replacement and scoped-ref escape are
semantic errors. An out-of-scope old binding fails the finite lookup profile
(`FIELD-PROFILE`), not a claimed stale-root semantic proof. All refuse at
registration, exit3/zero C; the rollback harness verifies unchanged state.
Actual normal root ending is observed in the owned body. Broader stale-root,
ptr/ref-base and conflict controls remain in unchanged existing regression;
no new nominal stale-authority integrity theorem is claimed.

Callbacks, FFI, reentrancy, heap allocation/ownership, multiple nominal ABI,
private representation and hidden invariant protection are NOT TESTED here.
The compiler/issuer/backend remain trusted for issued evidence; a future
library author still owes invariant/writer/error/teardown closure, and a caller
still owns application policy. The two examples expose mutable u8 fields and
claim no hidden invariant, so no observed safe-writer counterexample to such an
invariant is being concealed.

**CAPABILITY:** finite candidate ordinary source→CHECK→owned→C17/native.
**SAFETY:** finite rejection/rollback/OOM/regression/sanitizer observations only.
**PRODUCT:** audit-locality improvement versus factored C and ordinary-safe API
closure are unmeasured; no Product GO, H1 PASS or representation hiding claim.
**BURDEN:** fewer source name remappings is usability capability; human
UB-prevention audit/re-audit scope reduction and author/caller costs are not
measured. Test/maintenance cost is retained explicitly rather than claimed zero.

## Test-instrument correction

The first new observer incorrectly required live current facts/child lists to
remain after lexical end. It failed equally on the unchanged Pair control.
Existing `nl_fixed_detach` clears these; the corrected observer requires ended
roots/children/facts0 and checks retained point-in-time pre/post read evidence.
The first failure and explanation are preserved in
`evidence/observer-harness-correction.json`. No compiler change or expected
program value was made to fix this instrumentation error.

## Local disposition before the one candidate CI pair

Same compiler patch: GCC13 full regression **364/364**. Initial overlapping
Clang full run **363/364**, ASan/LSan **362/364**, UBSan **363/364**;
failures were only existing p517 native and (ASan) p633 native 90-second CTest
timeouts under oversubscribed 2-CPU execution. Initial logs are retained, not
rewritten as first-pass green. After all overlapping suites finished, isolated
unchanged failed cases passed: Clang1/1 (16.98s), ASan2/2 (49.83s), UBSan1/1
(25.42s). No timeout/expectation/compiler fix was applied for these replays.

The final stronger test-only observations (including distinct nominal rejection)
and both migrated historical negative groups pass **9/9** on GCC13, Clang,
ASan/LSan and UBSan. Local full runs preceded the final test-only observation
strengthening; exact prepared HEAD receives the natural Actions pair.
Each novel source passed registration OOM injection at146 failure points and
actual main() issuance at60 failure points, unchanged snapshots/NULL failure
artifact. ASan leak detection and UBSan halt-on-error stay enabled.

The sole final Issue671 comment records exact candidate SHA, natural FULL5 and
PARTIAL run/job/attempt results. Those later CI results are deliberately not
committed by a second push or folded into these historical local logs.
