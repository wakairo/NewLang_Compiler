# P651 — two original lexical Pairs, one Copy IF, two closed writes

Exploration candidate only. Draft17.32 semantics are unchanged. The fixed
baseline is `f58f1b166780569704ace110d74110ede07b86d2`; no changes to PR649 or
its sources/oracles. `freeze.json` and `STAGE0.md` were recorded in separate
commits before Production edits. This directory contains copies of the frozen
P647 inputs plus a new, pre-execution FALSE/value variation.

## Admission and physical execution

The original control repeats ordinary CHECK0/public source-issued owned,
followed by CLI4 `V1-BACKEND-UNSUPPORTED`, no C bytes. The first private rejection
is the single-scope frame check; the later one-root and arm place-prefix
assumptions also matter. The FALSE variation's baseline CLI4 was a prediction,
not a separately executed baseline measurement.

The new private consumer is a read-only, source-ordered two-row cursor. Each row
holds only existing original root/child/incarnation, ValueID and ValueFact IDs.
It authenticates both arm worlds using the existing parent-owned origin record,
checks the full entry place/value/binding prefix, maps actual source Pair
constructors to original dependent fields, and advances through two already
issued closed scope/replace transitions. It correlates old/new child and parent
facts, original refs and scope closure, then checks the final consumed bindings
and exact recomposed field IDs. The first new value must resolve to this actual
IF; the second must resolve to the first accepted loan's actual old Copy result.
Equal scalar numbers are insufficient to establish this source correspondence.

No world cloning/replay, semantic mutation, fresh ID, ownership permission,
registry, public API or issuer is added. There are no explicit LifetimeDomains,
Allocation regions, heap roots or nonCopy owners in these sources. Existing
one-Pair admission remains the first path. The old arm-local consumer is reused
with an explicitly validated place-prefix length (two roots add four children).
The actual C emitter needs no change: it already emits two distinct original
source-symbol carriers and the correct sequential old-value load/write/result.

The accepted finite class requires two original two-u8 Pair constructors before
one pure u8 Copy IF, two sequential closed direct writes to field index 0 (one
per original), the old-result transfer, and subsequent reads of all four fields.
The usual finite scalar/quantity checks remain. No names or literal values are
special-cased. Calls, a second IF, additional arm locals and general graph/effect
composition remain outside this proof path. Recorded fact-order checks are
intentionally strict: changes to checker issuance require renewed audit, not
relaxed acceptance.

## Evidence and tests

| Source | Initial original Pairs | Actual selected value | Old Copy results | Final original Pairs |
| --- | --- | --- | --- | --- |
| one_if_control.nl | (14,181), (33,207) | TRUE → 72 | 14,33 | (72,181), (14,207) |
| false_variant.nl | (46,165), (108,222) | FALSE → 17 | 46,108 | (17,165), (46,222) |

Original root places/incarnations are 3 and 6, dependent children 4/5 and 7/8.
The first write changes parent fact 3→13 and child 4→12; the second parent
6→21 and child 7→20. Sibling child facts remain 5/8. Closed scope IDs are 1/2.
These numbers are observed output, not admission constants.

`gate.c` loads the genuine source through parser, registered checker and owned
main-call artifact after destruction of the input AST/parser. Public view APIs
produce the readback JSON. Existing semantic validation and the private backend
consumer revalidate each original artifact. Fifteen test-owned corruptions
exercise wrong original parent/child/base/index, old/new ValueID, child/parent
pre/post fact, ended-root currentness, active scope, redirected arm context,
wrong arm ordinal and invalid entry root/child values. Each is rejected, restored
and revalidated, with context/node fingerprints unchanged during each probe.
Wrapped allocation functions observe zero allocations in the private walk.
The fingerprint covers semantic tables and checked node arrays; it is not a
serialization integrity proof or arbitrary-pointer corruption model.

`native_test.py` invokes ordinary `newlangc --check` and `newlangc SOURCE`, checks
deterministic nonempty generated C, and compiles plain and passive variants at
O0/O2 under `-std=c17 -pedantic-errors -Wall -Wextra -Werror`. The passive variant
only captures addresses and reads actual original carriers/fields (including
volatile reads), checks both old values and both sibling fields before/after
each real write, and checks the source field-read results. It never initializes
guest values or fabricates authority. A deliberately wrong observer expects
exit91 and is labelled synthetic, not a NewLang source safety rejection.
ASan/UBSan configurations sanitize both the host compiler/probes and these
generated native programs.

The unchanged two-IF and extra-arm-local inputs remain CHECK0/CLI4/no C;
wrong quantity remains semantic3 `USIZE-LESS-THAN-TYPE` with rollback and NULL
artifact. The existing P514 allocation injection is reused unchanged on this
new complete source: 357 registration and 203 main-call failure prefixes must
roll back, with NULL output on main-call failures. The read-only consumer itself
has no allocation failure site. Existing P517/P605/P626/P628/P633/P634/P636
hostile/one-Pair regressions remain registered and unchanged.

## Reproduction

Use the repository's locked SDK for the normal full build. For a local
LLVM-independent frontend/C17 reference build (PARTIAL, not the FULL5 matrix):

```sh
CC=gcc-13 cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
ctest --test-dir build -R '^p651_' -V
./build/newlangc --check tests/p651/one_if_control.nl
./build/newlangc tests/p651/one_if_control.nl > one-if.c
gcc-13 -std=c17 -pedantic-errors -Wall -Wextra -Werror -O2 one-if.c -o one-if
./one-if
```

Repeat with Clang and `-DNEWLANG_SANITIZER=address` / `undefined` for sanitized
host and guest. The four new CTests use the configured compiler. Full remote
GCC Debug/Release, Clang, ASan/LSan and UBSan, plus the separate PARTIAL workflow,
must be recorded at the frozen DRAFT PR head; local retained Clang23.1.2 is
distinct from CI's locked23.1.3. Local raw logs/summary and actual generated C
are under `evidence/`; workflow run links belong in the terminal Issue report.

## Limits and goal alignment

This is an in-process private proof consumer over a genuine checked artifact
and trusted retained buffers/parent issuance records. It is not an independent
public proof format, universal hostile-host validation, formal proof, arbitrary
multi-root join or general CFG. Remaining risk includes the sizeable finite
frame audit and tight coupling to source-issued bookkeeping. Fresh independent
R is for Coordination to decide. Actual emitter code is unchanged; the new
trust decision is authenticating an additional finite frame before emission.

Compass → Product Strategy §§1.1/1.3 and Process §§4.2/4.3.1 support this bounded
reusable compiler investment. Historical Draft17.32 §§17.4/27.3/27.4 are KEEP;
no source admission semantics are selected or revised. Alternatives of dropping
the scope guard or inventing expected C literals are rejected; general CFG or
new issuer work is deferred. Technical I evidence is measured here; S remains
bounded test evidence, D includes about 430 added private consumer lines, and
library-author/caller UB-prevention audit locality V remains UNMEASURED. No new
goal departure is identified, but fixture-class maintenance cost remains a
separate concern. Product GO scoring remains PAUSED, H1 HOLD. This is neither a
library product success nor a reason to restart H1. DO NOT MERGE.
