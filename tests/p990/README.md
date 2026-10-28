# P990 / C103: direct old-u8 Copy returns in mixed C17

A real mixed H/T call could CHECK and validate its one Change, yet the C17
consumer required two checked items: a REPLACE binding and an identifier RETURN.
The private consumer now also accepts one RETURN(REPLACE) after the same actual
GET/current body/parameter validation. The event must select that exact
callee initializer, the two actual formals must match, and the returned value
must be its original old-u8 result. Existing binding-return checks remain.

## Authority and unchanged contracts

Fixed parent PR984: c981ff742865731decb434c2382a6fb418c83de5, tree
aa01141aa821f4433725aa3b533cccd268cb064f. Main authority:
438170098d5b98096f1ad1f1f200dc1f326c75c3, CURRENT_SPEC -> Draft17.34.
KEEP Draft17.34 / INTENTIONALLY REPLACE NONE. This is a faithful private
consumer change, not a public syntax/issuer/API/ABI or CFG/MIR/LLVM selection.

Production changes only copy_call_proof in src/checked_c_copy_internal.h.
H origin/canonical custody, existing postterminal/retired-backing IF checks,
current UNIT/V0 guards and toolchain/workflows are unchanged. Borrowed evidence
is used synchronously while its real CHECK owner is live. No syntax creates a
new capability, counter is reset, or foreign same-number world is accepted.

## Independent sources and oracles

Issue990 comments6464301679/6464309116 published the complete new positive/
negative sources and their oracle/hashes before execution. Source-only commit
f8127dcb90d4d3c374c802ad8a17cd51367e3f88 retains that first freeze.

The new pre-H family calls the second Dial990 root's field0 and transfers old121
to the first root's field1. Initial values83/17 and121/201 become83/121 and37/201;
H payloads97/173 retain original head/tail identity and cleanup. The new post-H
family constructs both Meter990 roots after both H releases, calls first-root
field1, and transfers old151 into second-root field0. Initial61/151 and233/29
become61/89(or43) and151/29; H payloads47/193. Direct/bound and real IF TRUE/FALSE
pairs retain actual old/new values and siblings.

The original six P986 sources and old oracles/manifests are byte-identical in
p986/. Their parent first failures, false prediction and old grades are not
rewritten. No R979/R985 source, oracle, fixture or prediction was imported.

## First failures and forward-only calibration

- Candidate5a31f2a's first versioned-freeze attempt rejected a missing protected
  dependency, tests/integration/two_heap_platform.c. Compiler/native had not
  run. The failed current-freeze-v1 remains intact; v2 adds the dependency and
  predecessor digest rather than overwriting v1.
- Candidate4ffb7b3 passed CHECK/current/auth for12 sources, then the new source
  oracle expected a duplicate RETURN refusal but received C17 success. Readonly
  tracing showed CHECK had removed the unreachable source tail: five issued
  nodes, one RETURN, one Change, original old121. The fixed parent's old bound
  spelling already accepts the same dead source tail. The first failed oracle
  and logs remain RED. Separate frozen V2 normalization controls check the
  original direct and old bound form; a new conditional early-return plus final
  return source remains CHECK0/C174 with named-output preservation.
  Production was unchanged during this test calibration; no AST gate was added
  solely to force the mistaken negative.
- Source/oracle-v1, negative-layered-v1, all three current-freeze generations,
  and original P982 manifests/source/oracle/first REDs remain unchanged.

Candidate code/harness tested: 5416759f421c710df15c26ae6dacd7e0c232f562,
tree a3ea5196f6177d9c1f724d7f7622e7f131ffaaf0. Later evidence publication changes
no tested production/harness/source/current-manifest bytes.

## Prospective freeze integrity

The original P982 manifest bytes and file digests are authenticated against the
fixed parent's retained raw Git commit, eight trees and25 original blob paths.
Original CMake, three production files and checker bytes are retained separately.
Unchanged P982 source/oracle/probe/runner files must still match their original
Git blobs now. A future current digest cannot launder a protected-source change.

The retained p982_freeze.unit name runs both historical validation and the
current versioned freeze. Current v3 pins506 current input files and links v2
and v1 by exact SHA256. It does not reinterpret old production digests as today's
code or silently replace any old manifest. CI needs no fetch of a shallow
checkout's missing parent objects. Deliberate protected-source mutation, bad
changed-file digest, altered archive, missing generation update, bad future
digest, bad predecessor and attempted source laundering all fail. A scratch
future generation with a legitimate production-byte change passes.

The separate independent R registration/workflow hook problem remains outside
this protocol. No workflow was modified.

## Validation evidence

Final measured results are in evidence/local-results.json. All original545
frontend CTest names remain, plus30 P990 tests. Fixed full formatter291 C/H and
historical SHA gates pass. All four configurations pass575/575 frontend CTests. They cover GCC14.2, fixed
Clang23.1.3, ASan+LSan and UBSan; actual compile instrumentation is recorded.

Per configuration:
- Original owner494 and CHECK/definition reached OOM502 (320/182) are reacquired
  and matched in full, not inferred from totals. Original P982 emitter1788
  records also match its preserved complete inventory.
- P99014 source contexts check actual GET/current/event/result,364 restored
  public metadata negatives and56 foreign/corrupt candidate views. Twelve mixed
  contexts test node-emitter refusal; two T-only contexts test validators.
  Canonical A teardown leaves genuine B usable.
- New emitter3388 positions are independently reached: four post profiles447
  each, six pre profiles203 each, two head-read profiles191 each. Actual statuses
  remain2980 OUT_OF_MEMORY and408 conservative UNSUPPORTED through inherited
  boolean validators. Every failure is unpublished, preserves the summary
  snapshot, revalidates original current proof and permits exact C17 retry.
- Ordinary CLI output is compiled unchanged with strict C17
  -Wall -Wextra -Wpedantic -Werror, O0/O2. P990 raw28 and physical guest72
  observations check actual old Copy, both independent T originals/four child
  addresses, new/current/siblings, H originals/LiveTail/terminal/0-1-2 exact
  cleanup. P982 raw8/physical24 are reacquired separately. Passive hooks only
  read live objects; finish uses saved scalar observations.
- Ten active source negatives preserve exact layer and named output bytes/
  dev/ino. Fourteen accepted sources reach named write/flush failure, retain
  previous output without partial publication and succeed on exact retry.

Full raw-log SHA manifests, both first REDs, immutable source manifests, complete
new reached records and unmodified C17 text are preserved. Summary snapshot
rollback is not a claim that every semantic-world byte was compared.

## Limits and value risk

This is finite producer evidence, not arbitrary malformed pointer/length,
opaque-owner, concurrency/reentrancy/FFI, all H/T orders/callee/CFG, universal OOM
or independent fixed-code F/R soundness. R first-pass verdicts are untouched.
The new child stays DRAFT HOLD, no READY/merge or new F/R launch. CI authority
is at most two natural FULL5+NO-LLVM PARTIAL pairs/12 jobs, no manual same-SHA
rerun; actual runs/attempts/checkout SHA/tree are reported in Issue990 without
an evidence-only extra push.

VALUE-ALIGNMENT-RISK: one finite private return-shape branch retains original
CHECK/Change authority, with zero public issuers/core laws or additional
library-author/ordinary-safe-caller duties. The shape adapter, freeze protocol
and calibration evidence increase compiler audit/maintenance dependencies.
Human UB audit/re-audit closure reduction versus well-factored C, performance
and maintenance costs remain UNMEASURED. Product GO PAUSED / H1 HOLD.
