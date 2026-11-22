# P934: two lexical Copy originals and a pure two-normal unit IF

Base `fbbf8a0261669ce0e8920e5b019134268a6a58c1`, Draft17.34 §27.4.
Read-only [Gate A](https://github.com/wakairo/NewLang_Compiler/issues/934#issuecomment-6409264998)
preceded branch/implementation/CI. The unchanged ordinary CLI accepted four
P929/P930 programs but rejected C17: both constructors passed the backend,
then IF admission incorrectly required an effect-field view. With no Change,
that view is legitimately absent. This was not a public validator failure.

`src/main.c::pure_unit_if` is a backend-only predicate for this narrow family:
two source-issued literal-u8 nominal constructors, one statement IF, final
unit, two actual owned empty normal arms with explicit unit tails. It checks
parent/match/order/context-origin lineage, same inherited current frame,
registered constructor/package/child correspondence and the existing bool
operand checks. No source/field-name hardcode, semantic-ID constants, new
source reparser or test expected values select admission. The existing renderer emits the two distinct C
originals and the actual comparison/branches. Existing effect/return proof
validators remain required for their separate families.

Only three code/test paths change: main.c, the current p930 CLI regression,
and native.py. CMake/issuer/public API/checker/backend fallbacks/toolchain/spec
remain unchanged. Historical P929/P930/P932/C55 sources, first oracles and
failed results remain immutable. The active CLI test prospectively expects
new C17 success; it does not rewrite historical EXIT4 oracles or run a hidden
old compiler. The same CTest additionally invokes the new native regression.

`freeze.json` was written before the changed backend ran, SHA256
`b1c4b9b35221836673ba01899aa799fc00df3c6070d05220cd6d5b20861815f5`.
Original four positives plus new PacketQuota and TemperatureBand exercise
different declaration/initializer orders, TRUE/FALSE for equality/less-than
and u8 edges0/255. Both new sources CHECK0/C17 EXIT4 on unchanged main, as
predicted. Independent expected fields/branch are never guest assignments.

First changed-backend GCC13 focused tests:4/4 PASS; full frontend474/474.
Available local Clang23.1.2 ASan/LSan and UBSan:4/4 each. These are local
preflights; pinned Clang23.1.3 belongs to the independent CI matrix.
All first logs and exact generated C bytes/SHA are retained. CMakeCache gives
the test the actual configured compiler and sanitizer; all native builds use
strict `-std=c17 -Wall -Wextra -Wpedantic -Werror`, O0 and O2.

For every source, unmodified C17 is compiled/run separately from a passive
observer copy. The observer retains the SAME declarations/initializers and
reads original field addresses/volatile values inside the owning function
before/after the actual IF. Only its own branch marker changes. It never
modifies guest fields or dereferences a pointer after local lifetime ends.
Bare O2 may eliminate unobserved locals; its exit0 is not address evidence.
Observed builds materialize two distinct originals; current native hosts
report size2 and f1 offset1. Internal C layout is compiler-owned, not a new
language/FFI ABI. Native witnesses and raw generated C are explicitly distinct.

Malformed fields/condition, Copy-as-ref and wrong-domain controls stay EXIT3.
The stateful two-normal CALL case remains precision `P6-JOIN-PRECISION`,
CHECK3, not a semantic invalidity claim. A closed-loan+pure-IF program remains
CHECK0/backend4 outside this finite profile. Independent existing foreign H
as T destructuring is also checked separately: semantic3 on both baseline
and candidate. No arbitrary active loan/ownership/effect join is admitted.

Existing p694 fault driver exercises the real new named-output path: flush,
close, commit failures and382 reached wrapped allocator failure positions
preserve the previous output, remove temporaries and recover. New predicate
allocates nothing; this is not exhaustive OOM or a new Domain/Allocation test.
Deterministic C bytes, initializer order and output rollback are checked.

CI must be recorded in Issue934 at the immutable DRAFT candidate. No merge,
manual retry/rebase/repair campaign or automatic R startup. Future empty,
nested, nonempty, stateful or authority-rich IF profiles need their existing
contracts or separate work; this is not general CFG/SSA or full v0 coverage.

Goal Alignment: ordinary CLI gains a genuine source composition step, while
no public rights, rich lifetime bridge or F72 refinement theorem is created.
Author+ordinary-safe-caller UB audit savings versus well-factored C remain
unmeasured. Product GO PAUSED, H1 HOLD and independent R histories unchanged.
