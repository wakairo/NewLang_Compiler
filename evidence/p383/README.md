# P383 — bounded Pair call C17 experiment (DRAFT / DO NOT MERGE)

This evidence is stacked on **unmerged G0 PR381**, exact
`f0a4ccf89439ab8eca159695134b543da2aac21a` / `p380-g0-u8-return`.
Canonical main is `348ca7d48f6dd5d658706f4547ab7ef9a63af33d`, Draft17.30.
Neither layer is current main functionality. FormalProof reference
`a8de4ebb4099f4742fc161d26867740dcac48414` is not runtime soundness evidence.

## Measured boundary and finite change

The unchanged main and unchanged PR381 each reject the exact P382 input
(SHA256 `4033da51c6b2434b07b5f118ce07c3950ff66cd7b6cbb807573447810323c22f`)
with exit **4**, zero C bytes, `V1-BACKEND-UNSUPPORTED` after semantic success.
`preflight.json` records that initial measurement before any implementation.

The only production change is in the Checked-C driver (`src/main.c`). A Pair
field write loan can forward the result of one source-backed, two-argument
`ref<write,u8>,u8 -> u8` call whose entire body is a tail `replace` using its
two formal identifiers. No parser, checker, public API, syntax, experimental
flag, spec, or general ABI/effect/owner mechanism changes.

For **each** actual call, the gate checks the retained body's world, actual
parameter types and exact package IDs against the historical formal bindings;
the copied ref's place/incarnation/scope/provenance/access must match the
completed caller loan. The callee field Change must match the caller's original
projection, parent/child incarnation and pre-facts, have fresh post-facts and
forward exactly its old Copy result through call and loan. Reusing a function
never reuses the first call's actual place or value as the next call's proof.
The emitted helper uses real pointer/u8 parameters, a runtime load/write, and
returns that runtime load. No literal-return specialization, inlining, pointer
escape, `restrict`, host seed, or synthetic Domain/Allocation is involved.

These are **lexical local Pair roots**, with implicit local stability. There
are no heap Allocation/Domain responsibilities here. Ended roots/scopes in the
final context are not evidence of invalid earlier access; point-in-time checked
field records are used. No general immutable-Pair-artifact revalidator is
introduced: existing public APIs supply source-issued node/value/binding/place
views and retained bodies, which the driver gate and separate read-only probe
check. The public interface is unchanged.

## Reproduction

Build normally and run the normal CTest integration:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R 'g[01]_.*native|fixed_field|body_' --output-on-failure
python3 tests/integration/checked_c_pair_call_test.py \
  build/newlangc build/pair_call_checked_probe gcc none evidence/p383/pair-g1.nl
build/newlangc evidence/p383/pair-g1.nl > /tmp/pair-g1.c
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror /tmp/pair-g1.c -o /tmp/pair-g1
/tmp/pair-g1
```

`pair-g1.pristine.c.txt` is verbatim generated C, **not** host-authored output.
The `.txt` suffix preserves emitter bytes independently of repository C source
formatting. `pair-g1.observed.c.txt` is a test-only COPY with callee-entry
counting and reads/logging inserted. Copy either to `/tmp/example.c` to compile
with the same strict C17 flags. Pristine execution returns 0 without output.
The observer does not replace any argument, field write, return calculation or
guest call, and does not call the helper from host code. Expected values/symbols
come from a separate source-generated owned-artifact probe. Compiler/checker
and native agreement is bounded evidence, not a general soundness proof.

## Results

GCC 14.2 and Clang 23.1.2 both strictly compile and run all four pristine and
observed cases: exact input, differing source old/new/marker values, ordinary
caller-local actual u8 values, and a renamed helper. CLI/native/observer exits
are all 0. `gcc-debug.json` and `clang-debug.json` retain sources, hashes,
public checked evidence, native snapshots/bindings and diagnostics.

Original native snapshots:

| Point | A.left | A.right | B.left | B.right |
| --- | ---: | ---: | ---: | ---: |
| before | 7 | 1 | 9 | 2 |
| after first call | 11 | 1 | 9 | 2 |
| after second call | 11 | 1 | 13 | 2 |

Caller old values: 7 / 9; actual helper entries: **2**. The different source
case observes old 80 / 90, new 123 / 124, markers 3 / 4. Caller-local actuals
observe new 22 / 33. Full GCC Debug regression: **250/250 passed**; the normal
five-job CI results for the final candidate are recorded in Issue #383 / PR
checks, not inferred from the ancestor's green CI.

Eight source negatives preserve their boundaries: scoped ref escape,
helper-mediated read-only-to-write and value-to-ref misuse, u8(256), wrong
helper result type (semantic exit3); arbitrary record name (unsupported
FIELD-PROFILE exit3); richer callee body and general scalar-parameter call
(backend exit4). All emit zero C. Three controlled output corruptions—wrong
field, missed guest call, wrong callee return—still compile/run but are
**rejected by the independent observer comparison**. These are correctness
controls, not claims that the source checker rejects wrong application policy.
Existing safety/transactional/OOM suites are preserved. The new gate itself
allocates nothing; no new OOM success guarantee is claimed.

## Authority, historical audit and product limits

Compass -> Product Strategy / North Star / Coverage Map; Process §§2/4.1–4.3/7;
Design Decision Procedure and Intent Ledger DI-001/002/006/007 were checked.
§4.2 **N/A for a faithful implementation** of selected ordinary functions,
scoped local loans and fixed Pair fields (Draft17.30 §§8/13/17/18/20/26).
KEEP `@` / `::` distinction and explicit ordinary capability passing; DEFER
private/receiver/general record/loan syntax. No normative or Ledger update.

Author: one `replace` body shared by two calls. Caller: two explicit scoped
loan wrappers, field choice, new value and Copy-old receipt remain. The formal
ref type does not distinguish application state from marker policy. In C,
`uint8_t replace_u8(uint8_t *p,uint8_t next) { uint8_t old=*p; *p=next; return old; }`
already has tiny factored caller ceremony, with manual valid-address/lifetime
obligations; Rust `mem::replace` and a factored Zig pointer helper are also small.
NewLang statically rejects insufficient ref mode/nonescape errors but no human
benefit or superiority was measured. L/P get bounded checked capability flow;
V has actual old/new/sibling evidence; R/nonCopy and nontrivial C/control remain
outside this workload. Same-unit body-visible S5a only; no opaque library claim.

**GOAL-ALIGNMENT-RISK for Coordination:** do not expand general ownership,
privacy or effects to improve this tiny case's appearance without a measured
combined author/caller benefit. G1 technical execution is not product PASS.
A focused independent R assessment of the new runtime-ref/checked-artifact
bridge is recommended; this task launches no other Track. cJSON H1 remains
PASS/FAIL UNDECIDED; STB remains HOLD. Stop at this bounded result.
