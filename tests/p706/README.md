# P706 / v0-M2 — caller-original write / old Copy return

Candidate evidence only; DRAFT / DO NOT MERGE. Base: Compiler/main
`45aa20cc1584a24c9d1a79ec99e034c7fb13c50e` (Draft17.33).
The fixed-main push FULL5 39593209910 and PARTIAL 39593210017 were independently
rechecked: exact SHA, push, attempt1, completed/success, before M2 edits.

## Source / oracle freeze and first barrier

`frozen.json` records two newly authored exact `.nl` sources, SHA256 and numerical
oracles before the first execution and implementation. Stage A Issue comment
6315339895 records those hashes before candidate execution. `evidence/baseline.json`
is the actual unchanged fixed-main binary result: both positive CHECK0 and
C17 CLI4 / V1-BACKEND-UNSUPPORTED / 0 stdout bytes. Negative source bytes and
first diagnostics are preserved too. No preexisting source or oracle was changed.

- `settings`: original index0 port31→167, original index1 mode94 unchanged;
  real returned old31 feeds a second original's index1 mode52→31; its port18 stays.
- `window`: reversed constructor field order, index1 ceiling203→14, sibling
  floor6 retained; explicit ordinary return of old203; second original index0
  floor71→203 with sibling ceiling45 retained.
- `additional-freeze.json` independently freezes further source **before its
  first execution**, after the initial implementation. `relay` calls the same
  helper on distinct originals: a.active80→0 returns80, then b.idle160→80 uses
  that actual Copy argument and returns160; a.idle39→160 uses the second result.
  It prevents a representative call's known scalar from replacing runtime args.
  This supplementary example is not misreported as unchanged-main evidence.

## Source-issued history and safety boundary

The existing M1 receipt attachment owns the M2 evidence; no hidden registry or
new source rule. `nl_copy_body_parameters` runs **after real actual evaluation
and fresh formal binding, before the real body**. It records the actual caller
loan/field, exact argument nodes and freshly allocated formal symbols, then
clones the active checker world (not the fragment's as-yet-uncommitted public
context). Original root + child/incarnations, current packages/facts, sibling,
active scope and exact ref provenance/access are retained. The existing checker
continues to perform Change through `nl_fixed_change`, and its usual exit,
parameter cleanup and candidate commit implement the actual caller post-state.

Each issued Change uses M1's before/after replay. A live exit checkpoint precedes
EndRoot, a handoff checkpoint follows callee cleanup. Outer caller scopes remain
active at that handoff; callee-created scopes may not survive. Private array
anchors protect exposed historical entry/handoff views. The enclosing completed
body seals the **whole caller graph**, because it was still under construction
when the inner call returned. Actual source/call/body/world identity is required;
matching numerical IDs or equal scalar values are insufficient.

The public validator walks the full callee graph in evaluation order, consumes
all events, authenticates actual→formal substitution, replays Change on a private
clone and checks root/child post-facts, sibling state, old Copy identity and
return graph, caller-place frame and scope cleanup. Full graph seals are trusted
issuer-owned origin anchors; equality with a mutable public node is not a
self-issued authority. No host may submit arbitrary worlds as source authority.
The public views are **historical observations, not presently usable refs,
Allocation or LifetimeDomain capabilities**, and are not a serialized proof API.
Copied views are compared before any foreign-world dereference.

C17 admission requires this validated history for the explicit two-formal
`(ref<write,u8>,u8)->u8` path. C17 calls a real helper with the real caller field
address and real u8 operand. The helper reads the old byte, stores the incoming
byte through the formal pointer, returns the old Copy; normal caller loan result
forwarding and a subsequent scoped write use that returned runtime value.
No test host supplies guest data, permissions or pointers. The M1 zero-formal
path and its foreign export exclusion remain intact.

## Tests and reproduction (frontend needs no LLVM)

Configure/build normally with `NEWLANG_FRONTEND_ONLY=ON`, then:

```sh
ctest --test-dir BUILD -R 'p706_|p701_|p605_|p634_' --output-on-failure
BUILD/newlangc --check tests/p706/settings.nl
BUILD/newlangc tests/p706/settings.nl > settings.c
BUILD/p706_public_probe tests/p706/settings.nl
python3 tests/p706/native.py BUILD/newlangc BUILD/p706_public_probe cc none tests/p706
```

`public_probe.c` parses/registers real source, destroys the original parser/AST,
checks `main()`, and uses public semantic/checked/receipt APIs only to inspect
original identities, pre/post facts, sibling currentness, actual arguments,
Copy origins and scope active-at-call / closed-at-final state.

`native.py` uses the independent frozen oracle. It compiles **unedited stdout C**
and a separately labelled passive observer with strict C17 warnings-as-errors,
O0/O2 and selected sanitizer. The observer captures only real live original
addresses, checks actual helper pointer/operand/old return, untouched siblings
and the other original's physical state. Observer pointers are cleared before
caller locals end. Assertions never modify guest values or supply authority.
Deleting the actual call is a failing instrumentation control. Named output must
be byte-identical to stdout; all negative invocations preserve an old output file.

`gate_test.c` tests the real driver and source artifacts, not a host-created
semantic fixture. It mutates copied public views and test-owned graph/world/event
storage: wrong root/field/index/facts/incarnation, missing/duplicate event,
read/write/scope/provenance, stale read-back, wrong parameter/argument/return,
wrong sibling, equal scalar with different Copy origin and another authentic
source world with identical numerical IDs. Rejection is followed by restoration
and successful revalidation. These are **producer** attacks, not independent R.
Fail-at-N registration, call issuer, validator and repeated backend admission
require explicit OOM, rollback and NULL failed artifacts. A full driver fail-at-N
sweep also requires zero partial C; existing host parser/checker OOM CLI3 and
backend resource CLI1 are preserved and explicitly distinguished. Unbuffered /dev/full
fails emission; after successful admission the host emitter allocates no heap.

Source negatives are classified, not conflated:

| Source | Actual first result |
| --- | --- |
| wrong_value, copy_as_ref, read_scalar | semantic P3-TYPE-MISMATCH, CLI3 |
| wrong_return | semantic P8-BODY-RESULT, CLI3 |
| closed_scope | semantic P3-UNKNOWN-BINDING, CLI3 |
| callee_escape | semantic P8-EXIT-DEPENDENCY, CLI3 |
| read_authority (field read loan) | unsupported LOCAL-LOAN-PROFILE, CLI3 |
| escaped_formal | precision P8-SIGNATURE-PRECISION, CLI3 |
| conflicting_loan | unsupported P5-EXPRESSION-UNSUPPORTED, CLI3 |

The last three are finite source/analysis limits, **not** proof of the intended
full semantic rule. In particular ordinary write refs are nonexclusive; no new
rule banning all shared write aliases is introduced. The private scope/access
mutation tests are a different evidence layer from actual semantic rejection.

## Calibration, costs and limits

The first local malformed-graph test exposed a missing NULL caller-node guard
before public validation; its initial ASan report is retained. The bounded fix
checks NULL before obtaining the call node. This is an implementation validation
bug under a test-owned invalid artifact, not a new language/authority choice.
The first local passive observer retained global observer pointers beyond local
lifetimes; GCC13 O2 rejected this with strict dangling-pointer warnings. The
instrumentation was repaired by ending those observer borrows before function
exit; source, oracle, generated guest code and compiler warnings were unchanged.
Both first failures are stored under `evidence/`, not hidden by CI reruns.
The added driver test initially expected all OOM to be CLI1; the unchanged
driver actually reports host parser/checker OOM at CLI3. The test was calibrated
to require explicit `error(host)` / `OUT-OF-MEMORY` at CLI3 and retain zero bytes,
without changing production diagnostics (first mismatch also preserved).

Additional per-call cost: two full semantic history worlds (entry and handoff),
private flat place/value/scope/binding anchors and a completed caller graph copy,
in addition to M1's per-event worlds, exit world and callee graph. Storage scales
with checker state/event/call graph size under existing resource limits; this is
not a zero-cost/general effect summary or a production performance claim.
There is no new arbitrary-arity ABI, recursive/general call graph, ref return,
heap-owner handoff, branch join, general FFI or generic implementation. The C
consumer retains its existing finite two-u8 nominal carrier/profile restrictions.
Richer selected v0 needs remain on the critical path; they are not abandoned.

Historical audit: KEEP Draft17.33 §§18.2,18.4,18.7,18.8, ordinary scoped write /
Change, DI-001 @ and DI-017 nominal fields. N/A semantic selection: faithful
implementation, no normative change. DI-004 receiver syntax, general ref-field
projection, private-module safe library boundary and FFI stay Deferred. The
unchanged P605 frozen-hash gate still validates protected arithmetic files.
Goal Alignment: this advances reusable ordinary internal function composition;
it is not UB-prevention audit-locality Product PASS, H1, or general v0 completion.
New history/caller trust boundary needs Coordination's independent R decision;
no Track was launched by P and no merge is authorized here.
