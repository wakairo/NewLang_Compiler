# P718 — one normal IF edge and a distinct unit RETURN (candidate)

DRAFT / DO NOT MERGE. Released base/main
`345e58be1bf805f2bc12cc802608f344351e7cd8`, CURRENT_SPEC→Draft17.33.
C's M4 RELEASE is Issue718 comment6331091334. Exact integrated-base push/attempt1
FULL5 39694207660 and NO LLVM PARTIAL 39694207657 were independently queried:
completed/success, correct SHA, all required jobs successful. M3/R716's adopted
finite **two-normal** boundary is retained, not claimed to cover this addition.

## Freeze and baseline

Before any Production edit or execution of the candidates, `freeze.json` recorded
UTF-8 SHA256 and independent numerical/path oracles for two distinct families:

| family | original | normal edge | normal current / old / sibling | terminal edge |
|---|---|---|---|---|
| return_true/false | SensorState.level, declared index1, initial57 | else | 91 / 57 / marker206 | then: return unit, zero store |
| continue_true/false | QueueTuning.quota, declared index0, initial23 | then | 38 / 23 / stamp144 | else: actual store219, old23 discarded, return unit |

They differ in declaration index, constructor order, condition/local order,
values and edge orientation; each has actual TRUE/FALSE source variants.
Unchanged base: all four SOURCE/CHECK0, C17 CLI4
`V1-BACKEND-UNSUPPORTED`, zero output. The unchanged historical
`tests/p713/sole_normal.nl` has the same boundary. Actual diagnostics and hashes
are in `evidence/baseline.json`; there was no source authoring calibration.

Five frozen negatives distinguish wrong returned type (`P9-RETURN-TYPE`), wrong
terminal replacement type (`P3-TYPE-MISMATCH`), scope escape
(`P8-EXIT-DEPENDENCY`, including attempted returned ref) and unknown projection
(`FIELD-UNKNOWN-FIELD`). Both arms are checked even if source constants determine
one runtime outcome. Sources/oracles are never changed to fit emitted C.

## Source-issued mechanism and public verification

Existing `source_if` already rechecks the sole NORMAL source arm in the common
world, rather than importing arm-private state. Existing `source_return` ends
bindings and verifies function exit; `join_if_function_exits` separately checks
terminal function boundaries. Their selected semantics are unchanged.

This candidate captures before RETURN cleanup and after the real RETURN/exit
record, and captures the genuine common-world re-execution. It reuses M3's
source graph seals, original-entry/type/frame checks and ordered direct
Change/read replay, with explicit per-event **issuing artifact and stream**.
Three streams are distinct: source fork0, source fork1, actual common replay2.
Only the sole normal stream supplies continuation root/child/facts/Copy result.
Terminal result is **unit with no ValueID**; it is never a synthetic normal result.

The added `NLIfReturnView` and `nl_checked_if_return_*` functions in
`newlang/if_field.h` expose borrowed historical observations. They confer no
present ref, Storage, Allocation or Domain authority and are not wire proofs.
Owner lifetime and immutable private issuer anchors/seals remain trusted.
Observation alone does not validate a receipt; a NULL validator candidate means
validate the actual issued receipt, not skip validation.

The public validator:

* binds registered source/body, actual owner and complete sealed parent/arm
  graphs, condition, IF, exact normal/terminal indices and source RETURN node;
* independently walks both source arm graphs and the common normal replay,
  including every checked node, scoped ref, event order, old/incoming Copy,
  original root/child/incarnation/index, field facts and unaffected sibling;
* correlates the independently checked normal source graphs by ordered source
  node kind/span/name/type, not post-fork ID equality. A final whole-state
  comparison is a consistency check after source/world-qualified walks, not an
  authority transfer. No code assigns a fork's packages to the common world;
* checks the exact function-target/return-state objects and both actual control
  exit sets. A restrictive replay of the **existing** end-bindings operations
  verifies the pre-return→ended state: all references ended, scopes closed,
  Copy+Discardable lexical locals only, dependency check, fixed detach/value end,
  consumed bindings, and core function-exit validation. No heap/nonCopy teardown
  or new destruction rule is inferred;
* validates post-IF reads against the normal-only checkpoint and rejects missing,
  foreign, duplicated/cyclic, stale or reclassified normal/terminal evidence.

M3's existing public validator still requires TWO normal arms and rejects an M4
receipt. It has no one-normal bypass. The new driver gate cannot fall through to
pure IF emission after a failed/OOM return certificate. Constructor→entry binding
→original C carrier checks are shared with the existing M3 gate. Existing unit
RETURN emission is reused; the C IF assigns its result only on the normal edge
and emits real `return;` on the terminal edge. No cached truth chooses the branch.

## Meaningful first failures and fixes

* First backend edit mistakenly introduced a duplicate RETURN switch case. GCC
  rejected compilation. The new case was removed and the pre-existing unit
  RETURN emitter reused. A later helper was briefly not connected to its caller;
  strict `-Werror=unused-function` rejected that build. Records are retained;
  neither is a NewLang source/semantic failure or native evidence.
* During this task's **test-owned private producer** mutation, exchanging complete
  trace lists between two arms with colliding numeric node/value/fact IDs was
  accepted by the development candidate (validator0/admission1). The original
  graphs emit different literal bytes38/219; this exposed a missing stream-origin
  check in shared replay. It was not a demonstrated source miscompilation or
  forgery through opaque public APIs of adopted main. The final code captures
  and requires exact event artifact/stream and checks incoming/old Copy values
  against the actual source-issued node world. The same swap now rejects; M3
  valid inputs still pass. `evidence/first-foreign-stream-swap.log` preserves the
  failure. Independent R must assess this strengthened shared trust boundary.

## Verification and reproduction

```sh
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
ctest --test-dir build -R '^p718' -V
./build/newlangc --check tests/p718/return_false.nl
./build/newlangc tests/p718/return_false.nl > generated.c
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 generated.c -o guest
./guest
./build/p718_public_probe tests/p718/return_false.nl
```

`native.py` runs **unchanged** emitted C separately from a passive observer at
O0/O2 with the configured GCC/Clang (and ASan/LSan or UBSan as configured).
Observer locals point only to the actual source-constructed record, and only
read its bytes/addresses and actual captured old Copy. Observer counters record
chosen edge, stores, pre-return and post-IF phases; after the real callee returns,
the native entry checks terminal bypass versus successful continuation. No guest
value, condition, store, ref or ownership is supplied by the host. No pointer to
an ended local is dereferenced after RETURN; only observer counters survive.
Original P713 source/hash and numerical behavior also receive this coverage.

Synthetic C controls alter condition, actual guest field address, old capture or
remove RETURN. Wrong condition/field/old are native91 under the same observer.
Removed RETURN can produce native91 with GCC or strict Clang compilation refusal
(`native: NOT RUN`): the output explicitly distinguishes these stages. Never call
a C compile refusal a measured wrong guest exit. The normal CLI's native exit0
is unit completion; observer instrumentation is separate testing evidence.

Public/producer negatives mutate root/child/index/incarnation/fact/world, equal-byte
but distinct Copy origin, incoming/result, source owner/stream, cycle/missing
trace, RETURN/normal classification, live terminal original, sibling or ref scope/
write access. Two independently produced sources with the same numeric IDs do
not share authority. Source semantic failures preserve snapshots/null artifact,
zero C, and prior named-output bytes. Registration/call/validator/admission/driver
fail-at-N OOM verify rollback and resource reporting; host parser/checker CLI3 OOM
remains distinct from explicit backend resource CLI1. Emission after admission
allocates nothing; `/dev/full` fails honestly.

The only historical test expectation migration is P713 `sole_normal` backend4→0;
its original source, SHA, old freeze/evidence and old README remain historical.
The new P718 harness verifies the original source's proof and native behavior.
No P605 historical hashes/normalization, arithmetic issuer, toolchain lock,
workflow, canonical or Process documents are changed. Local Clang23.1.2 is
separate from unchanged CI pin23.1.3. Frontend-only evidence is PARTIAL, not FULL5.
Final exact candidate CI links/conclusions are recorded in the PR/Issue report.

## Scope, cost and alignment

Finite candidate: one original lexical 2-u8 nominal field, two source arms with
exactly one NORMAL edge and one explicit **unit RETURN**, genuine direct scoped
Change on the normal edge, optional direct Changes on the same target before
terminal RETURN, zero-argument source body with no caller-prefix live authority.
The normal Copy result and source reads are preserved; terminal originals end.
Not claimed: arbitrary typed early-return callees, caller ref formal/heap teardown,
nonCopy availability/owner return, pure zero-Change early returns, nested IF/calls,
loops, zero-normal CFG or general SSA/effects/FFI. These need further composition
work; they are not declared invalid language semantics or implemented by fixture
exceptions here. No extra source rule or ref/Domain authority is introduced.

Snapshot/graph memory and replay scale with captured event count and semantic/
source graph size, using existing finite budgets and explicit OOM. The checker
already executed the two forks plus common normal replay; new evidence makes
that cost and correspondence inspectable. Representative receipts track 11/13
historical contexts including the two existing control-exit snapshots, excluding
owner/final caller context and transient replay allocations. Flat anchors are
119632/163576 bytes, graph seals86400/101952 bytes; **nested semantic/registry/
source storage is additional**. Per-event validation repeats the full validator.
This is not a compact general IR or a scalability/product guarantee.

Historical §4.2 disposition: **KEEP** Draft17.33 §§13.5a,16–17,18.8,27.2–27.4,
27.9 and DI-001/006/007/017. Existing normal/terminal edge separation, field
Change/currentness and loan nonescape govern; final/general loan spelling and
broader owner/CFG remain DEFER. M709/C M3 and this M4 RELEASE authorize faithful
implementation; old unmerged PR510 is not imported as source/proof authority.
Inaccessible full historic chat is not claimed audited or nonexistent.

Goal Alignment: selected v0 source can now express early exit without manually
hoisting conditional stores or simulating RETURN in C. It preserves programmer
control flow and moves bookkeeping into source-issued checks. This is compiler
capability evidence, not demonstrated author+ordinary caller UB-prevention audit
locality benefit or ordinary-safe library completion. Material concerns remain
snapshot/proof maintenance cost and limited function/owner composition. Product
GO active scoring PAUSED624 and H1 HOLD268 stay unchanged. C decides fresh BLIND R
and adoption; P never starts another Track, merges main, or self-closes the Issue.
