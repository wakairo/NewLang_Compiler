# P541 — GO LIMITED CALIBRATED CANDIDATE

Track P, NON-BLIND CALIBRATED compiler quality correction. This does not replace
R540's original BLIND FINDING or its independently UNMEASURED owned/physical
subclaims. No independent Red Team or formal proof is claimed.

## Authority and read-only cost gate

Current normative Compiler/main is
ebb8c4d034a6df2ec6fbfae7e47ebb37650accae, CURRENT_SPEC selects Draft17.32.
Implementation starts exactly at frozen unmerged P534 compiler
7f639a3a89d55f44848e361e1458529fe5395345. Original PR539 and its entire
tests/p534 directory remain unchanged. The historical CURRENT_SPEC on this
candidate's ancestry still points to17.30 and is deliberately not edited;
the normative reference for P541 is the FIXED CURRENT MAIN, not an assertion
that this old branch's pointer changed. No normative documents are imported.

Read Compass -> actual Product Strategy1.1/1.2, Development Process4.1.1/4.2/4.3,
Design Decision Procedure/Ledger DI016, adopted17.32.1–4, R540 original terminal
and C disposition. Historical audit N/A — faithful implementation correction of
already selected17.32.2 per-constructor recipient, KEEP DI001/008/016 and
DEFER DI002/005/006/007; no new source eligibility/meaning selection.

Initial read-only inventory started2026-10-17 00:22:12JST (Oct16 15:22:12UTC).
Cost decision SMALL CALIBRATED CORRECTION at00:47:34JST, after25m22s and BEFORE
any Compiler edit. Traced constructor global-value guard, actual root placement,
singleton captured world/record/destruction, clone/function-plan ownership,
resource counters, public closed-frame validator, original C carrier, retained
source/AST, negative/OOM/native tests and unchanged FULL5/PARTIAL workflow.
Both main and PR539 remote HEADs were rechecked unchanged before branch creation.
Hard150min stop is02:52:12JST. See PREFLIGHT.md for predicted costs and stops.

## Frozen sources and unchanged compiler

Pre-execution freeze commit4cffbff7e8682153cac9e2b97e472953d120233f stores all six
UTF8 sources/oracles in frozen.json. P's self-authored two_roots.nl SHA256:
829f8d7b9a0aea422f4349d3fd0dc2b389d995b0ab13119b3f490cefc4cb30d1.
This is a P-authored CALIBRATED input, not independent blind R evidence.
The byte-identical R source copy r540_two_roots.nl has SHA256
3d01ff12734a194ccc8357625e16de5d1740019a59873ba467467f2152760f9f,
read from b580dabf14fe3b79255007cfb64546f046e07a07:r540/source/positive_two_roots.nl.
No R artifacts or oracle were edited. Hashes of all sources are checked before
each native harness execution. No source/expected outcome changed after freeze.

PRE-FIX7f639...: both two-constructor positives exitCLI3 during registration,
precision ARRAY4-ONE-ROOT-PROFILE. R copy span8:18–13:6, P source8:20–13:6.
Parser succeeds, semantic checker reaches the SECOND constructor; public
rollback PASS/artifactNULL, no owned/C17/native. Second arity3/5 already reports
semantic arity errors; second wrong-type/nonleaf are initially masked by the
same precision guard. baseline.json preserves all CLI/source/rollback results.

## Limited implementation and proof contract

Five Compiler files only: src/array4.c,src/semantic_internal.h,src/checked.c,
src/semantic_check.c,include/newlang/array4.h. Net266 C/header lines
(364 added/98 removed), plus14 net CMake test-target lines.
No parser/type/fixed/sum kernel, workflow, lock, dependency, canonical or source
flag changes. No generic, Array source access, Copy statement implementation,
Ring, nonCopy Frame/owner/refund, heap authority or new ABI/effect mechanism.

- Actual direct constructor receiving still uses the existing whole-value
  binding/fixed-child kernel: fresh original local root and four dependent
  typed Option children. The old per-function type-scan guard is removed only
  after replacing its issuer/validator/C17 assumptions. No IDs are synthesized.
- Retain a source-order owned EVENT COLLECTION under existing checked-node/
  semantic resource budgets. First inline world/record fields are kept solely
  for the unchanged P534 fault-test hooks. Each additional actual constructor
  owns its immutable issued record and actual point-in-time world clone.
  Clone/describe/allocate complete privately before appending. Failure never
  publishes a partial event; all world/record lifetimes are released on rollback.
- Public count/view_at/world_at/valid_at select CONSTRUCTION EVIDENCE RECORDS,
  not Array elements or a new source indexed projection. Legacy singleton
  accessors/validator fail closed on multi-construction fragments.
- Independent validation authenticates the complete direct-binding/normal-unit
  checked graph against the registered retained immutable body AST and actual
  source tokens, preserving binding/receiver/initializer source order. A selected
  checkpoint must contain exactly its complete issued prefix of original roots
  and dependent places/packages, with unique coverage and current typed None.
  Every earlier original is revalidated in later checkpoints before invoking
  the invariant kernel. Wrong/detached/duplicate roots or cross-root child/value
  swaps cannot be hidden by counts or equal Copy values.
- The input unit source/AST are destroyed by the public test loader before
  checking main(). Legitimate immutable body AST/source remain owned by the
  retained registered plan. This is not a source-file reread or a hidden owner
  registry. All IDs and world pointers are diagnostic observations, not newly
  issued lifetime/ownership/access capabilities.
- The ordinary C17 consumer authenticates the unit entry and validates EVERY
  record before ONE staging allocation. It emits a distinct actual typed C
  Array local for each validated source binding, in source order. The original
  single-Array C output remains byte-identical (SHA256068581b7c75a356be56fbb13f5d621dc2c01327fdbff9147fcda75a143e43217).

Default ordinary newlangc remains source-OFF. newlangc_p534 reuses the existing
test-only opt-in parser, actual production checker/owned/C17 and main driver.
New CMake wiring only adds P541 public/fault targets and four CTests; OOM and
semantic rejection reuse the existing genuine-source gate, not a seed fixture.

## Measured source/owned and static refusals

Both frozen positives: parser0, CHECK0, actual source-issued public owned PASS,
C17 CLI0. candidate.json stores exact diagnostics, output and local binary/
Compiler input hashes. Every original P534 source and test remains intact.

Actual public observations (not guessed IDs):

| event | source binding / constructor | original root/inc/fact/package | dependent places/inc/facts | element packages/source nodes |
| --- | --- | --- | --- | --- |
| 0 | symbol1 / node7 / ctor2 | 1/1/1/5 | 2–5 | values1–4 / nodes3–6 |
| 1 | symbol2 / node14 / ctor9 | 6/6/6/10 | 7–10 | values6–9 / nodes10–13 |

Structural Array type11, canonical Option element type10. Two distinct owned
worlds hold5/10 live places and1/2 independent roots. First original facts,
incarnations and parent/member relations remain current in the second world.
Each qualified projection is (original parent incarnation,index,child
incarnation). Both roots finally end normally; bindings are consumed.
These retained historical checkpoints are NOT currently live safe refs.

No Some occurrence/payload or new explicit Allocation/Domain/BackingRegion/
Storage grant exists. Explicit region/domain IDs0 describe the existing
compiler-managed implicit local place/incarnation relation, not authority
shared by two roots. The live physical objects are separately observed below.

Parseable second-constructor negatives all reach registration/semantic error
CLI3, public snapshot rollback PASS/artifactNULL:

| source | final diagnostic/span |
| --- | --- |
| second_arity3 | ARRAY4-INITIALIZER-ARITY 8:20–12:6 |
| second_arity5 | ARRAY4-INITIALIZER-ARITY 8:20–14:6 |
| second_wrong_type | ARRAY4-ELEMENT-TYPE 9:9–9:14 |
| second_nonleaf typed None block | ARRAY4-ELEMENT-CONSTRUCTOR 9:9–9:29 |

P534 payloadless Some retains P6-CONSTRUCTOR-SHAPE; Some(u8(1)) retains
semantic-stage unsupported ARRAY4-OPTION-VARIANT, not a universal type error.
No P3-INTERNAL is counted as a successful semantic rejection.

## Integrity and allocation failure

Actual source-issued public view copies:100 independently altered views reject
(cross-root root/symbol/constructor/receiver/package/child/fact/incarnation and
source origin, whole peer record at the wrong construction index).

42 cloned-checkpoint cases reject through the PUBLIC validator AND before C17
output. Forty isolated cases plus TWO coupled cross-root cases: a complete
parent/member rewiring and Copy-like lexical root conflation remain valid
under the raw internal invariant kernel, but have NO matching source-issued
operation and public validation refuses them. These are negative copies only,
never positive host-generated issuer evidence.

11 owned/source-graph controls reject: missing/duplicated/cyclic event,
wrong captured world, second receiver/initializer/source span/order,
hidden second source construction and wrong final incarnation. No consumer
can hide the second original and emit a false singleton carrier.
All corruptions are restored, genuine original validates again. Validator/
failed emitter do not mutate checked state; output NULL/length99 unchanged.

OOM source registration170 allocation sites and actual main104 sites:
rollback PASS/artifactNULL throughout both captures. Original P534114/65
remain unchanged. Collection validation allocates0. C17 staging failure1
leaves NULL/length77 and public/context/checkpoint fingerprints unchanged;
restoring allocation emits successfully.

## Native physical evidence versus ordinary execution

generated.c17.txt is ordinary compiler output682bytes, SHA256
516de3980e1b667dcc0e22a85de3780df2e3df9daf751e1b9dc5402ca85a750c.
It declares one full Option envelope type and a genuine typed [4] Array type,
then nl_local_1 and nl_local_2 in actual source function nl_fn_10. Every
element has an explicit compiler-generated symbolic None initializer.
No memset/zero-bit assumption, independent fixture globals or test values.

GCC14.2 and local Clang23.1.2, strictC17 -Wall/-Wextra/-Wpedantic/-Werror,
O0/O2: both actual frozen sources compile and ordinary generated code exits0
without guest I/O. Separately labelled observed.c17.txt passively observes
those SAME TWO originals after both declarations while both are live.
Two bases/disjoint ranges, eight distinct element starts, actual stride2
matching sizeof(Option envelope), each semantic None via opaque actual tag37.
No inactive payload is read; observer writes only its own address/I/O records.
Unobserved O2 may eliminate unused arrays: physical observations refer to
observed-C, not mandatory machine storage or a pinned-address guarantee.

For EACH source/host:4 successful plain/observed native runs atO0/O2;
3 bad-C variants ×2opts explicitly SIGABRT: aliased second carrier, wrong
second None tag and wrong extent. They are controlled C-output refusals,
separate from NewLang semantic negative tests. observed.c17.txt2166bytes,
SHA2566ea6a7c6a0bbd368246753d099ca28e5a0422e6f51de271468005a87b9992feb.
Host addresses vary by ASLR; no literal address is an oracle.

## Tests, CI budget and local development records

Local integrated frontend regression315/315 PASS80.41s. Final targeted
P534+P5418/8 PASS in GCC Debug1.27s and Clang ASan/LSan5.49s.
The regression run preceded two final fail-closed metadata checks; final
targeted tests cover those additions. Exact final-head FULL5/PARTIAL will
fresh-build/run all tests; actual outcomes belong to PR checks and the SINGLE
terminal Issue541 report, not a pre-recorded CI success claim.
Local LLVM/Clang23.1.2 is PARTIAL host evidence, never a replacement for the
unchanged locked23.1.3 FULL5. One natural initial FULL5+PARTIAL, no rerun shopping.

Preserved local development failures, separate from compiler profile findings:
baseline ASan target list omitted ordinary newlangc (built missing unchanged
target, only failed native test rerun); first refactor build needed stdint.h
for SIZE_MAX; first bad-C alias observer needed const-void casts to avoid
comparing distinct pointer types with -Werror. All frozen sources/oracles are
unchanged. These were corrected BEFORE the natural CI pair; original logs
remain. Log display trailing whitespace is normalized, not source/C/JSON bytes.

## Reproduction and cost/stop

```sh
cmake -S . -B /tmp/p541 -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p541 -j4
/tmp/p541/newlangc_p534 --check tests/p541/two_roots.nl
/tmp/p541/p541_public_probe tests/p541/two_roots.nl
/tmp/p541/newlangc_p534 tests/p541/two_roots.nl
ctest --test-dir /tmp/p541 -R '^p(534|541)_' -V
ctest --test-dir /tmp/p541 --output-on-failure
```

Read-only predicted200–350 net Compiler C/header lines; actual266 fits that
boundary. No language/core/AST/generic/owner refactor was needed. Retained
checkpoint tables remain O(k²) memory in constructor count, and repeated
prefix/source replay also has increasing CPU cost; this is not a scalability
claim. Local sizeof(value)=1024 and sizeof(place)=248; second10-value/10-place
checkpoint tables alone add12720bytes plus registry/type/name/record/node
overhead. No total-memory/runtime/performance improvement is measured.
Only1/2 originals are calibrated; other statements, Array Copy proof,
general callers/ABI, element projection/operations and Ring remain outside
this closed producer/consumer profile and are not silently claimed.

VALUE-ALIGNMENT-RISK (separate): fixing already adopted natural independent
Array eligibility is a reusable capability, not nonCopy Frame/Ring FULL/EMPTY/
refund/teardown or reduced author+ordinary-caller L/R/P/V/C audit closure.
Safe Rust tagged Ring/factored C/Zig/HOLD remain real alternatives.
Product GO stays HOLD; Coordination decides adoption, new-SHA independent
verification and any future investment. Original H1/STB contracts unchanged.

DRAFT / DO NOT MERGE. No original PR539/main/canonical/R edits, no self-close,
merge or other Track launch. One terminal Issue541 report and STOP.
