# P728: one source-issued unit-call statement in a scoped caller

Track P / non-BLIND implementation regression. DRAFT / DO NOT MERGE.

## Authority and freeze

Implementation parent is frozen PR724 candidate
`13532ab30c55b74a6685be278fa2669b8300d6a1`, tree
`ec18bd9a5f44c1becc993910b363a02c296728c8`; canonical Draft17.33.
Current main at preflight was `ddc6bc478583deff13da19a0f6426d19c8aeb7f8`:
relative to the prior M4 base only the Coordination Roadmap changed. This
candidate does not import that document change or alter PR724/R726.

Commit `41084c2` froze all P source bytes, scalar oracles, negative predictions
and unchanged-candidate outcomes before production edits. `freeze.json` contains
four independent P positives: BrakePanel target index 1, ValvePanel target
index 0, different constructor order, sibling values and distinct receiver.
Each has TRUE/FALSE sources. Brake returns before writing; Valve returns after
writing. All source conditions are actual guest `usize` comparisons, with both
source arms checked; these are not dynamic-input or universal proofs.

All four initially CHECK0; their genuine unit, parameter and IF validators
returned ANALYSIS_PRECISION (3); ordinary C17 CLI4 emitted zero bytes. The
baseline probe's process exit 0 denotes successful **observation**, not validator
success. Its two-call negative exits 1 only because the diagnostic probe expected
one receipt; the recorded individual validator statuses and CLI rejection remain
separate. Pointer addresses in this historical diagnostic are not proof IDs.

`inherited/A0v2-usize-terminal.nl` is the byte-identical R725 calibrated v2
source from R726 `e33d903738dcba43b45ddc75a978c63f5674c84d`, SHA256
`b00d4c58cb77c9180c9a3e17bc3396ea72888fc71a8deff21c8e2f8588018180`.
It is explicitly **inherited R-origin regression**, not fresh independent P or
BLIND evidence. Its original CHECK0/public precision/C174 history is preserved
in baseline.json; the R source/oracle/PR/failure records have not been edited.
P727 forensics remain separate evidence, not an imported compiler change.

## Proof and implementation boundary

`copy_body.c` retains the completed issuer-owned caller graph/source/body seal,
entry/handoff worlds and the M2/M5 formal mapping, IF return/replay streams,
callee return target, structural Unknown child+parent, sibling and loan closure.
The new independent validator route reconstructs exactly this shape:

```
loan_write(original@field) { |actual| helper(actual, scalar); }
```

The block has no tail and exactly one unit, nonterminal STATEMENT wrapper. Its
initializer is this exact named-direct `(ref<write,u8>,u8)->unit` call; arguments
and wrapper have their issued order and nested source spans. Only the block,
call, two arguments and wrapper may occur inside this scoped body. A complete
graph scan rejects a shared call, duplicate executing parent or orphan operation.
The existing tail-call path remains supported, with call uniqueness checked for
both unit spellings. The wrapper does not discard a returned ref/Copy package or
turn a callee RETURN into caller termination. There is no new semantic world,
issuer, authority, public API, registry or syntax. The new graph checks allocate
nothing and cost O(caller graph size) within the existing bounded graph.

The existing backend consumes the public unit validator and lowers the actual
wrapper using its existing statement walker. An additional strict-C defect in
the inherited R case was an unused u8 formal: unit bodies now emit the same
`(void)formal` warning suppression as scalar bodies, reusing the same item walker.
This adds no guest operation and does not relax C warnings or alter source.

History gate: KEEP DI-001/007 field/mode separation, DI-011 body-sensitive
known-call substitution and Draft17.33 §§13.5c,18.1,19.1 scoped/block rules.
The tail-only finite backend proof restriction is extended, not a normative
source rule. DEFER additional scoped statements/calls, explicit unit tail after
the statement, general effect/CFG/ownership, heap, escaping references and FFI.

## Independent observations and negative classification

`public_probe.c` uses only real registered source and public views/validators.
It checks original root/child/incarnation/index, parent fact change, structural
Unknown current child, unchanged sibling, active postcall scope then closed
caller scope, three issuer-qualified IF streams and genuine old Copy origins.
JSON files are diagnostic observations, not authority in their own right.

`native.py` compiles **unmodified** generated C and a separate passive observer
with strict C17 warnings at O0/O2. Observer data only compare/read actual guest
fields: target and receiver addresses, formal pointer equality, actual call,
old Copy captures and stores, both return/normal routes, sibling preservation,
caller readbacks and the disjoint receiver's physical store. Observer counters
supply no guest values, refs or rights. The per-case physical results are:

| case | target initial -> post | sibling | receiver old -> post | callee |
|---|---|---|---|---|
| brake_true | 53 -> 53 | 174 | 26 -> 53 | RETURN before write |
| brake_false | 53 -> 106 | 174 | 26 -> 106 | normal write |
| valve_true | 38 -> 224 | 207 | 14 -> 224 | write then RETURN |
| valve_false | 38 -> 82 | 207 | 14 -> 82 | normal write |
| inherited R A0v2 | 34 -> 34 | 23 | 51 -> 89 | RETURN before write |

The frozen read_ref prediction was corrected by **classification only**:
`loan_read(record@field)` stops at LOCAL-LOAN-PROFILE, not a semantic read-ref
judgment. Escape (P8-EXIT-DEPENDENCY), wrong return (P9-RETURN-TYPE) and wrong
argument (P3-TYPE-MISMATCH) reject semantically. Explicit unit tail, extra
statement before/after, two calls, binding wrapper are legal CHECK0 but fail
public proof precision and CLI4, zero C and preserved preexisting named output.
Existing M5 semantic read-formal/ref authority negatives still run unchanged.

`gate_test.c` checks 23 structural wrapper mutations per family, including
mutating both graph and its private byte seal so a seal alone is insufficient.
Wrong wrapper, initializer/order/parent, duplicate wrapper+tail, result, span,
termination and foreign caller issuer reject with recovery. The existing M5
`p722_gate_test` is reused on these **new sources**, without duplicating its
integrity/OOM harness: wrong root/index/facts/Copy origin, foreign source/world,
same-number foreign streams, scope/writable/return-target changes, validator and
admission fail-at-N, registration/main-call snapshots, NULL artifacts and zero
partial driver output. It also checks no allocation while emitting admitted C,
plus /dev/full failure. This is bounded fault injection, not a general proof.

## Reproduce

```
cmake -S . -B build-p728 -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p728 --parallel 4
ctest --test-dir build-p728 -R '^p728' --output-on-failure
ctest --test-dir build-p728 --output-on-failure
python3 tests/p728/native.py build-p728/newlangc build-p728/p728_public_probe gcc none tests/p728
python3 tests/p728/native.py build-p728/newlangc build-p728/p728_public_probe clang none tests/p728
```

Dedicated sanitizer builds use `-DNEWLANG_SANITIZER=address` (with leak detection)
or `undefined`. Local compiler is GCC13 / Clang23.1.2; unchanged repository
FULL5 uses its locked LLVM/Clang23.1.3. Natural PR attempt-1 FULL5 + NO LLVM
PARTIAL results and exact candidate/virtual-merge SHAs are recorded in Issue728.
`first-local-failures.txt` preserves the initial wrapper+tail mutation failure
and inherited unused-formal strict-C warning before repair; no Actions rerun
can erase a first failure. Generated C `.txt` files are exact compiler stdout;
passive variants and hashes are separate, never substitutes for unmodified C.

Goal Alignment: this repairs ordinary unit-call statement composition for the
selected v0 critical path and reduces dependence on tail-only spelling. It does
not demonstrate an ordinary-safe library's UB audit-locality benefit, heap
ownership, universal soundness or Product GO. Product scoring PAUSED624 and H1
HOLD268 stay unchanged. The finite profile and inherited M5 history-memory costs
remain; this extension adds no snapshot allocation or new public proof family.
