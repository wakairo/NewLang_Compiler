# P338 — unchanged-compiler non-cJSON source admission

Track: P. **W1/W2 BOTH SOURCE-OWNED CLEAN (LIMITED)** — independently meaningful
short prefixes only; UNSELECTED experimental evidence, DO NOT MERGE.

## Authority, activation and historical gate

Compiler/main `a5343bf70004b30a8b026e52aa2fde93b5920be0`, CURRENT_SPEC canonical
Draft17.30; FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`, F59 proof-only.
All executions use unchanged compiler PR326
`de9c4b175784f484675b670a109f62edeaf35de1` with its TEN existing default-OFF
options explicitly ON in an isolated Debug build. No new option.
PR331 `46a02e60cabce1b63cc3db4bc9035fe06f5f8009` supplies ONLY pinned source/public
observer calibration. PR335 `89244619f5f9e152aede362726368e3b09398ca1` remains unchanged.
All old/new experimental PRs DRAFT / DO NOT MERGE. Draft17.31/DI015 remain HOLD.

Read Issue338, V337 terminal6115282167 and Coordination acceptance6115446783,
M336 terminal6113627313, P334 terminal6113526474, original H1 Issue268 and
Process §§4.1/4.2/6/7/8/9 before new source experiments. User launch authorizes this
one bounded session. No concurrent Track O sources consulted or worker started.

Historical design audit: **N/A — source/evidence-only admission measurement,
no normative/source/API/profile/compiler changes.** KEEP Draft17.30 §§3.2b,
14.5,17.4,26 and DI001/006/007/009/010/014: original fallible grant, original
Domain-scoped write ref, fixed ProjectionId, current facts and conditional
occurrences; no rights from Copy pointer. DEFER general effects/borrow/owner,
modules/privacy (DI002), capacity expansion, eleventh flag, certificate design
and any wider source selection. M336/V337/P334 are experimental evidence, not
normative authority. Ledger absence is not proof of no old proposal; unavailable
private chats/old prototype archives were not exhaustively searched. No historical
rule intentionally replaced or foreclosed.

## What was actually tried

Each motif has **one** faithful short source, no syntax calibration or positive
variant. Each also has the expressly requested **one wrong-original-Domain
negative**, after proper semantics was reached. There is no subsequent extension.
Five allocation/initialization/failure-cleanup syntax ladders are calibrated from
P329. The entire cJSON six-link and B/B/A/C detach sequence, Holder, owner-return
functions and success cleanup were removed. Reusing allocator syntax is not
reusing cJSON topology: the new actual link transitions below differ materially.
These are desk-defined motifs, not ports of an authentic upstream device product
or scheduler. Option pointers are NONOWNING Copy links; list policy is separate
from memory safety.

| Source | Actual all-five-success sequence | SHA256 |
| --- | --- | --- |
| calibration_returned_owner.nl | Immutable P329 returned-owner calibration ONLY | bd7692f8118c8e4de2b7662b0f90fcf503a548d9948bfded27bd430ad127716d |
| w1_device_refresh.nl | directory.child None→Some(slot2)→None | e39eb75d1161e90caf8392626462d086e8d31b7552e646ba7862e34900b1893f |
| w2_scheduler_rotation.nl | X.next None→Some(task_b); Y.prev None→Some(task_d); X.next Some(task_b)→Some(task_c); Y.prev Some(task_d)→None | e64160665a22e185cb8191afecbad9a14d1c5a5411afb3da17832ab6d5b5427c |

Each source has five independent actual `try_allocate_one<Node>()` issuer sites,
its own original Allocation/Storage, initialize and LifetimeDomain, and explicit
matched cleanup on 0–5 success worlds. Success path independently discharges
all five in order [5,4,3,2,1], unlike P329 donor-first receiver-only terminal.
No sixth heap allocation, hidden registry, unchecked, helper bypass or safe ref
from an unqualified Copy pointer. W1's repeated same child is different from
cJSON; W2 changes first root next and second root prev and reupdates both.

## Real boundary, actual owned artifacts and public validators

Calibration: parser0/register0/main0, actual nonNULL owned; five captured,
two scoped-call and one requiring-two-root public validators all0; release order
[2,4,1,3,5], original identities and static histogram [1,1,1,1,1,1].

**Both new positives:** parser0/register0/main0, actual nonNULL source-generated
checked-owned entry and its owned five nested allocation arms. On EACH of the
five enclosing actual match nodes, `nl_checked_captured_closure_validate` returns
0; enclosing public snapshot stays unchanged. This validator rederives real
Change primitives and scoped write authority, original release worlds and
source chronology. It is not host-created semantic evidence. The new observer
uses only public borrowed views, no seeded/mutated/cloned host world or private
headers. No scoped-helper/two-root-owner validator is claimed for these sources.

The new public observer also reads each actual Change.before/after: matching
original R/O-incarnation/A/D and field ProjectionId, correct original-D scoped
read loan and write ref, old/new facts and variants, old Some occurrence ended,
new Some occurrence live/fresh, target original ptr with no A/D authority,
all19 other fixed fields unchanged, five unique current A/D bindings unchanged,
previous scopes ended before the next operation and all scopes ended at terminal.

Original Allocation values [1,15,29,43,57] and Domain values [6,20,34,48,62]
are preserved across every snapshot. Every event preserves all five original
root incarnations. Success terminals end all original roots/Domains; the actual
public closure histogram establishes each static 0–5 cleanup world once.
These IDs are anchored in the corresponding owned artifact; equal numeric IDs
in another workload are not authority sharing.

| Motif/event | Root/incarnation; original R/D | Projection (nominal,index), child | Old/new variant; target | Old/new occurrence; fact | Lexical scope |
| --- | --- | --- | --- | --- | --- |
| W1 0 | 5/8;1/1 | (10,2),11 | None→Some;slot2/R2 | 0→1;11→68 | 6 |
| W1 1 | 5/8;1/1 | (10,2),11 | Some→None | 1→0;68→74 | 7 |
| W2 0 | 5/8;1/1 | (10,0),9 | None→Some;task_b/R3 | 0→1;9→68 | 6 |
| W2 1 | 18/21;2/2 | (10,1),23 | None→Some;task_d/R5 | 0→2;23→74 | 7 |
| W2 2 | 5/8;1/1 | (10,0),9 | Some→Some;task_c/R4 | 1→3;68→80 | 8 |
| W2 3 | 18/21;2/2 | (10,1),23 | Some→None | 2→0;74→87 | 9 |

Actual source spans (body-retained spans are mapped by unique exact public
source-byte containment, not guessed offsets): W1 [10071,10128)157:49–106
and [10425,10471)161:49–95; W2 [10208,10265)157:49–106,
[10560,10617)161:49–106, [10912,10969)165:49–106 and
[11264,11309)169:49–94.

The first downstream unavailable boundary is **CLI4 V1-BACKEND-UNSUPPORTED**,
C stdout0 bytes, not a source/semantic precision refusal. Native NOT tested.
Two repeated CLI executions are byte-identical for each positive.

## Two small real semantic negatives

Each changes precisely one `loan_read` operand, preserving all original grants,
link operations and cleanup. W1 first directory loan incorrectly uses original
slot2 D2; W2 Y.prev loan incorrectly uses original task_c D4. Both parser0,
registration **1 = NL_CHECK_SEMANTIC_ERROR**, CLI3, not registration3 precision.
Diagnostic `error(semantic)[ALLOCATED-DOMAIN-MISMATCH]: pointer requires matching
readable live domain` is at the actual `ref_from_ptr`:

- W1 bytes[9975,10016),156:63–156:104.
- W2 bytes[10467,10507),160:63–160:103.

Hash-checked negative sources and their SHA256 are in `source_hashes.json`.
Public registration snapshot remains unchanged; main/owned entry/backend are
not attempted for the refused source. After refusal, the SAME context registers
its matching positive source, checks `main()` to real owned and reruns all five
public validators and event observations. This is actual clean retry, not host
injected restoration. No stale-occurrence/active-loan/source escape/OOM/validator
poison negative or native allocator-failure execution is claimed.

## Actual burden and limits

W1:189 physical/188 nonblank lines,59 lexical lets,5 allocation/initialize sites,
7 read-loan sites (5 init+2 changes),15 exclusive cleanup-loan sites,2 safe refs,
2 replaces,15 lexical deallocations. W2:197/196 lines,63 lets,5 allocation/init,
9 read loans (5+4),15 exclusive cleanup loans,4 refs/replaces,15 lexical
 deallocations. Cleanup sites are branch duplication, NOT15 executed frees.
No extra heap nodes, manual nested failure cleanup and one explicit scoped loan
per update. This is recorded ceremony, not a productivity benchmark against
C/Zig/Rust or evidence of zero runtime cost.

**Earliest meaningful success STOP:** W1's two-update refresh prefix and W2's
four-update cross-root rotation prefix are genuinely source-owned/publicly
validated. No extension after these successes. Full W1 second refresh and other
slot field, full W2 next rotation, seven/eight/nine+ event patterns, named-effect
substitution, generalized source/borrow/effect, public certificate tape,
real-time/device/scheduler policy and native remain UNPROVED/NOT TESTED.
This refutes a blanket claim that every non-cJSON first field sequence is
rejected; it does not contradict P334's ninth caller capacity HOLD or M336's
7th/8th specialized guards. No generic compiler implementation GO is implied;
Coordination alone can consider a separately bounded design review.

Frozen H1 remains 0/2 true adoption, no C17 native, PASS/FAIL undecided.

## Reproduction

Checksum-locked `scripts/bootstrap.py`, activated LLVM prerequisites,
CMake>=3.28,<4, C17 GCC, Python3. From exact PR326 plus evidence-only additions:

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
bash evidence/p338/build.sh build-p338
```

The fail-closed runner verifies all five source hashes, complete calibration,
actual public motif evidence, semantic negative boundaries and same-context
retry. JSON output contains full observations. Local Debug compiler binary
SHA256 604bf1e3df5e870c33dfd1dccbef9e3f175c09b5860316a507a1e8eb74457a5b.
Pinned dedicated CI's green means these bounded source-owned positives and
wrong-D refusals reproduce, **not** generic certificate/native/H1 success.
