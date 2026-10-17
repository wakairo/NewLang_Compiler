# Track P #546 — HOLD DEFAULT ACTIVATION (read-only cost gate)

Compiler under measurement is exact main
`1c6d9cab7349a7e501b04c9157f25f60df242e02`; CURRENT_SPEC selects Draft17.32.
No Compiler, CMake, CLI, existing test, normative document, flag, or dependency
was edited. This evidence-only branch does not activate the default compiler.
No candidate PR or new Actions run is requested; this stops at the explicit
cost-gate condition for accepting source categories beyond the requested profile.

## First gate / decision

The mechanical change appears small: three parser decisions depend on
NEWLANG_EXPERIMENTAL_OPTION_ARRAY4_SOURCE (default0), while a duplicate test
parser is built with1. Both executables compile the same main.c and link the
same semantic/owned/C17 libraries. CMake builds parser.c twice and main.c twice;
P534 and P541 each have a current native-harness loop asserting ordinary CLI
rejection. Retained function bodies are reparsed through the same parser symbol,
so link order matters for initial parsing AND later registered-body parsing.
A source-mode switch must not only change the first parse.

**The toggle is not a constructor-only admission fence.** The opt-in compiler
also checks this source successfully (frozen array_copy.nl):

```newlang
fn main()->unit {
    let a=Array<Option<u8>,4>{Option<u8>::None,Option<u8>::None,
                             Option<u8>::None,Option<u8>::None};
    let b=a;
    unit
}
```

Ordinary CLI rejects at the opening Array type argument, exit3 /
P5-EXPRESSION-UNSUPPORTED. Opt-in --check exits0. The actual registered owned
body contains two original placements/ten places and ten values/two bindings,
but ONE constructor record and ZERO valid public Array records. Public C17
returns NL_CHECK_SEMANTIC_UNSUPPORTED (numeric2), outputNULL and length77
unchanged; actual opt-in CLI emission exits4 / V1-BACKEND-UNSUPPORTED with no C.
This is **fail-closed public certification/emission**, not evidence that Copy
creates invalid authority. It is also not successful source-owned Array Copy.

The opt-in parser separately checks a bare Option<u8>::None binding and a
named Option<u8>-returning function; both C17 emissions refuse. These are
surface inventory, not a claim of unsoundness or independent findings. The
canonical explicitly names concrete Option/None; that alone does not establish
all combinations have source-owned/native support. Array Copy source is
explicitly outside this task and §17.32.2 does not select a new Copy-binding
source profile. Removing the parser fence alone would expose that extra CHECK
admission. Do not silently interpret backend refusal as a source-admission fence.

Issue546 says to stop/HOLD on broad extra source acceptances. P therefore did
not activate the parser or add new whole-function/signature/value-use admission
policy. **No claim that a general mechanism is inherently necessary:** a separately
reviewed finite eligibility fix may be small, but its safe scope has not been
implemented/proved here. Coordination can authorize that narrow follow-up, or
explicitly resolve whether this additional CHECK-only source admission is
intended. This HOLD does not ask for general generics/effects/ownership.

## Baseline authority / gates

Main push FULL5 run38985471659 and NO LLVM/PARTIAL38985471624 were completed
SUCCESS, attempt1, at the exact main above before measurements. No new CI was
started. Local GCC14.2.0 frontend-only Debug regression315/315 PASS,71.48s,
full log retained. This is PARTIAL local evidence, not the locked full matrix.
Local Clang23.1.2 was used only for emitted native C; repository LLVM23.1.3
pin/bootstrap/workflows are unchanged.

Compass -> Product Value Strategy1.1/1.2, Process4.1.1/4.2/4.3.1, Design Decision
Procedure, Ledger DI001/008/016 and Draft17.32.1–4 inspected. Historical gate
N/A for the proposed faithful availability change/evidence: KEEP selected
concrete constructors/core identity laws; DEFER DI002/005/006/007/general
source/ownership. No new language choice was made. In particular this report
does not promote an implementation's wider CHECK acceptance to a source rule.
R544's original BLIND FINDING and incomplete old digest attestation are
preserved. R545's calibrated registered-path limited PASS and PR543 comment-only
contract clarification are distinct; neither substitutes for a new default-CLI R
review. No independent R/formal pass is claimed for P's measurements.

## Genuine existing source/owned/native results

Independent P inputs/oracles are in frozen-inventory.json, written before
executing inventory.py; all measurements use unchanged Compiler. SHA256:

- roots1.nl: e3f9e55337d5d46aec43104a89d5c3c760988d0abe6312fffbf7a3fc09c93d71
- roots2.nl: f4f486a9f558aba37f0f35190bf767f4e4868ca204ff1b9922ed0f4b9bb18099
- roots16.nl: 38217b2b756826c267e9336ec22e19f5e17608c788c6fa4154518e2cacdcf07b
- array_copy.nl: 662884254d522d5f645faab9a80c5becff2e0567540aa5bf6e18bfab7bac5606

All normal newlangc constructor inputs fail SOURCE exit3 at2:27–2:28,
P5-EXPRESSION-UNSUPPORTED; **no normal-CLI CHECK/owned/native advance**.
Opt-in direct constructors1/2/16: CHECK0 and C170. On the actual registered
function route after original unit AST/source destruction: record counts1/2/16,
all public valid_at true, checkpoint prefix sizes5/10/.../80. No seed_root or
host rights. Public probe1 checks40 copied-view refusals; probe2 checks100,
including foreign origin/cross-root identity. Original roots1 and6 have their
own distinct incarnations/facts1 and6, each four dependent typed None children;
final lexical roots ended/bindings consumed. The captured validation certifies
historical construction, never current live reference/owner/Domain authority.
owned1.json,owned2.json and baseline-admission-owned.json retain actual outputs.

Opt-in plain emitted C and separately passive observed C for1/2 originals:
GCC/Clang × O0/O2 × plain/observed =16 executions, all native0, strict C17
-Wall/-Wextra/-Wpedantic/-Werror. The observer uses the actual source-function
live typed Array carriers, full sizeof(Option) stride, independent disjoint
originals and compiler None tag37; no guest writes/value/authority injection.
Actual plain/observed text and run-specific observations/hashes retained.
These are **existing opt-in capabilities**, not default activation or new P
production implementation. Existing315-test baseline also retains all prior
lexer/parser/typed/lifetime/Node/Pair and Array semantic/fault/native/OOM tests.
No old fixture, manifest, oracle, historical result or R evidence was rewritten.

## Resource cost (Linux/glibc measurement, no production change)

Each source construction event clones a growing actual semantic context.
Stored prefix worlds are O(k²) in total; selecting/validating each prefix also
replays source and world structure. All-event C17 validation repeats prefix
work; observed CPU scaling is worse than linear. Validation itself allocates
zero in the existing fault probes, but that does not eliminate retained-world
cost. Existing budgets remain parser4096nodes, semantic4096entries; no new k cap.

Native child launcher removes the Python parent's RSS floor. `heap-cli` is a
SEPARATE locally linked measurement driver using the unchanged main/parser/libs
with malloc/calloc/realloc/free wrappers and glibc malloc_usable_size. Its peak
is **wrapped application live usable heap**, excluding libc's internal private
allocations; it is not total process memory nor requested-byte accounting.
All wrapped live bytes return to0 on measured success/failure. Unwrapped CLI
RSS/time are measured separately; one run each, not statistical performance.

| k | unwrapped CHECK s | unwrapped C17 s | unwrapped C17 RSS KiB | wrapped peak heap bytes |
|---|---:|---:|---:|---:|
|1|0.001381|0.000866|1260|104704|
|2|0.000867|0.000995|1336|137624|
|16|0.002171|0.004041|3148|1467568|
|64|0.017305|0.078343|23608|15688288|
|128|0.074519|0.534648|86704|57802352|
|240|0.286708|3.200917|296324|195262656|

For this exact direct body,241 originals fail before semantic allocation at
parser4096-node limit: exit3 error(host)[P2-NODE-LIMIT],242:81–242:87,
empty output. The bound is an implementation budget, not a new language k cap.
Even within it,~28KB source reaches~289MiB RSS, so cheap small examples must not
be sold as scalable untrusted-source support. No OOM-killer resistance or
arbitrary multi-function scalability has been proved.

External RLIMIT_AS32MiB, unchanged CLI:128/240 constructors reject with exit3
error(host)[P3-OUT-OF-MEMORY], empty output; for roots128 span86:5–86:116,
roots240 span85:5–85:116. This external test limit is not a new compiler policy.
Source-owned OOM injection at k16 additionally exercises1062 registration and758
main allocation failure positions: rollbackPASS, outputNULL. Existing1/2 OOM
and private-issuer corruptions pass in the baseline suite. These are bounded
fail-closed observations, not a proof that resource amplification is harmless.
Resource cost is a separate maintenance concern, **not the sole HOLD reason**.
Normal CLI costs past its parser boundary are unmeasured because not activated.

## Minimum next investment / stops

A bare macro/default change and duplicate-target cleanup are mechanically small;
about two old current-CI harness expectation loops need migration, not a suite
rewrite. But first distinguish selected direct construction from the already
broader opt-in CHECK surface. Do not invent an arbitrary root-count language
limit or silently accept all value-flow forms to make parser activation cheap.
A follow-up should freeze ordinary-CLI admission negatives for Array Copy and
other out-of-scope combinations BEFORE its checker/CMake edits, decide their
finite eligibility boundary against canonical, then use the same authenticated
issuer/public validator. Exact code/CI repair cost beyond that boundary is
**not measured**, not asserted to need a general framework. The resource numbers
above should inform that decision; a new general memory algorithm is out of
this task. Main/spec/API/flag changes, candidate FULL5/PARTIAL and default-CLI
independent R revalidation are all NOT PERFORMED here.

VALUE-ALIGNMENT-RISK (separate): making an adopted constructor usable through
ordinary CLI is a capability/availability improvement; no safe Ring4,
Option<OwnedFrame>, owner refund, indexed access, representation privacy, or
library-author + ordinary-caller UB-audit locality delta has been demonstrated.
Product GO remains HOLD. Keeping isolated opt-in/HOLD, a separately authorized
finite eligibility fix, a more valuable intrusive/arena exploration, safe Rust
tagged Ring and factored C/Zig remain alternatives. Coordination owns the next
scope/investment/independent-R decision; no other Track was started.

## Local execution limitations / honesty

A managed-environment restart interrupted the first /tmp-only inventory and
local baseline build/test session. /tmp was lost; workspace was preserved.
Final files here come from a fresh exact-main build in persistent /workspace,
not a claim the interrupted run passed. No CI restart or repair shopping.
The standalone measurement probe initially omitted <stdlib.h>; strict host C
compilation rejected free before any probe execution. Added that one missing
include; no source/oracle/Compiler change. The final portable evidence include
points to the same repository test loader; original measured code used its
absolute workspace path. This is local harness hygiene, not a compiler defect.

The task is stopped at the explicit broad-admission gate, without spending the
remaining implementation/CI budget on a new eligibility policy. Timestamps and
actual cost must be reported as observed, never a claim that an unimplemented
activation or the planned25–40min inventory elapsed.
