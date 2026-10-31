# P982 / C98: T originals after the original H terminal and teardown

The backend now admits two direct T originals after its real H producer,
returned local LiveTail, terminal, and both original H cleanups in the same
CHECK-issued arm. It keeps the existing Copy current-function, root, field,
sibling and loan checks. A source reorder or a new owner receipt is not used.

Authority: main438170098d5b98096f1ad1f1f200dc1f326c75c3, implementation baseline
d579c761a78e3c84765e35061f77936789788d82, CURRENT_SPEC -> Draft17.34. Exact parent
PR976 is b9224f1aa67329e3edead4d5d6c59cc911141cd1, tree
9c491586b6f84bb026e896862aa2f24decf6a467. Work is on the separate
p982-postterminal-t-original-c17 branch. KEEP Draft17.34 / REPLACE NONE;
public source/issuer/API/ABI, generic H/T/CFG/MIR/LLVM, and toolchain changes
remain deferred. Original parent branches and independent R inputs are untouched.

## Private admission and its authority

Five predicate groups form the additional backend review dependency closure:

| Predicate | Original authority / lifetime |
| --- | --- |
| Actual terminal traversal | Record issuer/node only after the old owner-entry/post/actual-operand checks and emitted terminal body complete |
| Present H callable | Compare retained actual body/plan/source, kind/result/role/effect/signature against that call's own CHECK entry snapshot |
| Both H originals ended | Match actual ancestor root/incarnation/backing/domain, stage8, no live affine/raw responsibility, and no pending packet/owner call |
| Same-arm postterminal T | Require the original Copy issuer and validated completed caller graph, producer/terminal/root/range relation, no earlier T lane, exactly two fresh T roots and one known Copy call |
| IF retired backing prefix | Keep full type/binding/place/value/scope/domain/backing equality; only the two authenticated ended H regions with zero raw intervals omit an inaccessible raw-byte read |

The three production files are checked_c_node.c, checked_c_copy_internal.h and
checked_c_copy_if_internal.h. These are synchronous borrowed backend observations;
no new semantic capability is created or cached beyond the live CHECK owner.
The old T lanes and canonical child destruction are unchanged. T bind/field/loan
consumers still use original nominal/place/incarnation/current child facts.

## Independent inputs and first results

Before execution, the full new second_true source, oracle and every input hash
were published in Issue982 comment6461615400; source-only commit
5b97d8c3c3c2c0649ffd6838a80a682154f968fd. after_true/false are byte-identical
original P978-owned source regressions: their old Gate0 RC4 is preserved. The new
second family changes H names/payloads, T declared-field order, and calls the
second T original's index0 before writing its old Copy to the first root's index1.
No R963/R970/R979/new independent R source, oracle or fixture was read or reused.

| Source | H payloads | Final T field values in declaration order | Old / displaced |
| --- | --- | --- | --- |
| after_true | 67 /149 | 37,181 /103,5 | 103 /211 |
| after_false | 67 /149 | 37,23 /103,5 | 103 /211 |
| second_true | 53 /187 | 197,13 /86,222 | 13 /41 |
| second_false | 53 /187 | 197,13 /172,222 | 13 /41 |

Source CHECK succeeds on all four. On the unchanged parent, both new second
inputs returned C17 RC4. The first production change is
3a25ecce3db54ed2bb96e346494822e9b2983b7f; it emits C17 for all four. The tested
code/harness checkpoint is b5b4da0b95af527db9d571c193527cd1575d1156, tree
d76a0a6a56c49d0cc27f0044e6ee39cceb71b6f8. Later evidence publication does not
change those compiler or harness inputs.

All first REDs remain separate from later success:

- 54dd510: the new strict probe build rejected two implicit const-context
  conversions. No negative/native test ran. Explicit casts are restricted to
  genuine live CHECK-owned objects in the restored negative probe.
- cc44cdc: the first auth probe stopped before mutation because it assumed
  arm node0 was a literal. Actual order was BLOCK/U8_LITERAL/IF_ARM; edge0,
  count3, root3 and the original-root extent control were valid. The new probe
  follows public root -> body -> tail. Production and source/oracle stayed fixed.
- Initial ASan/UBSan full attempts used four configurations, each CTest-j3,
  on three CPUs. Stock90s timeouts hit old P633 CLI and P517 native tests.
  Raw first logs are retained. Same SHA/source/oracle/stock timeout isolated-j1
  passed14.31s and19.53s; full sanitizer attempts then used-j1. No warning,
  expectation or timeout was weakened, and no manual Actions rerun occurred.

The old harness-freeze-v1/v2 files are immutable historical records, not current
worktree hash expectations. harness-freeze-v3 and source-freeze-v1 bind the final
production/harness/source/oracle; freeze_test verifies them. They are not updated
to erase a failure.

## Validation evidence

GCC, Clang23.1.3, ASan+LSan and UBSan each pass all545 frontend CTests.
Each configuration independently reacquires494 original ownership trials and502
old reached OOM records (CHECK320/definition182), all matching the old freeze.
New coverage per configuration is80 restored metadata faults and1788 reached
emitter OOM positions (447 per input). Strict C17/native totals32 raw runs and96
physical heap0/1/2 observations across four configurations.

The final measured per-configuration results and all reached records are in
evidence/local-results.json and the evidence manifests. This is a finite gate:
original535 frontend tests plus10 new tests, genuine A/B/F CHECK worlds,
20 restored metadata faults per input, both T fields/old Copy/siblings and
original H carrier transfer/cleanup. Passive observations read only live guest
objects; finish uses saved scalar data after lexical exit. Heap0/1 have no T
original, heap2 frees both H originals before either T bind.

The new emitter OOM sweep records every actual healthy-invocation allocation
position for each input. A failed attempt must leave both output slots
unpublished, the CHECK snapshot unchanged, original proof still valid and an
exact-byte C17 retry. Some inherited boolean validator failures report
UNSUPPORTED rather than OUT_OF_MEMORY; status counts are retained rather than
regraded. No failure is forced through to C17/native. Original P494 ownership
trials and P502 CHECK/definition records are reacquired independently and compared
in full with the unchanged old freeze, not inferred from the new sweep.

Exact unmodified C17 text is retained as evidence/generated/*.c.txt, with SHA256;
it is not a new handwritten C fixture or a formatted replacement. The ordinary
CLI output is what strict GCC/Clang C17 O0/O2 compile. Native/error/named-output
logs and the first build/assert/timeout REDs remain individually hashed.

## Limits and handback

This does not establish all H+T interleavings, generic effects/control flow,
arbitrary forged pointer/length/opaque owner safety or universal OOM coverage.
In particular, another independently reported T-before-H failure is not solved
by claiming success on these postterminal inputs. No independent fixed-code F/R
was launched or substituted by P tests. The new stacked PR stays DRAFT HOLD;
READY/merge and Product GO are not granted. New-series CI is limited to two
natural FULL5+PARTIAL pairs/12 jobs, no manual same-SHA rerun. Final CI run IDs,
actual merge-checkout tree and remaining budget are reported in Issue982
without an evidence-only extra push consuming another cycle.

VALUE-ALIGNMENT-RISK: the five added private predicate groups increase compiler
audit and maintenance dependencies. Their provenance is the original CHECK,
not numeric IDs, retired addresses or user unchecked claims. No new library-author
or ordinary-safe-caller duty is introduced. Actual human UB audit/re-audit
closure reduction versus well-factored C, performance and maintenance cost remain
UNMEASURED. Product GO PAUSED / H1 HOLD.
