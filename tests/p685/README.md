# P685: ordinary zero-formal Copy helper into a lexical field write

Base `eabae4b5e0e352d00ea1a770abae30a25e34befb`, CURRENT_SPEC Draft17.33.
Historical gate §4.2: N/A, faithful implementation of existing §§18/19/24.
KEEP the P399/P419 typed-return/body ownership gates and P679 field path;
DEFER general ABI, I/O, entry/status, heap and effect/CFG mechanisms.
PR684 remains a separate archived experiment; no code is taken from it.

`freeze.json` was written before execution. `pump.nl` uses a forward-declared
zero-formal tail-return helper producing17, updates declaration field0 from5,
and preserves sibling83. `sensor.nl` uses different names, reverse constructor
labels, an explicit return through a Copy local producing53, updates field1
from4 and preserves sibling9. Both use the same normal compiler path:

```sh
./build-frontend/newlangc --check tests/p685/pump.nl
./build-frontend/newlangc tests/p685/pump.nl > build-frontend/pump.c
gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror build-frontend/pump.c -o build-frontend/pump
./build-frontend/pump
ctest --test-dir build-frontend -R '^p685_' --output-on-failure
```

The guest returns0 and prints no numeric output. A separate passive test copy
observes values; this is ordinary function composability, not a user-output
API, full USER-CLI-1 or Product GO. The examples have lexical roots, no heap
Allocation/explicit Domain, opaque modules or safe-library guarantee.

## Shared implementation boundary

Only src/main.c changes in Production (+23/-11). Zero-formal unit/u8 signatures
still need the existing registered function/body owner/source/context checks.
The actual result type chooses the existing scalar-body gate; tail or explicit
Return package must equal the source-generated call result. The unchanged
finite Copy-body checker and two-scalar-frame bound remain. Codegen uses the
same actual callee and nl_call carrier, omitting a nonexistent formal and
emitting a uint8_t(void) signature when the checked result is u8. The existing
caller binding and field/loan code receives that runtime return variable.
There is no new field/nominal/write-count predicate or source-spelling gate.

The CLI entry is explicitly kept unit-only as before: nonunit_entry.nl
CHECK0 still emits backend4/zero C. No status transport or special flag exists.
Richer callee IF remains CHECK0/backend4; invalid result/write types are CLI3.
All admission runs before output. Existing broken FILE I/O behavior is unchanged.

## Evidence and test scope

public_probe.c uses public APIs after source AST destruction, following actual
helper return/call ValueId into the incoming binding, Copy identifier into the
write, old returned Copy, original root/incarnation/child facts, final read and
sibling, closed loan and ended locals. No context/package/permission is seeded.

native.py compiles exact unmodified compiler output, plus a **separate passive**
copy. The copy only adds addresses, volatile reads and failure assertions after
actual initializers/calls/stores; it supplies no guest values. It observes the
actual C helper result -> receiver -> original field store, old result, final
value and unchanged sibling at O0/O2. Literal substitutions for call/store are
refused by that observation harness, not by a new public proof validator.
The private gate test reuses v0_validate_call, rejects six genuine-artifact
corruptions per program and restores/revalidates; the gate allocates0 and leaves
the public semantic snapshot unchanged. Real registration and main-call OOM
fail-at-N tests check rollback and NULL failure artifacts for both sources.
These are P-owned finite tests, not independent R or universal integrity proof.

Three old active regressions expected these already legal zero-formal calls to
remain backend4. Their unchanged source now correctly emits C17/native. Only
those active expectations were migrated; original source strings/files,
p596 frozen.json, historical hashes, previous evidence and independent R
material remain untouched. The first full local failures are preserved here.
The unsupported store, richer body, semantic-invalid and unit-entry boundaries
remain negative tests. The initial frozen sources/oracles of this task did not
need calibration or change.

C can factor the same small helper and writable-object update very simply.
Manual C access/alias/lifetime and writer-order duties remain, but this small
lexical example does not establish reduced author+ordinary-safe-caller
UB-prevention audit/re-audit burden. Product GO remains PAUSED, H1 HOLD,
one visibility domain and R655/R675 evidence limits are unchanged.
