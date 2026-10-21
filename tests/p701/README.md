# P701: ordinary stateful Copy return to a real NewLang caller

Base `fe3e0b81d64246df14e2ec4bb76a137630582f17`, canonical Draft17.33.
Default normal CLI; no new option, source spelling, ref/owner authority or ABI.
The original frozen P700 sources retain their exact SHA256. Three independent
variations change nominal/field names, declaration/initializer order, values,
old versus current return and three sequential changes. See `freeze.json` and
`native-oracles.json`; numeric oracles describe expected values, not host inputs.

```
newlangc --check tests/p701/internal_two.nl
newlangc tests/p701/internal_two.nl > program.c
gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror program.c -o program
./program
```

The program prints nothing and exits0. The stronger tests observe the real
callee changes, old Copy forwarding, real typed return/call result and caller
store, original physical addresses and untouched sibling. Exit0 by itself does
not communicate those values. This is not a public safe library or Product GO.

## Evidence authority and lifetime

`copy_body.h` exposes borrowed historical views and an independent validator.
Actual zero-formal source-registered u8 calls capture ordinary nominal placement,
field reads and ordered Changes at semantic issuance while roots/children are
live. Each Change retains before/after worlds and immutable original root,
child, value and scope anchors. The exit is captured **before** lexical EndRoot.
The completed checked graph is separately sealed after successful checking,
retaining body/source identity. The seal alone cannot establish past liveness.

The validator authenticates the actual registered call, body/retained source,
complete node graph, typed result and each event in evaluation order. It tracks
all admitted original records through initialization/changes/reads, compares
parent/child current facts, siblings and Copy issuance identities, and replays
each Change with the existing fixed-field kernel on private clones. Captured
scopes and values are checked against private immutable issuance anchors. A
matching scalar or numeric ID in another world is never a receipt. Copied public
views are untrusted. Observation does not certify; valid history does not grant
currently-live memory, Allocation, Domain or safe-ref authority.

The artifact owns checkpoints/graph and borrows its owning caller; registry
plans/source are retained under existing function-body lifetimes. All data is
freed on failure/destruction. This is an explicitly expanded, source-issuer trust
boundary; arbitrary jointly rewritten private issuer storage is not an external
proof format. There is no deserialization, signature, hidden registry or host
world import. Fresh independent R is still required by Coordination before main
adoption. The tests here are P-owned and do not substitute for that R gate.

Snapshots avoid inventing a general delta interpreter but cost growing-context
memory per event. Existing semantic/checked-node budgets bound storage and work;
OOM/resource failure is explicit. Pure scalar callees do not create a receipt.
No exact one/two-write count, source/type name, value or node-ID whitelist exists.
Three changes use exactly the same validator. Arbitrary caller ref formals,
branch/owner joins, general generic/Array and foreign stateful export are outside
this implementation. General non-Copy and heap originals are not constructed.

## Actual lowering and tests

The shared ordinary call gate first requires the authentic temporal validator
for a captured stateful body (even if a damaged graph resembles a pure body).
Existing C record/ref/replace/return emitters then generate the actual callee,
old-value temporaries, source reads and stores. Caller code receives a real
`nl_call` value. No checker-known result is substituted for an executed call.
The separate experimental foreign export remains Copy-body-only and refuses
these sources. Unit-only process entry remains unchanged.

`public_probe.c` destroys input AST/source before checking actual main(), then
uses public APIs for validation and numeric/identity observation. `native.py`
compiles untouched generated C and a separately labelled passive observer copy
at O0/O2. Observer code only reads guest memory, compares expected values and
tracks its own call count; it never sets guest data or permissions. The observer
requires actual source carrier/call/forwarded-binding structure and detects an
equal-value literal replacing the call. GCC/Clang native and sanitizers are
separate from static CHECK and owned validation.

`gate_test.c` mutates test-owned real artifacts and copied public views. It
rejects foreign/missing/reordered/duplicated events, roots/children/indices,
incarnations, stale pre/post facts, sibling corruption, wrong scope, equal-scalar
wrong Copy return, call/body/source mismatch and missing evidence. Restored
artifacts pass. Validator fail-at-N is27/53 positions for the original cases;
registration267/436 and actual main-call213/348 prove unchanged public state,
NULL failed artifact and cleanup. Emission after admission has0 wrapped
application allocations; `/dev/full` write failure is detected. Named-output
success matches stdout bytes; semantic/backend failure preserves sentinel output.

Semantic negatives cover wrong write/return type, read-formal authority and
scope escape. The read-only field-loan example stops at the **existing source
profile**, not a semantic permission proof. Richer callee IF is CHECK0/backend4.
The negative-source hashes/status/category/code contracts are separate from the
five pre-edit positives; before/candidate diagnostics are retained.

## Preserved historical test contract

The first local full run was378/380. P605 hashes the complete private
`semantic_internal.h`; the required new artifact attachment changes that header.
The initial failures/log are retained. P605 now strips exactly the new two-field
attachment at the exact `NLCheckedFragment` record plus its previously authorized
P633 block, then requires the **same original entire-header hash**. Original
P605 frozen manifest, sources, numeric/native oracles and arithmetic issuer and
public proof files are unchanged. P634's real-loop attacks retain old controls
and add missing/duplicate/changed P701 attachment rejection. Unrelated header or
arithmetic proof edits still fail. This is an active historical expectation
migration, not bypassing arithmetic revalidation.

The final local GCC13 frontend suite is380/380. Targeted Clang ASan+LSan and
UBSan and strict GCC/Clang O0/O2 native pass. Exact candidate/full CI evidence is
reported in Issue701 after the one permitted natural FULL5+PARTIAL pair; stored
local results must not be mistaken for remote CI or independent R verification.
