# P380 G0 — zero-argument u8 return, bounded C17 reference lowering

Track: P. DRAFT / DO NOT MERGE. Canonical Draft17.30 is unchanged.
Base main: `348ca7d48f6dd5d658706f4547ab7ef9a63af33d`.
FormalProof reference `a8de4ebb4099f4742fc161d26867740dcac48414` is not a
compiler/runtime proof. Scope and stop conditions are in Issue #380.

## Preflight and history

Compass, Process §§2/4.1–4.3/7, Product/Value Strategy, Testing Strategy,
Decision Procedure, Design-Intent Ledger, selected call/type/result rules
(Draft17.30 §§5–6/18/26–27 and known-body dependency rules), P11/P9/V1 reports
and P379 were checked. Historical audit **N/A — faithful backend implementation**;
KEEP already admitted source/call/Copy-result semantics, DEFER general arguments,
receiver/library APIs, arithmetic, recursion, arrays, strings, LLVM lowering.
No new normative rule, source admission, ABI, public API or Ledger entry.
The source and semantic checker modules are unchanged.

Goal Alignment: source-checked non-unit functions become genuinely executable.
Evidence: real source-owned expectations and actual generated-callee observations.
Alternative: existing unit/Pairs already execute, but do not close this return gap.
Stop/owner: any extra source/semantic/authority mechanism returns to Coordination;
G1 is not an automatic sequel. Constant return is table stakes, not product value.

## Unchanged compiler first

Exact input is [scalar7.nl](../../evidence/p380/scalar7.nl), SHA256
`4d86fe28552a973b758874e08235549e33c78346c65d044d62f11baa7b9c77c3`.
Fresh GCC Debug build of the exact base, no experimental PR or switch:

```sh
newlangc evidence/p380/scalar7.nl > baseline.c
# exit4; baseline.c length0; V1-BACKEND-UNSUPPORTED
```

Baseline compiler SHA256:
`1770c740cd725fec9beca45b3b47b55197394521777e61081ed0a7ad66e90abc`.
The independent read-only public-API probe loads this actual source, parses the
unit, registers durable source bodies, destroys parser/source owners, then checks
ordinary main(). It prints only after those operations succeed. The unchanged
artifact reports parse/register/check0, owned=true, callee function11/type6/u8=7.
The probe does not call the emitter, seed a value or create a semantic world.
The baseline driver binary hash remained identical after building the probe.

## Implementation and precise boundary

The CLI's private function table retains checked result type. Validation admits
zero-argument body-backed unit or u8 calls, with the existing exclusions on
special owner definitions. A u8 body must have a supported scalar tail with
matching body result identity, or a direct terminal u8 return. The scalar tail
uses the existing literal/local/field evidence predicate. Prototypes and
function definitions become `uint8_t` only for checked u8 results. Actual
callee tails/returns are emitted; discarded calls still invoke that function.

No C body is made from the caller's known result. Names, literal spellings,
semantic IDs and source types are not C ABI promises. Existing compiler-owned
artifact trust assumptions and body ownership are unchanged; no deserialization
or new caller-supplied checked-artifact entry point exists. The checker remains
responsible for semantic validity and scope/ownership completion.

The CLI entry remains unit-valued. Call-valued receiving/tails, parameters,
branch/loop native lowering, richer return packages and nested terminal control
remain unsupported. Generalizing that profile is not authorized by this report.
Existing V1's now-supported discard case is positive; its backend-negative is
replaced with the already-checkable but still-unsupported receiving form.
The fixed-field store/receiving backend controls remain intact.

## Actual native observations

`checked_c_u8_return_test.py` first obtains expected function IDs and scalar
results from the real owned artifact. It then compiles/executes pristine emitted
C with strict C17 warnings, and separately compiles a copied observer TU. The
observer inserts entry counters only; it retains every generated body return
computation and call. It executes the guest main, checks its actual call counts,
then calls those same generated functions to observe their returned values.
Expectations are comparisons, never result inputs. No authority is introduced.

The exact source7 and source9 cases observe one guest call and actual result7/9.
The source-only change has SHA256
`903351d566685204482a97985bdee26e9b6ce034d69e5058847f9a3398e30d4d`.
Two differently named definitions returning0/255, reverse call order and a repeated
call exercise identity-based lowering and deduplication. Local scalar tails and
explicit scalar returns are checked/executed too. Both pristine and observed
native programs exit0. Safe generated-C controls change a return7→8 (observer
exit77) and omit a guest call (exit78): neither is a modified source positive.

Semantic negatives: u8(256), wrong return type and unknown callee exit3 before
C emission. Parameters, call-result receiving and non-unit CLI entry remain
exit4/backend unsupported, not language-invalid. Neither host compiler failure
nor resource/output failure is reclassified as a NewLang semantic rejection.

The new integration belongs to ordinary CTest/CI. Address/undefined sanitizer
configurations instrument the compiler/probe AND pristine/observed native C;
ASan's existing leak setting is preserved. Observer checks remain active under
NDEBUG. No bootstrap pin, CI exclusion, sanitizer waiver or experimental flag.
Measured source/C hashes, owned views, native observations and statuses are in
[evidence](../../evidence/p380/local.json). Required CI at the final PR head is
reported in the single terminal Issue comment, never inferred from local green.

## Reproduce and stop

Use the existing locked bootstrap/activation and normal CMake/CTest workflow.
The optional `NEWLANG_G0_EVIDENCE` writes diagnostic measurements, not authority.

```sh
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
NEWLANG_G0_EVIDENCE=/tmp/g0.json ctest --test-dir build -R g0_u8_return_native -V
./build/newlangc evidence/p380/scalar7.nl > seven.c
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror seven.c -o seven
./seven
```

**G0 TECHNICAL ONLY, G1 NOT TESTED, H1 UNDECIDED.** This no-parameter result
discard does not offer G1's safe state-update API or author/caller contract. No
H1 detach/adoption, STB grow, broader helper, another Track, merge or spec change.
