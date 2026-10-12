# O5 — Production-aware semantic-state algorithms (Issue #354)

**O5 ALGORITHM RESEARCH VALUE (LIMITED). DRAFT / DO NOT MERGE.** This is a Python 3.12+ standard-library-only, deterministic **synthetic semantic algorithm laboratory**, not source-issued C authority, native backend execution, formal soundness, or a new canonical rule.

## Frozen authority

- Compiler/main `a5343bf70004b30a8b026e52aa2fde93b5920be0`, Canonical Draft17.30 (spec Git blob `db8fe08f811acc8286f2d37e9d3229b5d7a67067`).
- FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`.
- This evidence branch **is based directly on O1/O2 PR344 exact head** `8cdcdd7e2f5fa3219ff9fed1a3ae34d94705ce9c`; the O1 files remain unchanged.
- Experimental C PR326 `de9c4b175784f484675b670a109f62edeaf35de1`; source W1/W2 PR340 `dc0fc7e7d2f9f92286973a4f5062089acf0d85f2`; C G1 chronology PR351 `cc1707186e1338d71fdfba26bf90134d54f5de3e`.
- Concurrent P352 / unadjudicated PR353 observed head `82578ff6eb23970e3ae934c98a38383ca01468f7` during this study; **NOT USED as input, acceptance reference or implementation basis**.
- M #336 `HOLD DESIGN`. No existing compiler, FormalProof, spec, CI, O1 files or other branches changed.

## Run

From this directory:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -p test_o5.py -v
PYTHONDONTWRITEBYTECODE=1 python3 run_study.py > reproduced-results.json
python3 -c 'import json; r=json.load(open("reproduced-results.json")); print(r["passed"],r["positive_pairs"],r["adversarial_cases"],r["critical_snapshot_reject_delta_accept"])'
```

Python >=3.12, stdlib only; no C compilation, internet or O1 runtime imports. The generator has deterministic seed `20261012` and each case includes a stable `trace_sha256`. Exact `results.json` full corpus is regenerable by the runner; median timings and tracemalloc peaks naturally vary with interpreter and host. Machine-recorded medians and file hashes of the independently exercised local source are in `measurements.json`.

## Algorithms: genuinely separate state/verification implementations

| | `full_snapshot.py` | `persistent_delta.py` |
| --- | --- | --- |
| Representation | Full immutable-ish Python dict checkpoint clone of all 25 root/field entries per mutation, retain all past versions | Flat 25-entry current frontier + immutable `VersionNode(parent, updates)`, two updated semantic entries (root fact and selected field) per mutation |
| Synthetic authority assumptions | Five distinct R/O/incarnation/A/D/owner identities, each one has 3 pointer-Option fixed fields plus 1 scalar u8 | Same **assumed input** vocabulary only; no source C rights |
| Currentness | Compare all 25 typed before/after witness entries, old End/new Fresh occurrence and unchanged siblings | Compare against independently maintained current frontier, then commit precisely two typed changes, share 23 unchanged entries |
| Source event order | Source-step index and checked-record ordinal must be bijective; only known inert Copy may occur as an unrecorded gap | Separate monotone event cursor and source-step mapping; unrecorded mutation is forbidden even when scalar numerical value is unchanged |
| READ→WRITE | Correct original-D READ stability loan, derived WRITE root ref, typed projection, non-escaping closing scope | Independently implemented same obligations |
| Unknown/unsupported | `PRECISION` for Unknown effect or event budget, `UNSUPPORTED` for unmodeled owner move | Same statuses; never treats unknown as ACCEPT |

The algorithms **do not import/call each other**, nor O1's exact/abstract/P350 C. The test suite statically inspects imports and monkeypatches the other implementation to throw while the selected checker independently accepts an admissible synthetic case. Both share `model.py` types and **untrusted** generator fixtures; their agreement is NOT proof against shared-model errors.

### Minimal independent P350-like witness

`fixtures("directory", 2, scalar_gap="hidden")`:
1. Legal `directory.child None→Some(slot2)` Change; fixed-field incarnation preserved, new value fact and Some occurrence minted.
2. Unrecorded but **locally valid** `slot4.payload u8(3)→u8(3)` whole scalar Change/Reset. Bytes are identical, but its current ValueFact is new. No ledger event accounts for the effect.
3. Legal standalone `directory.child Some(slot2)→None` Change with internally coherent checked primitive before/after snapshot.

Both checkers reject the chronology on the hidden scalar source step **`scalar:gap:0205cd15`**: full `UNRECORDED_CHANGE_OR_ORDER`; delta `SOURCE_EVENT_BIJECTION`. A **recorded** scalar Change is ACCEPT; a truly inert Copy gap is ACCEPT; Unknown effect is PRECISION. A reused old Some occurrence, even with the same pointer target, is rejected independently.

### Corpus and measured limits

Actual Python **3.13.5/Linux** clean extracted ZIP re-test: 10 unittest methods PASS (including 128 deterministic randomized safe/poison pairs); 8 baseline W1/W2-like positive programs (2/4/9/12 caller changes); **16 corruption families×2=32 negative semantic rejections**; plus scalar-gap comparison in both motifs: 2 hidden rejects, 2 recorded accepts, 2 Copy accepts, 2 Unknown precision. **0 observed full-snapshot SEMANTIC_REJECT / delta ACCEPT**. Event budget8 on otherwise valid 12 reports PRECISION, not semantic refusal. No test is a C source-owned acceptance or real nine-caller compile.

With five original roots, twenty fixed fields and five root ValueFacts = **25 entries**, retained history entry counts:

| Change count | Snapshot newly cloned | Delta stored updates | Snapshot peak semantic entries | Delta peak entries |
| ---: | ---: | ---: | ---: | ---: |
| 2 | 50 | 4 | 75 | 29 |
| 4 | 100 | 8 | 125 | 33 |
| 9 | 225 | 18 | 250 | 43 |
| 12 | 300 | 24 | 325 | 49 |

31-run medians **with tracemalloc active** (not native C benchmark): directory/12 snapshot ~1.04ms / 14,922B peak, delta ~1.83ms / 8,686B peak; scheduler/12 snapshot ~1.11ms, delta ~2.33ms. Delta reduces stored history but is **not reliably faster** because all 25 input witness entries remain verified at every checkpoint. No asymptotic wall-time claim. Counts above are deterministic modeled state slots, NOT measured byte usage.

## Narrow Production recommendation / falsifier

Recommend to M/P **a typed incremental currentness frontier + source-event coverage ledger**, with root/field ValueFact and conditional Some occurrence distinct from actual Option/scalar bytes. Every semantic effect, including a same-u8 Change/Reset, must be accounted for by an ordered source Change entry or proved inert by a bounded known-effect classification. Verify original R/O/inc/A/D, correct-D READ stability → WRITE root ref → typed field ProjectionId, known-disjoint siblings and nonescaping scope. **A flat frontier with sparse immutable undo/delta records only when rollback/diagnostics require history** is worth comparing to simple snapshots in actual C. A persistent ancestry chain does not guarantee speed; keep original source grant checks, proof boundaries and OOM transaction semantics. Do not generalize from tiny Python timing to actual C.

Falsifier: two individually valid source-owned checked Changes enclosing an *unrecorded* same-value scalar Change/Reset; any incremental verifier ACCEPT there is unsound for that chronology invariant. Additionally require independent negative tests for known alias/overlap, stale occurrence, misleading equal bytes, branch-world qualification and Unknown effects before promotion.

**Out of scope:** real source→neutral authority, actual C9/C12 admission, caller/callee substitutions, nonCopy B+dst TreeBOwner, transactional OOM, CFG/joins, native execution, real 0–5 release worlds, cJSON adoption or H1 PASS. Track O PROVISIONAL; M336 HOLD DESIGN unchanged.
