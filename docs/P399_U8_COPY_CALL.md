# P399 — ordinary one-u8 Copy call, source-owned C17 candidate

Track: P. Issue [#399](https://github.com/wakairo/NewLang_Compiler/issues/399).
**DRAFT / DO NOT MERGE.** Parent is frozen Compiler/main
`348ca7d48f6dd5d658706f4547ab7ef9a63af33d`; CURRENT_SPEC selects Draft17.30.
No changes from unmerged PR381/384/395/396 or P397 are incorporated.
FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414` is not proof of this patch.

## Before and after

`evidence/p399/unchanged.json` records real unchanged-compiler runs of every
positive and negative source. All six positives reach semantic admission then
CLI4 `V1-BACKEND-UNSUPPORTED`, with zero emitted bytes.
The candidate reaches SOURCE / CHECK / source-issued CHECKED-OWNED / C17 / NATIVE
for the same source bytes. Generated C is retained verbatim as `.generated-c.txt`.

| Source | Candidate CLI | C bytes | Native post-call observations |
| --- | --- | --- | --- |
| echo7 | 0 | 402 | 7 |
| echo9 | 0 | 402 | 9 |
| two | 0 | 466 | 7, 9 |
| forward | 0 | 625 | 9, 9; second actual is first receiving local |
| renamed_local | 0 | 531 | 7, 9; decimal 007, different function/formal spelling |
| explicit_return | 0 | 490 | 255; direct return |

SHA256 and actual owned package IDs for each source are in
`evidence/p399/local-gcc.jsonl` and `local-clang.jsonl`.
Both native compilers use strict C17 with `-Wall -Wextra -Wpedantic -Werror`,
at O0 and O2, for unmodified generated C and a separately observed test copy.
Guest main returns unit; emitted C supplies no guest I/O or new exit ABI.

## Runtime lowering and authentication boundary

Only the driver changes. A body-backed registered call with one Copy u8 formal
and one Copy u8 result gains a typed `static uint8_t fn(uint8_t parameter)`
prototype/definition. Calls produce stable `nl_call_<checked-node-ID>` carriers;
a receiving local forwards that runtime carrier to subsequent actual arguments.
The actual callee body is emitted, not inlined from checker-known scalar facts.
Numeric spellings, function names and test expectations are not emission inputs.

Every invocation, including a second call to the same registration, checks:

1. Exact registered function identity, signature and retained synchronous body
   owner, source and context identity; no signature-only or foreign cloned body.
2. Actual source-checked scalar argument, exact u8 parameter type, one argument,
   and the fresh historical formal binding holding that exact Copy package.
   The immutable registration's parameter name only locates this binding;
   spelling does not establish a callable, type or runtime value.
3. A finite scalar body: Copy locals/use, one scalar tail or direct return, no
   nested call/branch/ref/write. Only the authenticated formal or source receivers
   obtain C local carriers. The call result package equals the owned body's
   checked tail/return package, with the correct normal/return exit evidence.
4. A later invocation is independently checked before reusing the representative
   C function body. Its new body-local IDs are never mistaken for prior C locals.

The private driver reads existing internal registration/control metadata because
public views do not expose an ordinary function's parameter/body ownership table.
No public API or checked format is changed. This validator consumes genuine
compiler artifacts; it is not a general externally supplied artifact verifier.
The test probe uses actual source parsing, registration and main checking,
disposes original syntax, then inspects owned bodies and public value/binding
views. It never seeds a callable, scalar, Allocation, Domain or ownership grant.

## Negative and control evidence

- CLI3, zero bytes: out-of-range literal, unit actual, wrong ptr actual type,
  wrong arity, uncalled invalid return type. Existing diagnostic codes retained.
- CLI4, zero bytes: two formals, nested-block richer scalar body, general ref call.
  The existing zero-argument u8-return backend rejection remains unchanged.
- Eight test-owned artifact controls reject: missing body flag, wrong function
  identity, missing result, wrong count, missing retained body owner, foreign
  cloned body context, missing formal package, stale formal identifier. Restoring
  each artifact leaves a valid proof. Validator semantic snapshots are unchanged.
- C callee mutants `return 0` and `return 7` instead of returning its actual
  parameter fail the unchanged post-call observer on the source-9 case at O0/O2.
  Observer code only reads existing call/receiving carriers; it does not replace
  a result or supply an argument.
- Local GCC Debug: all 249 tests PASS (28.03 s for final full run). Existing
  function-body/rollback/OOM suites remain in that run. New validation/emission
  adds no heap allocation; no new allocator/fault-injection contract is claimed.
- Standard exact-head five-configuration CI is recorded in the PR and final
  Issue report after publication; local native Clang is not a CI substitute.

## Reproduction

After normal repository bootstrap and activation:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 tests/integration/checked_c_u8_call_test.py \
  build/newlangc build/u8_call_checked_probe gcc
python3 tests/integration/checked_c_u8_call_test.py \
  build/newlangc build/u8_call_checked_probe clang-23
```

## Scope, gates and exploration map

Historical Design Intent Gate: **N/A — faithful implementation** of already
selected §§18.1,19,24,27, conditional on no source/signature/semantic change.
Compass → Product Strategy §§1–6,9 / Coverage Map and Process §§4.1–4.3 checked;
current docs-only §4.3.1 treats this as bounded Exploration GO, not Product GO.
No Ledger update is needed because no language choice is replaced or selected.

Preflight predicted a small-to-medium driver gap and a bounded few-hour session:
formal carrier plus return forwarding, contingent on actual owned evidence.
Actual work required one driver file, no checker/semantic/API modification and
no general ABI family. Measured session elapsed time and exact diff size are in
final Issue report. The new reusable rung is ordinary source-issued scalar Copy
parameter/result forwarding. Richer bodies, nested scalar calls, additional
parameter types/counts, arithmetic and general ABI remain separate gaps.
The original V0/V1/AVS/local-root/Node gates remain regression-tested.

This is a native capability candidate, not an independent Red Team acceptance,
formal proof or demonstrated user-burden/product improvement. No merge is done.
H1's original five fallible roots, 6+4+2 topology, B.prev=B, complete nonCopy
responsibility, failure worlds 0–5 and B1–B7 remain UNDECIDED/H1-only DEFERRED;
STB six allocator conditions remain HOLD.
