# P630 — HOLD, preserved arm-local IF feasibility evidence

**This branch does not implement or admit the feature.** Production `src/main.c`,
CMake, old tests/sources, toolchain and canonical are byte-identical to fixed main
`032cf60aabf4997e672bc3607cf53775d91ececb` / Draft17.32. No PR or Actions run
is claimed for the unapplied prototype. This is a durable bounded investigation.

## First measured source boundary

Independent inputs/oracles in `frozen.json` were frozen17:57 JST before baseline
and any Production edits. `additional-freeze.json` records both-local/outer-Copy
probes also frozen and checked while Production was still byte-identical to
main. All seven normal sources CHECK0; C17 exits4 / V1-BACKEND-UNSUPPORTED with
zero stdout bytes. No baseline native execution was possible for them. Semantic
negatives and two-local/nested unsupported sources retain their exact bytes.
`baseline-freeze.json` protects the original source/oracle/owned/control records.

Actual original Pair(18,209) gets83 for21<45 or24 for57<45; old18, sibling209.
Variants exercise THEN local, ELSE local, both arms independently using the same
symbol spelling/number, and a local initialized by an earlier outer Copy.
Pure IF without Pair has both truth controls. Node/== Pair/pure < IF/no-local
Pair controls emitted unchanged original C17 on fixed main.

The first Pair failure is `copy_if_pair_frame`'s entry binding/place census:
the actual THEN arm has bindings5/places7, expected original bindings4/places6.
Pure control has bindings4/places4 instead of3/3. Retained source-issued local
arm has six nodes (block, initializer, binding, receiver, copied tail, IF-arm),
a consumed local binding, ended local place, and loose copied result. Existing
three-node/no-block-items checks and ternary-only emission also omit that same
local-Copy shape. No missing parser/source meaning was observed.

## Unapplied local prototype — measured, not integrated

`evidence/prototype.diff.txt` is a zero-context research patch against fixed main;
metadata records its hash and the exact generated-driver source/binary hashes.
It was locally compiled and measured, then **removed from Production files**.
211 added /26 removed main.c lines in the initial formatted prototype reused
existing readonly source/IF/Copy validators. It checked one consumed pure local,
compared only the unchanged original prefix, and used **branch_fact_prefix**
for caller Change facts, never branch-local `last_value_fact`. The emitter made
a real scoped if/else local and forwarded its Copy to an IF result temporary.
No checker, public API/issuer, ownership or lifetime rule was changed.

Seven frozen positive sources reached prototype C17 and GCC13/Clang23 strict
`-std=c17 -Wall -Wextra -Wpedantic -Werror`, O0/O2, plain + passive native.
Unit main exits0; passive checks read the actual original fields/addresses,
real operand/bool/result, returned old18 and sibling209. They create no guest
object/value/permission. `prototype_observe.py` and original emitted C/records
are retained. These are **prototype results, not current branch CLI support**.
Public source-owned observations after AST teardown record original root/field
worlds, arm-local Copy and ended loan. Existing private P517 gate rejects71
legacy root/field/arm/scope mutations for each Pair source with unchanged worlds
and zero driver allocations. New local-specific mutation controls were not
completed: no independent validator/R/F/general soundness PASS is claimed.

Registration/main OOM rollback passes all failed prefixes: Pair literal cases
251/143, both-local259/148, outer-Copy261/150, pure152/96; outputNULL on failure.
Mixed core comparison and u8(256) reject semantically with registration rollback,
artifactNULL and no C. Two-local/nested sources stay CHECK0/backend4 in prototype.

## Actual integration STOP

Prototype acceptance necessarily changes the *historical unsupported* cases:

- `tests/p626/native_test.py` expects `complex_arm.nl` CHECK0/backend4/no C.
- `tests/p628/native_test.py` expects the same one-local shape to remain backend4.

Both old source files are legitimate exact examples of the newly requested
feature. On the prototype, these **two existing CTests fail** solely at those
CLI4 expectations (`evidence/old-oracle-conflict.log`). The feature cannot be
reusable while rejecting those old sources by filename/value. Issue630 allows
at most main.c/tests-p630/CMake changes and forbids editing older P sources/
fixtures. Updating those two existing test scripts' expectations is not in the
listed path scope. We did not rewrite them, intercept CLI output, replace their
registration with a weaker test, or skip them to obtain green CI.

Thus **HOLD — INTEGRATION ORACLE-MIGRATION SCOPE BLOCKER**, not a finding that
new language semantics or a new authority issuer is intrinsically needed.
Coordination must explicitly authorize preservation of original sources/history
with migration of precisely these two obsolete backend-stop expectations, then
require completion of local-specific proof controls and bounded integrated CI.
The prototype alone is not ready for adoption. Estimate additional60–120min for
those tests/negative controls/review and a permitted natural CI pair; not a new
CFG/owner project. Current task stops instead of silently expanding paths.

After removing the prototype, both old integration tests pass2/2 and the new
sources return their original CLI4. All four old control source/C17 hashes are
unchanged (`restored-main.json`). No FULL5/PARTIAL was started for a candidate
known to fail old tests. Original distinct fixed-main CI was green before work.

## Replay

Inspect freeze, baseline and prototype metadata first. With a clean throwaway
checkout of this branch, ordinary baseline remains:

```
CC=gcc cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
./build/newlangc --check tests/p630/then_true.nl  # 0
./build/newlangc tests/p630/then_true.nl          # 4, no C
```

**Research only, not a candidate or adoption instruction:**

```
git apply --unidiff-zero tests/p630/evidence/prototype.diff.txt
git apply --unidiff-zero tests/p630/evidence/probe-build.diff.txt
cmake -S . -B build
cmake --build build --parallel 2 --target newlangc p630_public_probe p628_public_probe p517_gate_test p514_gate_test
python3 tests/p630/prototype_observe.py build/newlangc build/p628_public_probe build/p630_public_probe build/p517_gate_test build/p514_gate_test gcc tests/p630
# Likewise clang; do not rewrite immutable original sources/oracles.
```

Historical audit: N/A faithful selected Copy/IF §7.4, fixed-field/replace §17.4
consumption; Decision Procedure/Ledger and P626/P628 implementations reviewed.
KEEP source/authority, DEFER general CFG/owner/lifetime/issuer. Goal alignment:
normal CLI usability under Compass/current Roadmap/compiler-first strategy;
no distinct goal deviation, Product/library/H1 claims or other Track launch.
