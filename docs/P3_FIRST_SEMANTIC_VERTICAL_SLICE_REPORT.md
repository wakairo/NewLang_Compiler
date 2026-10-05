# P3 first semantic vertical slice — blocked authority audit

Date: 2026-10-05 (Asia/Tokyo). **P3 is not complete or ready for review.**
Implementation is paused at the explicit ending-argument ambiguity gate.

- Repository: https://github.com/wakairo/NewLang_Compiler
- Branch: `p3-first-semantic-vertical-slice`.
- Verified merged P2/base main: `cd229877d550e913260ad5a79627b65c1eb405d0`.
- P3 implementation PR/current-head CI: not created/requested; baseline evidence
  is not claimed as P3 validation. P2 PR #4 is closed/merged.
- Input: user P3 handoff, SHA-256
  `c55d2d35769144735098bc0d1e4210682fe84017a547494d8a761649323236a9`.
- Live formal evidence inspected at
  `fdda0d99d3de961f782f465fa2033b5554790ba5`; Lean not built or added as a
  production dependency.

## Completed independent work

Read the complete P3 handoff, verified repository/P2 merge and exact main,
inspected production interfaces/policies, relevant Draft/Backend Contract rules,
F0 bridge/live F0 operation and ref/domain evidence, and the frozen oracle's
ending-argument code. The pre-implementation
[contract/audit](P3_SEMANTIC_SLICE_CONTRACT.md) records the permitted subset,
required ownership/transaction/error boundaries and the distinguishing fixture.
The attachment's task directions and evidence documents have separate authority.

Bootstrap and formatting succeeded on merged P2 before changes. Fresh baseline
GCC Debug and Clang Debug each ran **20/20 tests, PASS** (including LLVM C API,
oracle smoke, CLI, diagnostics and artifact integrity). Host: Linux x86_64,
Debian 13; GCC 14.2.0, Clang/LLVM/clang-format 23.1.2, C17, CMake/CTest 3.31.6,
Python 3.12.14. Existing bootstrap/activation remains sufficient; no environment
configuration, toolchain pin or dependency changes were needed.

Only the two P3 audit/report documents were created. Source, P2 syntax/parser,
tests, CMake/CI, README, historical references and frozen oracle are unchanged.

## Blocking finding and pending decision

**COMPILER-SPEC-AMBIGUITY ENDING-ARG-01:** canonical take/destroy argument
mapping: ordinary non-Copy consume versus automatic operation-local exclusive
reborrow. Draft §4.2 governs ordinary use; §12 permits the reborrow mechanism and
sequential authority reuse; §14.2–3 supplies primitive pre/postconditions.
The M7.5 checker explicitly consumes its direct ending binding. F0 evidence
abstracts authority and omits this source availability/scope rule.

An authoritative M8.1 mapping was not available among the supplied documents.
The ambiguity is about applying the mechanism to this call spelling, not a
normative contradiction. The contract records the two-root/same-domain minimal
fixture, sections, oracle evidence, formal boundary and candidate resolutions.

The user handoff §117 requires reporting ambiguity without forcing P3 completion;
§226 forbids a silent choice. A clarification asks for the existing M8.1 rule and
its source/decision text. No answer, timeout or prototype behavior is treated as
authorization to select semantics.

No other blocking COMPILER-SPEC-HOLE, IMPLEMENTATION, IMPLEMENTATION-LIMIT,
DIAGNOSTIC, PERFORMANCE or LOWERING finding has been established. The P3
representation/resource choices and pointer exclusive-write authority have not
been finalized or tested; they are not reported as implemented facts.

## Validation/completion status

| Check | Status |
|---|---|
| Merged P2 baseline bootstrap/format | PASS |
| Merged P2 baseline GCC / Clang | 20/20 PASS each |
| P3 semantic types/context/checked representation | Not implemented |
| P3 operations/loan headers/rollback/OOM fixtures | Not implemented |
| P3 source -> parser -> checker integration | Not implemented |
| Final P3 GCC / Clang / ASan / UBSan / Release | Not run |
| Final P3 current-head PR CI | Not run; no P3 implementation PR |
| P3 completion | BLOCKED, not ready for review |

Next step: resolve ENDING-ARG-01 from the adjudicated M8.1 source or explicit
semantic decision, update the pre-implementation contract, then resume the
requested P3 implementation and full final validation/PR workflow. No PR merge,
P4, full-program/body checker, M8.3/M8.4 frontend expansion, dependency/effect
solver or LLVM lowering was started.
