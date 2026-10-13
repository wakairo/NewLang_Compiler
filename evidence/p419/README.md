# P419 — one scalar relay frame (Exploration, DO NOT MERGE)

Authority: unchanged Compiler `54e1cbebfd82dddf3e9746f296ffbf908646b00d`,
CURRENT_SPEC Draft17.30 §§18.1/19/24/27. No language, checker, AST, public API,
owned certificate, LLVM pin or ownership change. Historical gate N/A: faithful
lowering of selected ordinary known-body Copy calls; KEEP immutable registration
and source-issued formal/result identity. No general call graph/ABI claim.

## Phase A before production edits

Test-only commit `340cd92c6d47cdc5a84bb4e2937c285e6e98b1b1`;
[Actions 38471748120](https://github.com/wakairo/NewLang_Compiler/actions/runs/38471748120),
job 115552300004, SUCCESS. Workflow separately checks out the immutable compiler,
adds only a test probe target, and does not overlay production code.
Main baseline [38470706175](https://github.com/wakairo/NewLang_Compiler/actions/runs/38470706175)
completed SUCCESS at the exact fixed main.

| Source | SHA256 |
| --- | --- |
| nested9.nl | `7937d2b96e33312e5d61bdd49209e28b1627018923e28d0269cd67dd0cd0fa0e` |
| nested255.nl | `96bca5f91340d0a1fa2df3ccf6d76441fab955368655f3b801d52e0ef92dca98` |
| renamed.nl | `9db8761761e9afa1fd831e91547104cc8282a8d870b3d78ebb8152cac638326b` |

All three: parser/registration/CHECK/actual OWNED PASS (probe exit0), four scalar
frames, CLI4/C0 `V1-BACKEND-UNSUPPORTED`, NATIVE not reached. Diagnostic has no
source span; the note names the input. The exact checked boundary is the scalar
callee BLOCK tail's REGISTERED_CALL, rejected by the old `v1_copy_body`.
Prediction matched observation; no new checker/source/certificate deficit.

For nested9: first relay formal1 receives actual package1; inner leaf formal2
receives package2; inner and outer return package3. Second relay formal4 receives
package4; inner formal5 receives package5; both return package6. These IDs are
context-qualified source-issued packages, NOT assumed globally valid numbers.
The immutable registry body owner/source and signature are authenticated; exact
actual package uniquely locates each fresh hidden formal binding. Formal copied
use and each body→call result are checked separately. The input syntax is disposed
before inspecting owned artifacts. No synthesized function, seed or host actual.

## Finite implementation

Only `src/main.c`: allow one registered u8 call as the scalar body's **tail**,
validate every actual frame using existing authentication while saving/restoring
its formal context, bound scalar depth to relay+leaf (2), materialize a genuine
runtime `nl_fn_leaf(outer_formal)` into `nl_call_ID`, return that carrier.
Prototypes precede definitions; a representative authenticated body supplies each
C formal name, never a caller's numeric symbol. Repeated invocation bodies are
validated before C-definition deduplication.

Scalar local/statement/direct-return support is unchanged. Nested calls in
arguments, explicit nested-return expressions, a second nesting level, refs,
nonCopy and two-formal calls are NOT added. No inlining/constant substitution.
The new gate is driver-local and adds no allocation or semantic state mutation.

## Reproduce

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
build/nested_u8_checked_probe evidence/p419/nested9.nl --controls
python3 tests/integration/checked_c_nested_u8_test.py \
  build/newlangc build/nested_u8_checked_probe gcc evidence/p419
```

Repeat with Clang. CI's locked bootstrap supplies the project toolchain.
The unchanged-compiler workflow remains a fixed-reference measurement on branch
pushes; ordinary PR validation covers the exact candidate in all five matrices.

## Verification and limits

Three positive actual sources: deterministic CLI0 / generated C836 bytes
(nested255 C838); four owned frames, distinct formal bindings, exact actual/bound
and result packages. Strict C17 GCC and Clang O0/O2 run both unmodified generated
code and a TEST COPY with read-only observer statements. Observers read actual
inner/outer formals, inner/outer result carriers and caller receivers; counters
require two real invocations of each callee. They supply no argument, result,
Domain or ownership, and add no guest ABI/I/O. Wrong inner and wrong outer C
returns independently abort (exit -6) with unchanged observers.

For each source, 24 test-owned mutations across both outer and both inner frames
reject: body-backed flag, wrong registered function, genuine wrong result package,
missing body owner, wrong formal package and missing formal use. Each is restored;
validation succeeds again. Semantic snapshots are exactly unchanged on acceptance
and refusal; refusal is gated before emission (C0). No public general scalar
validator is introduced: these are existing driver-private registered-body checks
plus public checked/binding/value inspection, not a new public certificate.

Negative sources: second level/two formals/ref/nested actual → CLI4 backend C0;
recursion → CLI3 P11-RECURSIVE-ANALYSIS-PRECISION; LifetimeDomain nonCopy signature
→ CLI3 P8-SIGNATURE-PRECISION; range → CLI3 V1-U8-LITERAL-RANGE; unit actual →
CLI3 P3-ARGUMENT-RESULTS (existing unsupported argument-result classification);
wrong return → CLI3 P8-BODY-RESULT. Diagnostics remain classified by actual stage.
Original P399 six positives/eight negatives and the complete regression suite
remain required. No new OOM behavior is introduced; existing semantic rollback/OOM
suites run without modification. No new source-specific allocation fault campaign
or formal proof is claimed.

## Exploration map / goal boundary

One previously rejected compositional backend edge is now testable using the
existing source-issued frames. This provides new information beyond two calls
from main (already PASS). Two-formal ABI broadens an independent axis; Node
changed-match/public-certificate M414 remains a larger held obligation. Future
investment and independent Red Team review belong to Coordination.

This is capability/safety evidence, not C-class product/burden superiority.
No identified goal drift: ordinary helper composition remains within the bounded
Exploration authorization; Product GO is not inferred. V394/V411/V415 Product-P
HOLD, H1 original5/6+4+2/B.prev=B/completeB+dst/0–5/B1–B7 UNDECIDED/H1-only DEFER,
and STB six-contract HOLD are preserved.
