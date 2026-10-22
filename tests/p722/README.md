# P722: caller-original field + a callee-local RETURN

DRAFT / DO NOT MERGE implementation evidence; canonical Draft17.33 unchanged.
Base `5ce78224383948aad65133081494d43e30cc0835`, tree
`c5cfb350d2b05ff6b82a9e1136ef892c0fb888b8`. Integrated base push/attempt1
FULL5 39722593501 and NO LLVM PARTIAL 39722593509 were independently queried:
both completed/success on that exact SHA before implementation.
During work, Coordination merged Roadmap-only PR723 to main `ddc6bc478583deff13da19a0f6426d19c8aeb7f8`; its #722 clarification explicitly retains the released implementation base above. This branch is not rebased; current Roadmap is read as documentation, not a new compiler authority.

## Freeze and first boundary

`freeze.json` retains the pre-edit source bytes/SHA and independent oracles.
The four sources vary nominal, declaration index, constructor order, incoming
bytes, condition direction and whether RETURN precedes or follows a Change.
`evidence/baseline.json` records all four unchanged-base CHECK/ordinary CLI
results: exit3, zero stdout, P13-JOIN-PRECISION at the actual helper call.
Parsing and independent definition checks passed; joining the changed
caller-prefix aggregate against the function RETURN was the precision gap.
This is not source illegality or a backend-only refusal.

| Source | Callee path | Caller original field after call | Sibling | Caller archive |
| --- | --- | --- | --- | --- |
| before_true | RETURN before write | gain=41 | tag=193 | tag=41 |
| before_false | normal write | gain=87 | tag=193 | tag=87 |
| after_true | normal write | ceiling=62 | seal=151 | seal=62 |
| after_false | write then RETURN | ceiling=219 | seal=151 | seal=219 |

The archive is a disjoint lexical original and receives the actual postcall
Copy read, not a test-supplied value. These are lexical Copy records, with no
heap Allocation, LifetimeDomain, owner, FFI or product/API claim.

## Implementation and trust boundary

- Reuse M2's genuine scoped actual -> formal mapping and complete caller graph
  seal for `(ref<write,u8>,u8)->unit`. Definition checking still uses ordinary
  symbolic ref sites; a definition has no assumed caller-original identity.
- Reuse M4's actual fork worlds, return-live/ended snapshots, original normal
  fork and common-world source recheck. Direct formal Changes use the actual
  formal ref's place/incarnation/access/scope; no new capability is created.
- At function exit retain a **fresh Unknown field and recomposed parent**.
  The normal-only snapshot is not the caller postcall state. Core dependency
  checking and fixed-field Change invalidate old child AND parent ValueFacts;
  sibling, root/child incarnations and caller-prefix scopes remain intact.
- New public `NLCopyUnitView` is borrowed immutable history, not current
  reference/ownership authority. Independent validation checks exact source,
  owner, call, full caller/callee graph seals, parameter substitution, all
  three ordered event streams, source-issued ref/old/incoming Copy origins,
  callee RETURN target, local teardown and conservative postcall replay.
  A complete typed prefix graph walk also checks the actual usize operands
  and bool comparison. Actual caller reads use postcall facts, not fork IDs.
- RETURN teardown starts at callee binding/place/scope floors. It retains the
  caller's active scoped ref, while ending the callee formal/local storage.
  The actual caller closes its loan afterwards. RETURN target identity must
  be that callee function's target, never the enclosing caller's target.
- C admission consumes both receipts before emitting. The formal C pointer
  comes from the authenticated original caller field; the emitted function
  is a genuine `void` named helper with actual parameters, both C IF arms
  and a real `return;`. Cached truth does not select or inline either arm.

The compiler/issuer and its private immutable seal storage remain trusted.
Private-producer mutation tests are bounded corruption/validator regressions,
not a universal proof against arbitrary C memory corruption. Public views do
not accept foreign contexts as replacement authority.

## Reproduction

```
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure -R 'p(701|706|713|718|722)_'
python3 tests/p722/native.py build/newlangc build/p722_public_probe gcc none tests/p722
python3 tests/p722/native.py build/newlangc build/p722_public_probe clang none tests/p722
```

`public_probe.c` loads actual registered source and destroys its original
syntax inputs, checks the public call/IF receipts, root/child identities,
Unknown postcall vs preserved sibling and active->closed caller loan, and
prints world-qualified operations/read symbols for the separate observer.

`native.py` runs ordinary CHECK, ordinary C emission, public validation,
strict unchanged generated C17 and a separate passive observed translation
unit, at O0/O2. Observer comparisons/counters supply no guest data or rights.
They compare the formal address with the live caller original field, check
the real old Copy capture/store, both branches, return-only-from-callee,
caller continuation/readback and disjoint archive write. Observer pointers
are cleared before original caller locals end. Synthetic removed RETURN,
wrong store and skipped caller continuation are separate negative C inputs:
strict compile refusals are **native NOT RUN**, not native rejection.

`gate_test.c` mutates copied public views and test-owned private evidence:
wrong owner/root/field/incarnation/fact, arguments/formals, source/world,
stream exchange/omission/duplication, ref permission/scope, Copy origin,
RETURN target/edge and sibling. The after-write case explicitly has identical
numeric node/old/incoming IDs in distinct arm worlds, with different incoming
bytes; exchanging entire streams must be rejected. Fail-at-N tests cover the
public validator, admission, registration, actual call and complete driver;
public snapshots remain unchanged and failed owned artifacts are NULL.
Pre-admission failures produce zero C. Named-output failure preserves prior
bytes. Emission allocates nothing after successful admission and /dev/full
reports failure; no stronger stdout atomicity/power-loss guarantee is claimed.

## Initial failures retained

`evidence/development` keeps the first full local run and observer failures.
These were test-harness corrections, with frozen NewLang sources/oracles
unchanged: `-o` corrected to existing `--emit-c17-to`; synthetic skipped
continuation may be refused by strict GCC dangling-pointer diagnostics; and
arm-local numeric node IDs must also be arm-qualified in the observer.
The first full local suite had one P722 observer failure. Sanitizer/CI failures
must be reported separately if any; none may be reclassified as native PASS.

`negative-freeze.json` and `read-formal-freeze.json` retain initial predictions;
`negative-expectations.json` records corrected existing diagnostic contracts.
`loan_read(record@field)` is an unsupported source loan profile, not proof of
semantic read-only rejection. The separate `ref<read,u8>` formal actually
rejects `replace` with P3-TYPE-MISMATCH. Other semantic negatives cover return
type, store type, scope escape and use after loan scope. No source was changed
to hide a compiler failure.

## History, alignment, limits

Historical design audit: KEEP / N/A faithful implementation of Draft17.33
§§13.5a/c,16-17,18.7-18.8,27.2-27.4,27.9. Consulted Compass, Product Strategy
§§1.1/1.3, Process §§4.2/4.3.1/4.3.2, Decision Procedure and Ledger
DI-001/006/007/017. Existing M1-M4 adopted implementation is reused; old DRAFT
PRs are not treated as current authority. No syntax, normative semantics,
exclusive borrow, heap custody, general CFG/SSA or new FFI was introduced.

Finite scope: one known direct helper with these two parameters, unit result,
a pure literal/usize-binding flat `<` condition, one normal arm and one unit
RETURN, direct Changes of the same original field. No nested IF/call/loop,
arbitrary typed early return, heap/nonCopy or product completeness is claimed.
Further source shapes require their own complete evidence and lowering gate.

Goal Alignment: this advances the selected v0 critical path by composing
caller-owned mutation and function-local control exit. It does not establish
ordinary-safe library encapsulation, smaller author+caller UB-prevention audit
closure versus factored C/Rust, or Product GO. Active scoring PAUSED624 and
H1 HOLD268 remain. Evidence retention duplicates semantic histories and has
bounded but nonzero memory/validation cost; it is not a zero-cost claim.
