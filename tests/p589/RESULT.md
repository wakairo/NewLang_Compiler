# P589 — FINITE SOURCE-ISSUED USIZE ADDITION CANDIDATE / DO NOT MERGE

Base `12c2c8e3338201ec3bda00a63ddac841ed80e574`, canonical Draft17.32.
C-selected grammar: M587/C comment6243326895. Fixed inputs, SHA256 and original
oracles: `frozen.json`; unchanged-main failures: `evidence/baseline.json`.
Integration/Historical/Goal gates: `PREFLIGHT.md`. No specification, workflow,
LLVM lock/bootstrap or unrelated Node/public proof changes.

## What changed

One flat `atom+atom`, mutually exclusive with existing flat `==`, reuses scalar
AST/checked operand links, literal parsing and ordinary Copy evaluation. Names
are admitted only at the bounded expression boundary; chains, mixed operators,
parentheses, calls/fields/loans as arithmetic operands and bare numerals stay
outside SOURCE. Both operands must be exact core usize, known and dependency
free. `b <= SIZE_MAX-a` is checked **before** computing the mathematical sum.
An unknown formal fails precision; type alone never proves representability.

The source checker issues a small immutable addition event from its actual
transaction state: qualified fragment/context, retained source node, ordered
operand/value IDs, fresh result, source spans, type/numbers and original binding
place/incarnation/ValueFact. No world clone or authority registry. A public
allocation-free validator rechecks source membership in the registered retained
body, exact operator/children, actual checked packages, binding identity and
historical receipt, Copy/dependency/authority exclusion, target bound and sum.

**Trust boundary:** the source-checker event issuer and immutable retained plan
remain trusted. The validator independently rechecks the local arithmetic event;
it does not re-interpret a whole program or cryptographically certify the
issuer. Historical currentness is point-in-time evidence, **not currently live
memory access**, and a raw/provisional source fragment is deliberately not
certified. Plain usize has no Option occurrence or nonCopy authority to grant.
Original user source/parser/AST are destroyed before owned observations.

Normal C17 validates the receipt before emission and computes **actual original
carriers**, e.g. `const size_t nl_local_3 = (nl_local_1 + nl_local_2);`.
These admitted atoms have no observable runtime effects, so the C binary
expression preserves the source's observable left-to-right semantics. Neither
checker-known sum substitution nor host-injected guest operands is used.
Existing CHAR_BIT/exact-u8 and SIZE_MAX guards remain unchanged.

## Measured matrix

- Six arithmetic positives: 2+3 literals, 2+3 names, 0+0, MAX+0, Copy chain
  19+23=42, and result consumed by existing usize equality (5==5).
  SOURCE/CHECK CLI0; real registered owned validator true; normal C17 CLI0;
  strict GCC14.2 and Clang23.1.2 O0/O2 plain and passive-observer native exit0.
  Original operand/result IDs, facts/spans, bindings, emitted C hashes and native
  statuses are in `evidence/{gcc,clang}-native.jsonl`; emitted C is retained.
- MAX+1: semantic USIZE-ADDITION-OVERFLOW at original `+` span
  1:54–1:55. Oversized literal: USIZE-LITERAL-RANGE at 1:27–1:54.
  u8+usize: semantic USIZE-ADDITION-TYPE at 1:32–1:33.
  Unknown parameter: precision USIZE-ADDITION-PRECISION at 1:23–1:24.
  All CLI3, zero C bytes, rollback, no partial artifact.
- Scalar-as-write-authority: P3-WRITE-REF-REQUIRED. Six grammar negatives remain
  parser refusals; no semantic/overflow success is claimed for them.
- Frozen escape prediction was too late: LOCAL-LOAN-PROFILE is the first barrier
  for a plain usize loan, so this input does **not** establish escape checking.
  Its source/oracle were retained and this calibration is explicit in the test.
- Original equality regression passes. Two obsolete tests migrated **without
  changing their sources**: `x+y` moves to parser success; `u8(7)+u8(2)` now
  expects semantic TYPE instead of old parser failure. No P585 evidence changed.
- Separate copied views and actual test-owned artifact faults: 48 refusals per
  literal case, 55 per named case. Foreign same-number origin/receipt, omitted
  receipt, wrong nodes/types/values/result/spans/operator/currentness,
  dependency/ref/Allocation contamination and false-overflow data rejected by
  public validator and (for actual artifact faults) the normal backend gate.
  Observed artifact/context arrays remain bytewise unchanged; originals pass
  after restoring each fault. Provisional fragment validation fails; a nonempty
  checked output slot remains untouched. No production fault hook introduced.
- Passive observers only read generated locals. Wrong arithmetic/result C
  mutants retain those observers and exit91 at O0/O2 (operator mutant only where
  XOR differs mathematically from addition), confirming observation sensitivity.
- Parser malloc/calloc/realloc failure sweep: 19 indices, no partial tree,
  parser reusable. Semantic registration/main sweeps: names84/39 and
  literals68/29; every earlier index OOM, rollback and output NULL. Public
  validator performs **zero** allocations even under fail-at-zero injection.
- Fresh GCC frontend suite: **322/322**. Local Clang focused regressions:
  **12/12**. Local ASan/LSan focused regressions: **12/12**, including genuine
  checker/parser OOM and generated-native O0/O2 tests. Logs retained here.
  GitHub FULL5 + NO-LLVM PARTIAL run IDs/results are recorded on Issue589/PR
  after this commit; local LLVM23.1.2 evidence is not relabeled pinned CI23.1.3.

## Reproduce

```
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R 'p589|p504|c17_target_model|p572_' --output-on-failure
build/newlangc --check tests/p589/names.nl
build/newlangc tests/p589/names.nl > /tmp/p589-generated.c
build/p589_public_probe tests/p589/names.nl
build/p589_gate_test tests/p589/names.nl
```

## Cost, limits and next decision

The expected scalar path worked; authentic historical binding currentness needed
one compact owned receipt rather than inferring origin from matching numbers.
Measured 64-bit-host storage: **256 bytes/event**, **16 additional bytes/fragment**
(public view216 bytes); destructor frees the event chain. Validation uses bounded
receipt/source/world scans, with no allocation or numeric interpreter. No broad
range analysis, generic precedence, uintptr arithmetic, dynamic precondition
proof, new numeric ABI, authority/effect/owner/Array machinery was added.

The next concrete barrier is **unproved dynamic range**, as demonstrated by the
unknown-formal precision diagnostic. This capability is reusable for known
quantity/Copy computations and existing equality consumers; it does not establish
ordinary-safe allocator/container libraries, arbitrary quantity expression
composition, Product GO, cross-target execution or any North Star PASS.
Alternatives remain literals/equality only or separately reviewed range-proof
work. A new public proof/backend trust boundary merits independent Red Team at
C's discretion; no Track was started. No goal deviation was identified within
this explicitly selected bounded Exploration. Main/canonical remain unchanged.
