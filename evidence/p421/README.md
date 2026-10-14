# P421 — existing-main field + ordinary Copy call composition

**Test-only Exploration / DRAFT / DO NOT MERGE.** Fixed production source:
`6fc8ba26d34a2e2aff0aed92512859ecf40bd0a5`, CURRENT_SPEC → Draft17.30.
FormalProof/main reference `a8de4ebb4099f4742fc161d26867740dcac48414`
is not a proof of this native composition.

Before the first source execution, post-merge main
[Actions 38490106462](https://github.com/wakairo/NewLang_Compiler/actions/runs/38490106462)
completed SUCCESS, all five matrices. PR420 is merged at that exact main;
PR395 remains a separate historical DRAFT on UNMERGED PR384/381.

## Exact inputs and pre-registered outcome

| Source | SHA256 | A left old/new | B right old/new | untouched A.right / B.left |
| --- | --- | --- | --- | --- |
| primary.nl (literal Issue source) | `777aa94fa97b175e86738d0de2872b835b1bab859936646f95e1e7823f2b3194` | 7 / 11 | 2 / 7 | 1 / 9 |
| variation.nl (names and declaration/initializer field order changed) | `2400745b53358845b494232115978b759a6ca4ba6819aa12d0020a3308c4be83` | 80 / 123 | 4 / 80 | 1 / 9 |

Prediction was an uncertain composition frontier, potentially a finite emitter
gap. Actual unchanged compiler: parse/register/CHECK/actual CHECKED-OWNED all
PASS (public API status0, probe process exit0); deterministic CLI0 with no
stderr, C1585/C1587 bytes; strict GCC/Clang C17 O0/O2 native PASS. No blocker
or production patch. This successful first probe ends feature exploration;
only its two inputs, falsifiers, evidence/reproduction and CI are preserved.

## Actual source-owned observations

`field_call_composition_probe` uses public API only, with existing test support
loading actual declarations, disposing syntax and checking actual `main()`.
There are no host-known functions, seed roots, values, Domain or Allocation.
It observes two original independent local Pair places 1/4 and incarnations
1/4 (context-qualified, not asserted universal numeric IDs). The first Change
is field0 in primary / field1 in variation, second is field1 / field0.
Each has distinct fixed child place, valid scoped write loan, nonescape,
exact old result, changed child AND parent ValueFacts and corresponding
post-change source field-read facts. Both loans end; original local roots are
ENDED at function exit, not claimed still live in the final context.

Primary Change A: parent fact1→9, child fact2→8, old package1 (7), new9 (11).
B: parent4→15, child6→14, old package5 (2), new15 (7). Variation changes child
identities with the declaration order and observes 80→123 / 4→80.

The old A package is received in a caller binding, copied into actual package11,
bound to the real fresh formal5, returned as package12, received in a caller
binding, copied into B's new package15. Public values confirm the causal values;
existing `u8_call_checked_probe` authenticates the immutable registered body and
actual/formal/result relationship, and its eight restored artifact corruptions
reject with semantic snapshots unchanged. This reuses driver-private call
validation, not a newly created public general composition certificate.

Public source-issued post-read facts match each Change's parent/child post facts.
Untouched siblings are observed in actual native Pair storage after each write
and at the end. We do not invent a public parent-fact history API or claim a
public temporal sibling certificate absent from the API.

## Native observation and falsifiers

The emitted code actually loads A's old field, passes the returned loan carrier
to `nl_fn_relay(old_a)`, receives its runtime result, and writes that result
through B's independent scoped write ref. The integration requires these C
carrier connections before execution. Plain generated C runs unmodified.
A TEST COPY adds read-only checks of real initial fields, old values, formal,
return/caller receivers, stores, post-read fields and both siblings. A test-only
call counter checks one real callee invocation. No arguments/returns/object facts
are supplied by observers; no guest I/O or exit-result ABI is created.

Two independently corrupted C copies (wrong callee return0; ignore transported
value and store0 to B) each abort at O0/O2 on GCC/Clang, exit -6. Expected values
are independently specified above, not accepted from compiler output alone.

Four source negative observations, all CLI3/C0:
- `loan_read(a@left)`: **parser source-profile** `LOCAL-LOAN-PROFILE`,
  span6:24–6:25; NOT a semantic read-to-write authority refusal.
- B replace new `unit`: semantic `P3-TYPE-MISMATCH`, span8:46–8:50.
- relay wrong `unit` return: semantic `P8-BODY-RESULT`, actual call span7:19–7:31.
- escaping scoped A ref: semantic `P8-EXIT-DEPENDENCY`, span6:13–6:37.

No speculative guard removal, general ref/nonCopy ABI, extra source admission,
heap ownership, LLVM, new effect system or second compiler mechanism is added.

## Reproduce

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
build/field_call_composition_probe evidence/p421/primary.nl
python3 tests/integration/checked_c_field_call_composition_test.py \
  build/newlangc build/field_call_composition_probe \
  build/u8_call_checked_probe gcc evidence/p421
```

Repeat with Clang. Standard five-job PR CI runs the new test and existing full
suite. Its usual CTest success summary hides positive JSON; invoke the Python
command above to print the detailed public observations, source hashes and mutant
exits. P evidence is not a blind independent R report or new formal result.

## History / exploration map / stop

PR395 `4d3f7cbcce76f6cc3c1fc1b7cef62acee407a907` already showed causal A-old→B-new
on experimental stacked G1 via named ref/u8 state-update calls. This gate is NOT
first-ever causality: it confirms the two separately merged main mechanisms
compose without that experimental compiler. Incremental information is limited
but specific: ordinary Copy call input/output can connect main's scoped field
updates. P419's one-level pure nested call is now merged; this fixture exercises
a main call, not another nested-call or ref-helper capability.

Compass→Product Strategy, Process §§4.2/4.3/4.3.1 and Roadmap §§4.0–4.2 reviewed.
Historical gate N/A — test-only faithful observation of selected Draft17.30
fixed-field loan/Change and ordinary Copy call semantics. KEEP source semantics,
trust guards and original root identities. No Ledger/spec update needed.
No new goal drift identified within the authorized final Copy gate; technical
PASS does not establish ordinary independent safe-library user benefit or a
C/Zig/same-topology Rust burden advantage. Product remains HOLD. Further Copy
smokes have little value; Coordination's next choice is a separately scoped real
application operation or the core operation it needs. Do not initiate it here.

H1 original five fallible roots/6+4+2/B.prev=B/B+dst nonCopy/failure0–5/B1–B7,
Node M414 public/native certificate HOLD, and STB six obligations remain unchanged.
STOP after one terminal Issue report; no merge, self-close or other Track launch.
