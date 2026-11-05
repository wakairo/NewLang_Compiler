# C243: phase-selected current on one original T

Implementation parent is fixed PR1171 `870e4f0972abb9725b7cf9f38d09178e51ff1280`,
adopted as main `217dda0c0c786f64ef4376aec1cb7e72b2a4ecc2` with the same tree.
Authority is CURRENT_SPEC to Draft17.34
13.5a/17.4/27.4-27.7. The implementation is private faithful composition;
public source/API/ABI/issuers and canonical custody/destruction are unchanged.

The finite family is direct unit main, one ordinary two-u8 original, one fixed
child, literal initial usize, `phase < L`, representable positive constant ADD,
one Continue and real-phase Break. A single inner IF compares the actual phase
to a usize literal with `<` or `==`; both normal arms return literal u8 values.
Its result and ordinary Copy aliases feed exactly one scoped replace. Local
old/current/sibling uses stay source ordered. No writer, call effect, control
exit or escaping loan is allowed in the inner IF. General pure expressions,
SUB, extra/nested loops, multiple writers, heap/non-Copy and dependency-bearing
headers remain outside this envelope and fail closed.

The existing arbitrary-u8/usize H remains. The private complete walk checks the
actual normal scalar join, both three-node arm graphs, common prefix/package/
scope/phase/current frame and the real IF-result -> binding/Copy -> incoming
association. It does not replace literal/known checks with arbitrary Unknown
acceptance. The new producer variant seals five graphs instead of three.
All old current/fresh child/root facts, canonical sibling owner mapping,
leaf/union exits, actual Break/caller projection and registry checks remain.
The C17 consumer uses the actual per-iteration phase, then the same original
store. Its scalar result adds no store on zero-edge or Break paths.

Six independently frozen positive profiles cover zero/one/multiple edges,
both fields, equality and less-than predicates, incoming Copy aliases and
same-bits rewrites. Twelve logical negatives and two exact-byte controls are
retained. The original four C242 positive/four negative inputs, oracle,
registration ADD-precision FIRST and all v1/v2 equipment/raw/bundle are sealed
in `evidence/p1174-checkpoint.zip`; they are not reclassified as successes.

`source-freeze-v1.json` keeps all original new-source/oracle bytes. The first
escaping-arm fixture used an unsupported `loan_read` field spelling and was
rejected at LOCAL-LOAN-PROFILE. That original stays unchanged. `oracle-v2.json`
selects a separately frozen version using the existing bounded `loan_write`
spelling; it must reach P8-EXIT-DEPENDENCY. This reaches the intended ending-
scope check without changing Production or discarding the original refusal.
Unrelated literal/Copy incoming cases reach the new relation's P14 refusal.
Other negatives may hit an earlier precision/profile gate; their rejection
is not claimed as independent evidence of a masked deeper condition.

`safety.c` uses real registration and main() CHECK, public owned observation,
then private current validation. Each positive tests 40 current/source/phase/
arm/world/result modifications plus 14 unrestored display/foreign challenges,
and A destruction with independent B survival. `run.py` checks ordinary/named
CLI, frozen current/native arrays, exact control bytes, strict C17 O0/O2 and
passive original-address/phase/incoming/alias/old/current/sibling/Break reads.
GCC13+Clang23 provide 48 plain/observed native runs; one sanitizer compiler
provides 24. Native bits are not a substitute for ghost-current checks.

`census.c` passively counts actual clone lifetimes and owned source graph nodes.
The first observer missed same-TU destruction inside nl_sem_commit and retained
two already-consumed candidates in its bookkeeping. Its cleanup RED and v1
bytes remain; its context totals are excluded. The v2 commit wrapper records
that real consumption, still calls the exact original implementation and still
requires live=0/untracked=0. Equipment v1/v2 are archived and v3 is current.

Measured on the local candidate: registered context count 1, owned 30
(additional 29), peak 31, versus literal-control owned25/additional24/peak26;
cleanup0. All six new profiles have this same retained count regardless of
runtime iteration count. Owned body source graphs are 6 versus 4; nodes are
71, or 77 with incoming aliases, versus control59. State/package/source size
still affects cost; 29 is not a universal bound for arbitrary programs.

New reached-allocation census: phase-left registration774 + CHECK930 + named
CLI1824 = 3528; incoming-alias 800 + 952 + 1878 = 3630. All 7158 ordinals at
49 distinct direct GCC allocation PCs pass OOM/rollback/no-clobber/cleanup/
retry. Shared allocation PCs represent distinct newly reached operations.
ASan/LSan and UBSan each pass the new current/census/failure matrix and native
cohort; the old 6091 result is not reused as new evidence. The corrected ending-
scope negative separately reaches its specific rejection on all three builds.

Prospective v40 externally pins the complete fixed v39 and inherits protected
history. P1107 gets an append-only v25 current-CMake snapshot, preserving its
original source/oracle/FIRST and all v1-v24. Final GCC/clean-checkout/CI receipts
are reported in Issue #1176; local sanitizer work is frontend-only.

VALUE-ALIGNMENT-RISK: ordinary IF/Copy/current composition is reusable v0
progress, but nested seals, transfer walk, phase-aware emission and +5 retained
contexts expand maintenance and re-audit dependencies. Audit locality remains
UNMEASURED; GO624 PAUSED, H1 HOLD and P1036/PR1050 STOP remain. No new independent
R input/oracle/runner/report was consulted and no Track was launched. General
loop soundness, arbitrary dependency-bearing H and Product PASS are not claimed.
Publication requires the Issue's actual PR1171-main adoption/tree/docs binding;
the result stays DRAFT/HOLD, without P READY/merge/Product GO.
