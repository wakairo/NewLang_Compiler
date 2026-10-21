# P688 Stage0, before production edits

Fixed main 92367144da5c672282b91fd291962e3b923492b3; Draft17.33.
Main-push FULL5 39541483597 (5/5) and PARTIAL 39541483643 (1/1):
success, exact SHA, push, attempt1. Dedicated p688-c17-u8-export worktree.

`freeze.json` predates all source execution. Unmodified compiler outcomes are
in baseline.json; normal source CHECK0/C17/native0 for both positives.
Public-only selected-call probe registers actual source, destroys input AST,
then parses/checks `low_threshold()` / `upper_limit()` using existing APIs:
call/body Copy result Value IDs equal, actual source scalars17/53.
No host semantic seed. Production src/main.c and CMake diff were empty.

First missing boundary: CLI rejects opt-in export arguments (2, zero C).
Existing normal emission has private static helpers and unconditional C main.
No semantic or owned-evidence gap for these scalar bodies was found.
The original rich.nl had a test syntax mistake (missing IF parentheses).
It is preserved; rich_calibrated.nl was separately frozen before execution:
CHECK0 / ordinary backend4, the intended legal-but-richer-body negative.
Public probe initially lacked stdlib.h; local build diagnosis corrected that
test tool before running it. Neither observation is a compiler finding.

Conditional GO: estimated production delta ~60-100 lines, confined to
src/main.c selector/entry text, root admission/output mode and a fixed C17
header. Reuse v1_call_parameter/v1_copy_body/v0_validate_call/v1_emit_body
unchanged. The already validated reachable-functions collector suffices;
no arbitrary CFG, source grammar, issuer, public proof or field-specific gate.
Tests/CMake/docs are separate verification/support cost. STOP if that estimate
requires new privileged mechanisms or a new body validator. Only one prepared
HEAD / one natural FULL5+PARTIAL pair; no CI repair/rerun authority.

Historical gate: KEEP Draft18 ordinary typed Copy returns, 19 sequencing,
24 no authority from bytes, 31 backend shim/non-amplification; REPLACE no
source rule. DEFER Draft34 general FFI, source export/extern, ptr/ref/owner
transfer, NewLang separate compilation and universal stable ABI. M687 and C's
explicit conditional target-only authorization are new experimental ABI policy,
not an assertion that external export was already implemented. PR684 remains
archived, P685 source/oracles unchanged; DI002/006/007/011 not reopened.

VALUE-ALIGNMENT-RISK: a zero-formal pure scalar export is trivial in factored C;
no original record post-state, realistic C-class API, audit/re-audit dependency
closure improvement or real consumer demand is demonstrated. This pilot tests
normal-success separate-TU consumption only, not Product GO or a safe library.
Ongoing target/header ABI maintenance is new cost. C retains adoption decision;
Product scoring PAUSED, H1 HOLD, no other Tracks launched.
