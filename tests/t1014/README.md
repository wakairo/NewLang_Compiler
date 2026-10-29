# T1014 source-publication-only — NEW reconstructed synthetic native bridge

**Status: UNHOOKED / NO PR / NO CI / NO R / NO Production code modification.**

This is a **NEW reconstruction (2026-10-29 JST)** from the public **design** of
[Issue #1005](https://github.com/wakairo/NewLang_Compiler/issues/1005#issuecomment-6470931428),
**not a byte restoration** of the local-only `t1005_stage0_synthetic_bridge.zip`.
That original ZIP (claimed SHA256
`358024a05a6c155bac6f30a8aa651ea650b2c614e0639a4cf9f858d291b67938`)
was not present in the current authorized workspace/available Project or Library
search. The original T1005 bridge source SHA
`5eb92365ccce5acaf844e83794b27688872eb7838a259499619329b187aba88a`
cannot be confirmed or claimed for these new bytes. This reconstruction's exact
fresh SHA256 is included in the T1014 Issue report; the source is reviewable on
one T-only branch that has **no CMake hook or Actions enrollment**.

## Scope and trust

`cmake/independent-r-native-bridge.py` is **inert preview code**: nothing
imports it from root CMake, existing `independent-r-tests-hook.cmake`, the
six-CHECK `NL_R_COHORT_V1` path, or any workflow. It has
`APPROVED: dict[str, Approval] = {}` **empty**. Consequently no actual R
native cohort can be registered or executed by its ordinary CLI. There is NO
runtime switch or environment variable to supply an approval. The synthetic
unittest explicitly injects a T-owned disposable `Approval` into a Python
function call on scratch files; this does not alter the committed constant.

The separate **`NL_R_NATIVE_V1`** descriptor lives at exactly
`tests/r/approved-native-manifest.txt`. It is **not CMake, Python, shell or an
executable plugin**, and it must contain 1–8 ordered cases from ONE issue:

```
NL_R_NATIVE_V1
cohort|r113
runner|tests/r113/native/runner.py|<64 lowercase SHA256>
observer|tests/r113/native/observer.py|<64 lowercase SHA256>
oracle|tests/r113/native/oracle.json|<64 lowercase SHA256>
case|r113_alpha|positive|tests/r113/sources/alpha.nl|<64 lowercase SHA256>
case|r113_beta|negative|tests/r113/sources/beta.nl|<64 lowercase SHA256>
```

Here `r113` is a **fictional T-only synthetic cohort**, never an authentic
frozen R source, oracle, runner, test or result. The stored fixtures are under
`tests/t1014/fixtures/`; unit tests copy them to isolated temporary `r113`
paths and calculate hashes, never touching repository R files.

A future Coordination reviewer (NOT the descriptor author) would have to
authorize all exact bytes in the `Approval` structure by trusted T code:
full manifest SHA256, cohort name, **complete ordered case ID list**, exact
frozen R HEAD and its parent, reviewed runner+observer+oracle+source SHAs, and
separately frozen independent oracles. The SHA in a PR-authored manifest is
not self-authenticating. Future Actions would additionally need to verify
checkout **synthetic PR merge parents and tree** and the fixed trusted
Production/compiler SHA, the preserved old freeze and the per-job execution.
**No such approval, integration or future R budget has been released.**

The T parser rejects bad LF/ASCII/size, mixed cohorts, changed manifest SHA,
duplicates, unsafe names, nonregular files, symlinks/hardlinks or path traversal,
and missing or altered runner/observer/oracle/source. It checks the same bytes
before and after the child. Its generated CMake template is fixed T code and
would register a single native-suite test ONLY with external approval; no raw
R content is included/evaluated as CMake and no manifest chooses the command,
interpreter, shell or arguments. `--run` rejects a missing manifest, any child
failure/timeout, missing/oversized/duplicate-key receipt, missing/reordered
cases, invalid negative reason, missing C17/Clang/world0/1/2/teardown, false
status, and changed source or helper SHA. Standard subprocess Python (even
without inherited secrets) is **not a sandbox**; future R executable runner,
observer and oracle are trusted code only after independent C review. Runtime
TOCTOU/path swap risk also needs frozen checkout and trusted CI review.

## Critical proof limitation

A receipt is a **CLAIM**. A malicious but approved runner can fabricate
`observed=true`, the SHA-like stage tokens, output values, guest0/1/2 and
`C17 unchanged`; parser schema completeness cannot independently prove any
of these physical facts. The bridge therefore returns explicitly
`CLAIM_SCHEMA_COMPLETE__PHYSICAL_UNAUTHENTICATED`, **never native PASS**.
A truthful independent R-owned observer, actual original source→CHECK→source-
issued current/old Copy and unchanged C17, real GCC/Clang O0/O2 native,
actually reached OOM worlds0/1/2, ASan/UBSan, teardown and reason-specific
negative diagnostics would require *separate C authorization and observed
GitHub Actions*, never local P-equivalent R execution. For a source whose
original guest cannot be observed, result is **NOT OBSERVABLE / INCONCLUSIVE**.
A generic parser/CLI rejection is NOT accepted as a semantic safety negative;
no-clobber is separately demanded. No `source-issued authority` or original
physical witness can be created by host-generated receipt labels.

## Synthetic-only offline verification

```sh
python3 tests/t1014/test_native_bridge.py
```

This runs only stdlib Python and disposable, T-authored mock files. It does
**not** run CMake, NewLang compiler, native C17 or any original R case.
The mock helper intentionally forges a syntactically complete receipt; that
proves the schema can be satisfied but also demonstrates why this check is
**not physical attestation**. Existing fixture `.nl` files are data only and
never passed to NewLang. Test data and fake oracle contain no P or R secrets.

The initial local result was **24/25**, with `test_07_mixed_cohort` refusing
the mixed case for the correct reason but a different diagnostic string than
the test expected; the initial output is preserved in `first-local-red.txt`.
The local selftest assertion alone was corrected, with no bridge code change;
subsequent **25/25 PASS** is recorded in `test-results.txt`. Those are new
T1014 results, **not** T1005's historical 16/16 tests or ZIP source hashes.

## Stop / value risk

This **source-publication only** branch is not a production snapshot and is
not eligible for merge automatically. No PR/Actions/CI/R run may be started
under C113; any extension needs a new C exact-HEAD release and reviewed
prospective P-owned freeze. The added bridge, immutable ledgers, runner/guest
trust review and human test-maintenance closure can cost more than they save.
The §1.1 library-author **and ordinary-safe caller** human UB-prevention
re-audit locality relative to well-factored C remains **UNMEASURED**;
Product GO PAUSED and H1 HOLD.
