# P628 / USER-CLI-2: original Pair after ordinary less-than Copy IF

Fixed starting main `9c2f2bd928fee00ac34266619c6eba65899f475a`, canonical Draft17.32.
Independent inputs/oracles were written before the baseline run (2026-10-19
17:20:32 JST); their SHA256s are in `frozen.json`. `baseline-freeze.json`
protects the unchanged pre-edit manifest, observations and original control C.
Those inputs and predictions remain intact, including the calibrated negative
below. No pre-baseline Git commit is claimed.

## First measured boundary and repair

Unchanged ordinary `newlangc --check` accepts TRUE/FALSE/one-Copy-alias sources.
Emission exits4 / `V1-BACKEND-UNSUPPORTED`, zero C bytes. The actual retained
source-owned artifact already has the original Pair, both distinct IF arm
worlds, real `<` operands, unknown joined Copy, original fixed fields,
Change/Reset facts and ended write loan (`evidence/baseline-owned.json`).

`copy_if_pair_frame` omitted `NL_CHECKED_USIZE_LESS_THAN` from its node census.
The one-line Production addition retains `boolean_scalar_at`'s source/operator,
operand and truth authentication, the two-arm identity checks, original root /
field / incarnation / provenance checks, original C carrier and ended loan.
No emitter expression, public API/issuer, parser or semantic rule changes.

## New observed composition

| Source | Real operands | Bool | Original left / returned old | Updated original left | Original right |
|---|---|---|---|---|---|
| pair_true | 17 < 23 | true | 27 | 91 | 201 |
| pair_false | 29 < 23 | false | 27 | 13 | 201 |
| pair_alias | 19 < 31, one extra bool Copy | true | 27 | 91 | 201 |

The ordinary emitted C, without observation edits, runs as unit main (exit0).
A separate passive observer only reads the actual live guest root/field
addresses and values, source operand carriers, real bool, selection, scoped
ref and old/result reads. No guest object/value, permission, Domain or chosen
branch is supplied by the observer. GCC13 and Clang23 strict C17 O0/O2 pass
both forms; stdout/stderr are empty. Native exit0 is not the guest field value.
Generated originals and local native records are retained under `evidence/`.

Public probe uses accessors on real registered source after AST/source teardown:
root3, children4/5, incarnations3/4/5; true/false root fact3→10 and left4→9,
sibling5 unchanged; alias root3→11 and left4→10. Both arms share the original
entry identities, the joined value stays unknown, the scoped write ref is
ended and points to original left. No explicit Allocation/Domain is issued:
this is a lexical Copy Pair, not heap/nonCopy ownership. Final ended roots are
not described as live poststate.

There is no newly introduced public Pair/IF validator. Public source-owned
observations are distinct from existing driver-private validation tests:
`p517_gate_test` rejects71 identity/arm/field/scope corruptions per source with
worlds unchanged and zero driver allocations; `p626_comparison_gate` rejects24
comparison corruptions (alternate same-value binding is NOT PRESENT here).
Five synthetic C observer controls (wrong comparison/direction/field/old/sibling)
exit91 at both optimization levels. These are observer checks, not NewLang
semantic rejections or independent Red Team evidence.

## Negative, failure and cleanup boundaries

- `wrong_quantity.nl`: CLI3 semantic `USIZE-LESS-THAN-TYPE` at5:22–5:23;
  real registration failure rolls back, artifactNULL, no C output.
- `readonly_write.nl`: frozen prediction was a semantic write-authority error;
  measured first refusal is **parser** `LOCAL-LOAN-PROFILE` at7:34–7:35,
  error category `unsupported`, CLI3, no C. Semantic checker is NOT ENTERED.
  Its unchanged parser state/artifactNULL does not prove semantic write refusal.
- `complex_arm.nl`: CHECK0, backend CLI4, no C; no native reached. This retained
  complex-arm profile boundary is the next measured limitation; not generalized.
- Every positive emits identical bytes on repeated ordinary CLI calls.
- Reused real registration/main OOM injection rolls back all failed prefixes:
  TRUE/FALSE registration243/main138; alias registration253/main145;
  failed owned outputNULL. Four new CTests cover integration plus these3 OOMs.
- Unchanged P521 == Pair update, P626 pure `<` IF and README Node controls keep
  their exact C17 bytes and run natively. Full regression includes adopted +/-.

Test harness calibration before publication: the first local GCC full sweep
had337/338 pass because its new read-only-negative assertion incorrectly demanded
semantic rollback after the parser had refused. Corrected only that test to
assert `semantic=NOT_ENTERED`; frozen source/oracle and Production fix unchanged.
The final GCC14 and Clang23 frontend suites both pass338/338 (LLVM-free /
PARTIAL); final targeted tests pass4/4. Original local failure log is preserved;
fixed-HEAD remote CI results are reported separately in the PR.

## Reproduce

```
CC=gcc cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
ctest --test-dir build -V -R '^p628_'
./build/newlangc --check tests/p628/pair_true.nl
./build/newlangc tests/p628/pair_true.nl > /tmp/p628-original.c
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 /tmp/p628-original.c -o /tmp/p628-original
/tmp/p628-original
```

Frontend-only local suites are PARTIAL, not LLVM FULL5. Unmodified workflows
supply the single natural PR FULL5/PARTIAL pair; no CI rerun is authorized.
Production changed root/field composition acceptance, so Coordination decides
independent R/F needs before adoption. No merge, new authority, general CFG,
heap ownership, general IF effects, library/Product GO or H1 claim.

Compass / current Roadmap / ProductStrategy1.3 / Process4.2,4.3.1:
selected v0 mechanisms compose into actual original lexical state mutation.
Historical gate N/A for faithful backend consumption (KEEP selected Copy/IF,
fixed fields, scoped loans and replace/Change/Reset; DEFER new source/issuer /
CFG/owner mechanisms). No distinct goal deviation identified. This finite
technical capability does not establish ordinary-safe library audit savings.
