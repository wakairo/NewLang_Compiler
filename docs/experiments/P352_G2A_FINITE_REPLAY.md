# P352 G2a: finite non-cJSON ninth source-owned replay — UNSELECTED

Track: P. Issue [#352](https://github.com/wakairo/NewLang_Compiler/issues/352).
DRAFT / DO NOT MERGE. No production/source-language selection or H1 PASS.

## Authority and historical gate

Parent **PR351 cc1707186e1338d71fdfba26bf90134d54f5de3e**, not its mutable
branch. Underlying frozen compiler PR326 de9c4b175784f484675b670a109f62edeaf35de1;
source evidence PR340 dc0fc7e7d2f9f92286973a4f5062089acf0d85f2.
Compiler/main a5343bf70004b30a8b026e52aa2fde93b5920be0; canonical Draft17.30
blob db8fe08f811acc8286f2d37e9d3229b5d7a67067; FormalProof/main
a8de4ebb4099f4742fc161d26867740dcac48414, F59 proof-only.
Draft17.31/DI015 remains unadopted HOLD. No PR298 import.

Preflight BEFORE code: [evidence/p352/preflight.md](../../evidence/p352/preflight.md).
Read Process §4.1.1/4.2, Design Decision Procedure A–C, current spec and ledger,
M349 proposal/adjudication and P350 acceptance. KEEP DI001/007/009/010/014 and
canonical original R/O/incarnation/A/D, scoped correct-D loan, projection,
Change/Reset/Option occurrence and static cleanup. DI011–013 KEEP only their
existing bounded calls, not extended here. DEFER DI002/006 general spelling,
privacy, general effects/borrow/owners/FFI/ABI and proposed DI015. No normative
rule is replaced; no Ledger amendment is justified by implementation evidence.
Old Surface/M0 prototypes and full inaccessible M9 history not exhaustively
reviewed. This limitation does not establish the absence of past alternatives.

## Fresh source and the first unchanged-compiler boundary

Exactly two positive programs, derived from the genuine non-cJSON W1/W2
allocation-and-teardown substrate; only append direct lexical loan/write
blocks. Original five fallible grants, H initialization and every 0–5 cleanup
arm remain byte-identical. No additional heap Node, named helper, owner move,
cJSON detach/adoption, source syntax or branch.

| Source | SHA256 | Nine operations |
|---|---|---|
| w1_device_refresh_9.nl | 1f22d9af5e2120939a15b89150963ddc9d9d823d06f7e1dddd728eee65670503 | directory.child Some(slot2), None, Some(slot3), None, Some(slot4), None, Some(slot5), None, Some(slot2) |
| w2_scheduler_rotation_9.nl | 078f20bbc47079c33705387240d09a38b96d598a8c907f7ee4e8676c5326f967 | X.next=Some(b), Y.prev=Some(d), X.next=Some(c), Y.prev=None, X.next=Some(d), Y.prev=Some(b), X.next=None, Y.prev=Some(c), X.next=Some(b) |

**Measured BEFORE compiler edits:** parser0, registration3
`CAPTURED-CLOSURE-PRECISION`, CLI3, no owned entry, snapshot unchanged.
W1 span bytes12162–12219, line181:49–106:
`replace(write_7@child,Option<ptr<Node>>::Some(ptr_slot5))`.
W2 bytes12308–12353, line181:49–94:
`replace(write_7@next,Option<ptr<Node>>::None)`.
No ninth primitive judgment was reached. `nl_captured_change_begin` requires
a prior second B helper at count6 in the frozen profile; eight-slot storage is
an additional later limit. See `frozen-observations.json` for actual diagnostics.

## Narrow internal integration

Only CMake default-OFF `NEWLANG_EXPERIMENTAL_FINITE_CHANGE_REPLAY`, private
`semantic_internal.h` and `captured_closure.c` change. No public header/API,
semantic primitive, source grammar, canonical or backend changes. **The new
option works alone, with all old ten options OFF.** A separate local run also
verified nine owned events with all old ten ON/new ON.

Nine owned snapshot slots accompany **G1 chronology ported into the existing
PUBLIC `nl_checked_captured_closure_validate` route**. It executes on the
completed source-generated five-root success arm during registration/sealing
and every public revalidation, before the original release trace. Original
tuples come from that trace's actual source issuer/initialize ledger. Reused
primitive clone/reapply `nl_fixed_change` and source terminal proof are explicit;
they are not new independent algorithms.

New independent checks: exact direct REPLACE node/captured-record bijection,
source span/order, correct live original A/D custody before and after each
primitive, previous lexical scope ended, all four heap field states and A/D
continuous between successive events, and every non-target field/value
preserved across each Change. Parent aggregate reanchoring of unchanged member
values is normalized only for the sibling value comparison; the primitive
independently replays actual custody. Gap whitelist admits Copy construction
and proved scoped read/write projections only. Registered callees and unknown
intermediate effects/nonCopy moves fail closed with precision. Existing finite
source graph and 0–5 teardown validation still apply; no new owner registry.

Legacy seventh/eighth H1 predicates and storage remain intact when the new
option is OFF. `legacy_equivalence.py` proves **preprocessed original TU equals
candidate TU**, whitespace-normalized, in default OFF and original-ten-ON/new-OFF
modes, including included private structure definitions. This is stronger than
inferring equivalence from passing two positive samples. No H1 ninth/tenth
source is executed by this experiment.

## Actual source-owned proof and attacks

Both new programs: parser0/register0/main0, nonNULL actual owned entry,
**9 actual captured Changes**, five public closure validators0, six static
release worlds `[1,1,1,1,1,1]`, successful source frees `[5,4,3,2,1]`.
Public-only observation tools link the NORMAL production semantic archive.
Original Allocation values `[1,15,29,43,57]`, Domain values `[6,20,34,48,62]`;
original root/incarnation/R identities preserved, D1 W1 and D1/D2 W2. Every
Change preserves 19 non-target fields; old Some occurrence ends and new Some
is fresh, old returned Copy packet carries no new A/D. Source spans map to
retained original bytes. Existing W1-2/W2-4 pass the new opt-in as well.
CLI4 `V1-BACKEND-UNSUPPORTED`: source-owned success is **not native execution**.

FIRST after obtaining nine-owned positives, actual source negatives change
only the ninth loan to another existing original D. Both parser0,
registration1 `ALLOCATED-DOMAIN-MISMATCH`, CLI3, no negative owned artifact,
snapshot unchanged, same-context positive retry0. Not a precision refusal.
Manifest `source_hashes.json` pins all four new sources.

Test-only corruption of real owned artifacts (copied private test archive;
normal library has no hooks) rejects by the **PUBLIC validator**, status3:
missing ninth captured record; duplicate/reordered record; ninth checked opcode
missing; spliced span; duplicate owned node; ninth acquired ref wrong Domain;
Unknown provenance; revived ended old Some; ninth new occurrence claimed old;
unrelated fourth scalar fact; a valid primitive pair with broken cross-event
heap adjacency; unmodelled intermediate TAKE. All restore/revalidate0.

The disconnected ninth pair is derived from the real ninth before world by
an extra same-value Copy scalar Change, then replay of the actual ninth link.
Its individual primitive validator0; full public validator3. No invented
Allocation/Domain/seed world or published forged positive. This demonstrates
independent value beyond nine separate primitive checks.

OOM: per motif 11 registration attempts and 11 entry-check attempts, including
EXACT ninth capture calloc, before clone and after clone fault tags (only in
test TU), early/middle/last sampled allocations; all4, rollback snapshots equal,
failed entry output NULL, same-context source retry produces real owned9 and
public0. The test-only capture tag is volatile: initial GCC Release CI exposed that a
nonvolatile tag around builtin calloc can be optimized away, preventing actual
fault injection (not a compiler OOM acceptance). Local Release rerun passed
after fixing only the test tag. First-entry allocation calibration warms existing body cache to avoid
injecting beyond the subsequent attempt's end. Public-validator 8 sampled
OOMs all4 and same-live-artifact retry0. This is sampled, not exhaustive. No
partial exposed owned output, double free or sanitizer failure in completed
runs. Snapshot equality covers the public semantic snapshot, not a serialization
of every private byte. Hostile snapshots remain test-local and destroyed once.

## Reproduction and CI

From this branch with locked LLVM/bootstrap activated and CMake>=3.28,<4:

```sh
bash evidence/p352/run.sh build-g2a
# Optional matrix configurations:
CC=clang-23 P352_SANITIZER=address ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  bash evidence/p352/run.sh build-g2a-asan
CC=clang-23 P352_SANITIZER=undefined UBSAN_OPTIONS=halt_on_error=1 \
  bash evidence/p352/run.sh build-g2a-ubsan
```

Runner creates a **detached immutable PR351 worktree**, executes unchanged G1
and P338 controls (including old true wrong-D/same-context retry), then compiles
candidate with just the new opt-in. It runs exact real-source boundary, normal
public observer, public corruption and OOM tests twice per motif; nondeterminism
or missing evidence fails assertions. No fabricated ownership JSON is imported.
`newlangc` and normal library hashes are protected during private hook linking.

Dedicated workflow: GCC Debug, GCC Release, Clang, ASan+LSan, UBSan, plus default
OFF regression; uploads actual per-job observations and frozen/G1 controls.
Actual CI status and immutable candidate commit are recorded in the ONE terminal
Issue352 report, not predicted here. Local machine observations are explicitly
marked with revision/dirty state at time of capture; CI publishes exact head.
Local LSan cannot operate under ptrace; remote CI must supply detect_leaks=1.

## Limits / next independent test

This is a finite, at-most-nine direct-caller proof on five statically nested H,
not a general source/effect/borrow/callee/owner framework or production-ready
selection. Private capacity is still bounded; no tenth event claim. No cJSON H1
ninth/tenth, no returned K_B+K_dst contract, no backend extension or native claim.
The arbitrary JSON/neutral-IR authority bridge remains outside scope. Any new
H1 source trial needs separate authorization after independent R adversarial
source and owned-corruption/OOM audit; do not launch that Track here.
