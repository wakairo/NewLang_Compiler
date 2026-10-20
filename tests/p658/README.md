# P658 USER-CLI-1 Stage0 — source eligibility gap (NO PRODUCTION CHANGE)

Fixed adopted main: `9da4b4ce6163592f576c5a2ecb937ff6e230e6f8`.
Canonical: CURRENT_SPEC → Draft17.32. This is a P-owned Work/Codex checkout;
PR652 and paused R655 were not execution bases or fixture sources. No private
checked-evidence mutation/restore test was run or added.

## Fresh workload and pre-execution oracle

`admit.nl` models a single dispatch-admission snapshot: count the protocol
header, subtract already reserved budget, leave at least one spare byte, and
write admit/defer to the original record's `ready` byte, retaining `channel`.
The input configuration is expressed as source constants; no dynamic input,
I/O, queue, allocator or library abstraction is claimed.

The intended values were frozen in `freeze.json` **before any CLI execution**:
27+5=32, 96−16=80, 32<80 TRUE, pending2→admit1, old2, channel9 unchanged.
The independent equality-boundary configuration has 75+5=80, 80<80 FALSE,
pending2→defer0, old2, channel9 unchanged. This adds actual quantity arithmetic
and application-labelled state; it is not a renamed P651/R656 fixture.

**The prediction was wrong.** I extrapolated the adopted two-u8 nominal
construction/backend shape to its scoped field source. That missed the
canonical's exact Pair source restriction. The original frozen program has
not been changed to `Pair`, `left` or `right` to make it pass.

## Actual first boundary and STOP

All five sources parse successfully. Both intended normal cases instead fail
function-unit registration with `NL_CHECK_SEMANTIC_UNSUPPORTED` (2), category
`unsupported`, `FIELD-PROFILE`, bytes [500,514), source12:35–12:49 at
`dispatch@ready`. Ordinary CHECK and C17 CLI both return3, C/stdout0bytes.
No source-issued main-call owned artifact, published original root/field proof,
physical C carrier or guest native result was obtained.

This is **not** a semantic safety error or demonstrated backend bug. The
Dispatch field operation is outside the currently selected actual-source
profile. Draft17.32's Draft17.20 clarification (near line537), §16.1 (near6387)
and §17.1 (near6957) explicitly select registered `Pair {left:u8,right:u8}`
and the separately completed recursive categories. Conceptual arbitrary
fixed-field examples do not broaden this surface. `src/fixed_field.c`'s
`nl_fixed_type` checks the exact Pair name and left/right field names, and
`nl_fixed_attach` only issues children for supported types. Conversely the
two-u8 AVS constructor/backend shape accepts other names; that distinction
explains the first user-facing friction.

Generalizing names here changes source eligibility and source-issued dependent
field admission. Removing the name guard is not an authorized backend fix.
Issue658 says STOP for a new source rule or authority/provenance mechanism;
therefore no Stage1 implementation, PR, full regression or Actions run follows.
The pre-execution hypothesis that arithmetic nodes might next be rejected by
the private C17 frame is static inspection only, **not an observed blocker of
this original program**. The frozen two-write variant is masked by the same
earlier field admission; it is not evidence of a legal-source backend refusal.

## Actual semantic negatives and cleanup

| Source | Actual result before field projection |
| --- | --- |
| wrong_quantity.nl | registration semantic1 / CLI3, USIZE-ADDITION-TYPE, 7:36–7:37, C0bytes |
| over_reserved.nl | registration semantic1 / CLI3, USIZE-SUBTRACTION-UNDERFLOW, 8:37–8:38, C0bytes |
| route_change_unsupported.nl | same FIELD-PROFILE as original; later scope/backend not reached |

`frontend_probe.c` uses only public parser, registry and snapshot APIs. It
compares all twelve public snapshot counters/identities across the real failed
registration, then destroys context/tree/parser/source normally. Every case
keeps the public snapshot unchanged. No issued evidence is altered, replayed
or restored, no source-less value or permission is seeded, and no private
context layout is accessed. This is bounded observable rollback, not proof of
all private state or exhaustive allocator/I/O failure coverage.

The exact ordinary CLI and probe runs are recorded in `evidence/baseline.json`
and `evidence/frontend-and-cleanup.json`. GCC13 and Clang23.1.2 host builds
agree; the latter uses ASan with `detect_leaks=1:halt_on_error=1`. No host
sanitizer error or leak was observed. A successful host probe means the
expected frontend refusal was verified; **it is not guest native execution**.
GCC/Clang O0/O2 guest, public successful owned observations, C17 physical values,
UBSan, FULL5/PARTIAL CI are NOT RUN / NOT REACHED for this workload.

## Reproduce the unchanged compiler and public failure observations

Use the README frontend-only route (NO LLVM / PARTIAL):

```sh
CC=gcc-13 cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target newlangc --parallel 2
./build/newlangc --check tests/p658/admit.nl
./build/newlangc tests/p658/admit.nl > /tmp/p658.c
# Both exit3; /tmp/p658.c is empty, FIELD-PROFILE at12:35–12:49.
gcc-13 -std=c17 -pedantic-errors -Wall -Wextra -Werror -O0 -Iinclude \
  tests/p658/frontend_probe.c -Wl,--start-group build/libnewlang*.a \
  -Wl,--end-group -o build/p658_frontend_probe
./build/p658_frontend_probe tests/p658/admit.nl
```

Repeat the host build with Clang and `-DNEWLANG_SANITIZER=address`; compile the
probe with `-fsanitize=address -fno-sanitize-recover=all` and run with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`. These are public API failure
observations, not an alternative path for admitting the original program.

## Decision and next investment

I-GAP / SOURCE ELIGIBILITY HOLD, **not USER-CLI-1 PASS**. Historical design audit:
KEEP Draft17.32, no semantic/source change; no nominal naming extension selected
by P. Goal is ordinary user capability, alternatives are exact existing Pair
surface versus a separately selected domain-named two-u8 record surface. The
latter requires M/Coordination scope selection before a future P implementation.
Do not infer that general CFG, ownership or heap machinery is necessary from
this finding; no such requirement was demonstrated. Implementation cost for
that separate selection has not been measured.

The ordinary-user friction is concrete: constructing a named two-u8 record does
not imply its labelled fields can be read/loaned. Current application concepts
would need manual mapping to left/right in a new separately frozen experiment.
Library-author/caller UB-audit-locality value remains unmeasured. This is not a
Product GO finding, H1 restart, main compiler safety regression, or justification
to use PR652. Product GO scoring remains paused and H1 remains HOLD.

Only this Stage0 source/observation record is retained. Production, canonical,
existing fixtures, toolchain, workflows and CMake are unchanged. Coordination
decides the next source contract or separately authorized program.
