# P932 C54 — STOP at first local source-adapter assertion

This is an **incomplete, failing P-owned diagnostic experiment**, not a merge
candidate. Fixed base `3d6e45bc8d89c0524ba0538113433f66f474dee1`, Draft17.34;
F72 mathematical B only. No Production file, public API, canonical, workflow,
toolchain, old source, oracle or historical evidence changed. No Actions, PR,
repair, full regression or sanitizer run after the first local failure.

## Read-only Gate A

[Gate A report](https://github.com/wakairo/NewLang_Compiler/issues/932#issuecomment-6408025933)
preceded this branch and implementation. The archived debugger probe used
the unchanged ordinary compiler and its original unit AST, real body creation,
checked BLOCK creation, actual pre-IF prepare and owner destruction. Four
original immutable sources yielded eight qualified observations (definition
and actual main). Source labels, initializer permutation, typed literals and
condition came from original AST/raw token spans, not P930 Expected or entry
scalars. Actual C member-owner ValueId was related through the root package
to its owner PlaceId. The archived Python/GDB files are the original read-only
Gate A diagnostic, not a new Gate B hook or Production engine. Pinned binary
and archive hashes are in `evidence/gate-a-environment.json`.

The Gate A prototype inspected initializer fields' `data.binding` payload but
did not assert their syntax kind. Its finite successful observations therefore
did **not** validate the later C adapter's assumption about that kind. The
positive Gate A established data availability, not correctness of this new C
adapter or a general A/C refinement theorem.

## Gate B frozen oracle and first failure

`freeze.json` SHA256
`c851008e0694d839bd01c024e6e169486d980d7affd95fa61a99fa32e9f05aab`
was written before the adapter build/test. All six P930 source bytes and its
Expected table, calibrated controls and old P929 erroneous EXIT2 oracle stay
unchanged. The adapter header is included before Expected and does not read it.
The implementation is 301 header lines plus eight added observer lines;
the original tree borrow ends before client cleanup. It allocates nothing.

First configure/build: GCC13.3 Debug, frontend-only, successful. First and only
focused invocation (exit8):

```sh
ctest --test-dir /tmp/p932-build -R '^p930_' -V
```

Three CTests stop with `P930 line89: f && f->kind == NL_SYNTAX_BINDING` in
`adapter.h`. The existing macro prints P930 even for the included P932 header.
The fourth ordinary-CLI test passes. No new source-to-entry correlation or new
TypeId/namespace/cross-phase refusal is reached; do not claim those properties
as tested. The OOM test reaches its original proof-calloc fault once, confirms
zero observations/inventory rollback, then fails at the adapter assertion on
same-context healthy retry. The second clone-malloc fault is **not reached**.

Read-only root cause: fixed `src/parser.c` creates aggregate initializer labels
as `NL_SYNTAX_FIELD` at line1346, then populates `data.binding.name/initializer`
at lines1362–1363. Semantic construction consumes that payload at
`src/semantic_check.c:2914`. This adapter wrongly demands `NL_SYNTAX_BINDING`
before reading it. It is a **P test implementation defect**, not a Production
semantic refusal, missing source datum, F72 proof failure, or unsafe accepted
authority. The old source and oracle need no change. Per C54 first-failure
STOP, this known local error is deliberately preserved, not repaired/retried.

Ordinary CLI remains four positive CHECK0 / C17 EXIT4
`V1-BACKEND-UNSUPPORTED`; malformed controls CHECK3
`P5-AGGREGATE-FIELD-COUNT` / `P13-IF-CONDITION`. Both output modes preserve the
sentinel and leave no temporary output. No native claim for these programs.

`manifest.json` authenticates the frozen oracle, failing adapter and original
first evidence. To reproduce after checkout, configure frontend-only using
the repo's existing compiler/tool prerequisites, build normally, and run the
focused CTest above with the new build directory. Expected result is failure;
this branch is archival and has no PR/CI invitation.

## First unmet contract / minimum handoff

A separately authorized continuation could correct only the FIELD kind mapping
after checking the actual parser/semantic payload contract, then first test the
source-to-registered-type/checked-binding/package normalization and its frozen
negatives. This task does not authorize that repair or another test/CI run.
No result beyond the first assertion should be inferred from unexecuted code.

Even after such a repair, complete source-event history/activeBlocks, globally
injective F72 encoding, general parser/registration A and C refinement and
source-issued D rich governing lifetime/carrier remain UNPROVED/HARD HOLD.
Qualified host labels and implicit local/domain0 create no guest authority.

VALUE-ALIGNMENT-RISK: this experiment currently adds a failing test adapter,
not ordinary user capability or measured author+safe-caller UB audit locality.
Avoid a repair campaign or new trace/authority system. KEEP Draft17.34;
REPLACE NONE; Product GO PAUSED/H1 HOLD and independent R evidence unchanged.
