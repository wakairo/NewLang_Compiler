# P1062 / C150: bounded H result normalization

DRAFT HOLD only. KEEP Draft17.34 / REPLACE NONE / faithful Process §4.2 N/A.
Fixed parent PR1059 is `4c039095455e0df32eebf538b0f9db1db1e089cc`, tree
`6c8dd17e7921becc3f02d66aefaf1c9c6130d87d`; main remains
`438170098d5b98096f1ad1f1f200dc1f326c75c3`.

Compass → actual Value Strategy §§1.1/1.3, Process §§4.1.2/4.2/4.3/7,
Backend Execution Policy and CURRENT_SPEC → Draft17.34 were read before work.
Canonical §§18.1a–b and 27.1/27.9 supply the existing read-loan, producer
RETURN, binding and final unit RETURN laws; no language design was changed.

## Independent freeze and first boundary

Source-only commit `278c7c7bc89d2cd44e37d39f3341b9661a590ea0`, tree
`a3c1de1eba74a07514c2f99d97ee17966a99a8a4`, precedes every source execution
and Production edit. Issue1062 comment6520462938 records the pre-run freeze.
The 19 complete sources plus oracle/observer/baseline are 22 immutable inputs.
Their manifest SHA256 is
`a9e5403f67550c178c73155a742f3a51eb583b15837454d052329762342b5534`;
oracle SHA256 is
`9847539057aee9139d55e43d15ee29b3903f35d03c0d9a578d7f826fe12e05c6`.

No R1016/R1051/R1057/R1060 original source, names/values, oracle, runner or raw,
and no B1061 original report, was accessed or reused. F90's fixed-parent
conditional static A was read only after the independent freeze. It is not a
formal proof of this new candidate. No new F/R execution or R retest was asked.

After the implementation and all local gates were complete, the publication
authority check read C151's Coordination summary of F90 (including its high-level
mention of B's local-return guard hypothesis). This was after the independent
source freeze, diagnostic and repair history. No fixture, oracle or implementation
change derives from that summary; B1061 itself remains unread.

All six positives have genuine CHECK0, actual Copy GET and body/parameters/event
current0, H origin validation, producer entry/valid/return evidence, and terminal
entry/post evidence. There is no claim that the API exposes a general H UNIT
current validator: that differs from the applicable public Copy validators.

| Frozen P source | Parent C17 | First measured private obstacle | Candidate C17 |
| --- | ---: | --- | ---: |
| `regression_closed` | 0 | Original P1058 regression | 0 |
| `regression_reversed` | 0 | Original P1058 reversed declaration/field/arm order | 0 |
| `terminal_without_probe` | 0 | Optional terminal read is absent | 0 |
| `terminal_explicit_return` | 4 | `block`: terminating owner body rejected | 0 |
| `producer_direct_result` | 4 | `no_read_entry`: local-return-only issuer view | 0 |
| `failure_scope_read` | 4 | Read in the other allocation world changes shared reloan count | 0 |

The ordinary CLI reports a flattened backend4; it does not identify a private
call stack. The source-locked diagnostic separately records the first guards:
parent instrumented `block:544` and `allocated_expr:1901` (last two sources),
with authenticated operands/counters. Its pre-run source SHA and exact raw are
retained. Diagnostic code is evidence data, never Production.

The four new variants use independent P names/values and different lawful H
body/cleanup/helper forms. New H stamps are 13/187; T originals initially
58/141 and 220/94, helper old results 58/94, next 167/58, direct old 220 → 94,
final originals 167/141 and 94/58. Complete original and regression oracles are
in `oracle-v1.json`; the inherited historical `parent_C17:4` fields are explicitly
disambiguated by the original pre-run Issue comment and current prediction fields.

## Implementation and trust boundary

Production changes only `src/checked_c_node.c`: +91/-28, net63 lines.
Two borrowed `Env` node pointers add no heap object or ownership receipt.
The pre-existing authenticated closed-main plan selects three normalizations:

1. Producer direct and local results use existing `producer_valid`; actual
   attach/Some, operand, head/root/incarnation/Domain, package and return checks
   still run. Every legacy consumer retains its original local-return validators.
2. Only the actual owning function's final literal `return unit` is normalized.
   A terminating checked block has type0 and `returned.type=unit`. Every preceding
   item is emitted and all original owner/lifecycle postchecks must pass.
   Terminating allocation arms, early cleanup exits and arbitrary RETURN/CFG are
   not introduced.
3. An actually traversed, nonescaping unit loan with an unused read reference is
   observational, not an H topology change. It retains all provenance, actual
   pointer/stability operand, type, full range, root/incarnation, Domain and scope
   checks, plus original live H stage4. It emits the real reference and scope end.
   It cannot advance or fabricate ownership/world completion.

Classification uses checked behavior and the existing consumer boundary, never
source spelling, helper/field names, constants or arm numbers. The all-arm cleanup
guards, public current validation, issuer/H-origin and canonical destruction ledger
are unchanged. Common main still needs both genuine normal H outcomes closed
before its independently CHECK-issued T originals and loans are consumed.

## Preserved repair epochs

All original sources, oracles, first RED and v1–v11 remain unchanged. Each subsequent
harness/source SHA was frozen before executing it; see `harness-freeze-v1..v7.json`
and `repair-*.json`.

- First candidate: final unit body additionally had checked type0; restore that
  existing checked convention without admitting other terminating blocks.
- Passive observer: optional local-packet callback was unused for direct RETURN;
  use portable `static inline`, with the same assertions and oracle.
- Legacy byte-identity RED: observational classification affected the old H read
  protocol; limit normalization to the existing closed-main consumer.
- New history negative had a stale basename; target actual immutable P1058
  `closed_then_two-C17.stderr`, preserving the same rejection oracle.
- Full Clang first failed old P738's finite direct-no-local boundary. Its lawful
  CHECK0 source is an old coverage refusal, not a wrong-owner or UB counterexample.
  Retain local-return guards in every legacy lane and restrict both result
  normalizations to the existing closed-main plan. Original P738 test then passes.
- Formatter whitespace only; no frozen source changed.
- ENOSPC stopped three builds. Only new generated ASan/UBSan build directories
  were moved to `/tmp`; all 2,642 files per directory were SHA256 checked equal.
  Original worktrees, evidence and builds were untouched; resumed builds pass.
- ASan old P633 hit the unchanged 90s timeout during simultaneous validation on
  a two-CPU quota. Its first log remains. After other lanes finished, the exact
  same test/flags/oracle/timeout passed in 11.10s; only the 82 never-run tests were
  then executed. The 39 earlier PASS results were retained, never fabricated.

## Actual local results

Tested final implementation commit `c8dc2a54c78627e02429a0372cf542c59ba3caed`, tree
`2d621704d704dd15bea24aa3fb1adf84beb02bea`; Production file SHA256
`a2ecb82ede7d20fa870da773d37d293cd7b53009123f87898779a73a45153546`.
Harness-v7 SHA256 is
`9c7b96ef1d9caba24c06bb02fdb0a081ca4db4957dd8b39ea37314cef4ce5279`.
Later publication changes add evidence and refresh prospective v12 only.

- Full frontend inventory: GCC 591/591; Clang 591 unique PASS (one targeted P738
  plus 590 remaining). P633/P817/P785/P790 and old H/T/one-normal+RETURN regressions
  are retained. Full local LLVM infrastructure smoke was not executed.
- ASan/LSan and UBSan: each all six new profiles plus 122 relevant old tests.
  ASan old set is disjoint 39+1+82; UBSan 122/122. Original P965 ownership494 and
  reached CHECK-ledger OOM502 are verified in each of all four compiler profiles.
- Strict GCC13 and curated Clang23.1.3 C17 O0/O2, guest0/1/2: 36 passive physical
  traces per compiler/sanitizer profile, 144 total. They observe original H
  address/root/incarnation/Domain and complete teardown, read scopes, actual
  malloc/free/trial counts, independent live typed T originals/fields/siblings,
  two actual helper returns and the actual direct-replace return. Raw unit exit0
  alone remains INCONCLUSIVE.
- All 13 frozen source negatives: seven genuine CHECK0/current0 → backend4;
  six semantic CHECK3. Fresh/existing/stdout no-clobber, same inode/no temporary
  leak; reached output failure then exact retry. Defined-u8 wrong-second-return
  calibration must specifically fail RET1; arbitrary signal is not a PASS.
- Per profile, new host negatives: actual current84, supplied views36, H owner24,
  world42, plus12 foreign-body/arm B-survival cases after unrestored A destruction.
  Outer metadata mutations are rejected first by the original H-origin seal;
  they are not claimed as direct coverage of the new completion predicate.
- New emitter injection: 2,750 actually reached positions per profile
  (459/459/455/463/455/459 by source). Outcomes are 2,634 OOM and116 conservative
  UNSUPPORTED, always unpublished/length unchanged, same-context snapshot and
  issuer/current/owner intact, exact C retry. Guest heap failure is a separate
  three-world measurement.
- 44 old generated C byte identities pass; strict pinned formatter passes.
- Original v11 SHA256
  `e78dc38e5dabc83fd7b619c43479ed293871fc67d830aeb98ddac26cac5b6bd4`,
  blob `e3b51f0cdd971418477aed4a5744b8a16789f073`, all2307 keys inherited.
  Five history attacks (coedit predecessor+successor, drop, old source, old
  oracle, old first RED) reject. Original22 source inputs and full first-harness
  archive hashes pass. No historical checker/oracle was disabled.

`evidence/local-results-v7.json` holds measured rows. Exact raw ZIP SHA256
`ad1eee30e4597788e3d96a0df25942d87f7891c8957bf5fb70d35ab752f741e1`
is 5,063,095 bytes with4,991 raw entries and an internal hash manifest.
It retains original generated C and all completed/interrupted first logs; the
missing first candidate's per-command archive was not invented.

## Limits, cost and return

This is a finite P-only capability result. Original blind R POS-B, arbitrary H
forms, third helper, escaping observations and general CFG are untested. No
source→accepted-rich→C formal soundness or adoption claim follows from these tests.
Legacy R675 regressions remain; two T synthetic tests and zero newly approved R
cohort tests register. No R/F launch, READY, merge or Product GO.

VALUE-ALIGNMENT-RISK: B keeps C-class pointer topology/owner/loan/cleanup and adds
63 private C lines/two borrowed pointers, with real observer/history/evidence and
retest maintenance cost. A remains truthful NO CHANGE plus a support matrix if
future complexity exceeds useful generality. Human UB prevention audit/re-audit
dependency closure for library authors AND ordinary-safe callers versus factored
C, comparative product cost, and performance remain UNMEASURED. C/D (new carrier,
issuer/API/ABI/IR or general interpreter) remain deferred.

At most one new DRAFT/HOLD stacked PR and one natural FULL5+PARTIAL group (six
jobs) are allowed after local safety. CI results are reported to Issue1062 and
the PR metadata; no post-CI push, dispatch, rerun or independent R retest request.
