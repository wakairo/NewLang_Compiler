# P679: ordinary two-field Copy-old forwarding

Authority: Compiler/main `c3963b625e9e91a1de08318bdf153019a3a8c99c`, canonical
Draft17.33. This task changes **no compiler, public API, issuer, validator,
canonical rule, existing negative oracle, workflow or toolchain**.
Historical gate §4.2: N/A — unchanged-spec source/example/test-only exercise.
Compass, actual Value Strategy §1.1 and current Roadmap distinguish ordinary
CLI capability from UB-prevention audit locality and Product GO.

## Frozen operation and first measurements

`freeze.json` was written before program execution. The standalone source in
`../../examples/device_settings_swap.nl` and both invalid variants are exact
copies of those frozen bytes. It is the fresh DeviceSettings contract from
Issue679; PR395's source/oracle was not read or reused. Existing P671 and R675
single-write successes are scope context, not this two-write result.

Declared fields are port(index0), mode(index1); constructor labels are reversed.
The original record starts `[7,31]`. Reading port saves7. Writing saved_port to
mode returns old_mode31 and leaves the disjoint port7. The second independent
scoped write passes **that actual returned Copy binding**, writes port31,
returns old_port7, and leaves mode7. Final reads are `[31,7]`.

Stage0 used the unchanged normal `newlangc` in a local P Work/Codex checkout:
parser/registration/CHECK/actual main() owned all succeeded; ordinary CHECK0
was silent; C17 CLI0 emitted1564 bytes, SHA256
`cd4f22d82e5107286cb2735fec224fc9ddb7f716c468fe4112f3ab3f06b61a4c`.
GCC13.3.0 and retained Clang23.1.2 O0/O2 passed original and passive-observer
strict C17/native. The separate C comparison also passed, without being used
to initialize or replace any NewLang guest state. Baseline main push FULL5
39525359037 and PARTIAL39525358922 were verified all-success/attempt1.

The positive has SHA256
`8e2703e6eb736a438aab15f4b25835eb8470724dde697000e0a714510e47290f`.
Unknown member has `6cc43ef78ddfd4d8d8fcc8fca84b419152b4c2b74a3061b6f22351fff69dde17`:
CHECK/C17 CLI3, semantic FIELD-UNKNOWN-FIELD, line4:22–4:36, zero C.
Wrong second input type has
`63939359fe87124e4e94e7974069e90fcb29a9bb00491fc2d2b825579ba11052`:
CHECK/C17 CLI3, semantic P3-TYPE-MISMATCH, line6:61–6:70, zero C.
These are invalid sources, not parser/backend coverage failures. No alternative
positive source or expected value was substituted after measurement.

## Genuine owned and physical evidence

`public_probe.c` parses and registers the real source, destroys its AST and
checks actual main(). It only reads public retained-body/context APIs:
original nominal10/root1/incarnation1, dependent port child2/mode child3,
write indices1 then0, parent ValueFacts1→7→11, mode3→6 and port2→10.
The first disjoint port fact survives until its write; final mode still has
its first-write fact. Scopes1/2 are separate, closed, nonescaping write loans.
The first returned old package is bound, then an actual Copy identifier of
that same binding supplies the second Change; equal integers alone are not
accepted as causality evidence. Root/children are ended and current facts
cleared after normal lexical exit. No heap Allocation/Domain is created.

`native.py` connects the actual owned symbol/field identities with this emitter's
C carrier mapping, discovers C temporary names from the emitted declarations,
and verifies ordered saved-read→first store→returned old binding→second actual
store→post reads. A literal second store is refused by the observer. Passive C
hooks are inserted **after original initializers**, capture actual addresses,
read volatile-qualified lvalues, and assert values before/after both writes.
They never supply guest values, overwrite guest fields or create permissions.
Original and instrumented guests run at O0/O2; an uninstrumented exit0 alone
is not physical/value observation. This is test observation of authentic public
owned plus the unchanged private C17 gate, not a new independent integrity
validator or universal authentication proof.

## Reproduce

Use README's Ubuntu24.04 frontend-only GCC13/CMake>=3.28,<4/Python>=3.12 setup:

```sh
CC=gcc-13 cmake -S . -B build-frontend -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-frontend --parallel 2
./build-frontend/newlangc --check examples/device_settings_swap.nl
./build-frontend/newlangc examples/device_settings_swap.nl > build-frontend/settings.c
gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror build-frontend/settings.c -o build-frontend/settings
./build-frontend/settings
ctest --test-dir build-frontend -R '^p679_' -V
```

The guest has `main()->unit` and prints **no values/stdout**. The CLI emits C,
not an executable. The regression's passive assertions demonstrate the actual
values and forwarding. Therefore this is a **USER-CLI-1 composition technical
subcheckpoint / partial rung**, not the full natural user-visible checkpoint
or Product GO. `comparison.c17` preserves the exact pre-execution C comparison
bytes (manifest's original name `comparison.c`); it is compiled as C by the
regression. Generated original C and first-stage logs are separate evidence.

## UB-prevention audit locus and remaining duty

The real source receiving operation issues the lexical root; existing fixed
attach creates its nonindependent children. Each source `loan_write` is scoped
to the actual child. Legal replace updates that child's and the ancestor's
current facts; the other field's value/fact is preserved. A future invariant
that depends on the whole parent must account for either legal field writer.
The example assumes no protected hidden invariant or representation privacy.

The factored C function receives a caller-provided pointer; C author/caller
manual duties include a live correctly typed writable object, nonnull pointer,
valid access/aliasing/lifetime and operation order. In this literal lexical
example those duties are small and directly inspectable. NewLang's retained
source/loan/currentness checks are capability/safety evidence, not a measurement
that an ordinary-safe library author+caller audit/re-audit closure is smaller
than well-factored C. No performance, zero-runtime-cost or burden advantage is
claimed. Heap, callbacks, FFI, reentrancy, general nominal ABI, private modules
and full safe-reachable API states are untested. No Product GO or H1 change.

New HEAD CI is one natural FULL5+PARTIAL pair, with no workflow or lock edits.
Exact final head, runs/jobs/attempts, sanitizer results and any initial failures
are recorded in the sole final Issue679 report. Local frontend-only builds are
PARTIAL; CI preserves the full pinned LLVM/Clang23.1.3 gate.
