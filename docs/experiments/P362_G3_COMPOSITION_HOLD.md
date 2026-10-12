# P362 G3: unchanged-compiler source/owned composition barrier

**P G3 COMPOSITION: HOLD — FIRST SOURCE/OWNED/PUBLIC PROOF BARRIER**

Evidence-only child of PR353 `82578ff6eb23970e3ae934c98a38383ca01468f7`.
DRAFT / DO NOT MERGE. No compiler, public API, flags, original sources or
specification changes. Canonical Draft17.30 and DI015 HOLD remain authoritative.
The experiment was authorized by Issue362 / accepted M361 design-only decision.
Preflight and conditional integration decision: `evidence/p362/preflight.md`.

## Fixed genuine source and mandatory unchanged first run

Inputs were hash-pinned before nonblind historical calibration:

| Input | SHA256 | Immutable origin |
|---|---|---|
| `first_adoption.nl` | `bef64179aca653b04a3fe20bc3d03c81906cc7a2fd7639ffb8bb1757b3c82ffc` | PR335 `89244619f5f9e152aede362726368e3b09398ca1:evidence/p334/dst_child_return_old.nl` |
| `returned_owner_calibration.nl` | `bd7692f8118c8e4de2b7662b0f90fcf503a548d9948bfded27bd430ad127716d` | PR331 `46a02e60cabce1b63cc3db4bc9035fe06f5f8009:evidence/p329/returned_owner.nl` |

The first source retains five independent fallible original src/A/B/C/dst
roots; six setup links; B whole consume through ordinary DetachedHolder;
two real named B helpers setting prev/next to None; direct A.next/C.prev detach;
exactly one direct original dst.child None→Some(original B); ordinary PURE
returned complete B+dst TreeBOwner; donor A/C/src release followed by finish_two
using only the returned owner. B.prev stays None; no second adoption is attempted.

Three configurations were built from the unchanged fixed compiler before edits:
all ten inherited experimental flags ON/new finite replay ON; old ten ON/new OFF;
old ten OFF/new ON. Exact inherited keys come from immutable P338 build.sh.

| Profile | First adoption | No-adoption returned-owner calibration |
|---|---|---|
| old ten ON / replay ON | parser0, registration3, CLI3, precision | parser0, registration3, CLI3, precision |
| old ten ON / replay OFF | parser0, registration3 at ninth replace, CLI3 | parser0, registration0, main owned0, CLI4 backend unsupported |
| old ten OFF / replay ON | parser2 AVS-DECL-PROFILE, CLI3 | parser2 AVS-DECL-PROFILE, CLI3 |

With replay ON, public `CAPTURED-CLOSURE-PRECISION` covers the deep success arm:
first adoption **136:37–249:38**, bytes **6961–16406**. A test-only copied TU/archive
(`trace.py`, explicit `NEWLANG_TEST_P362_TRACE` guard) logs the first actual
private stop: **UNMODELLED_CALLEE, node149, bytes12007–12053** (first B helper).
Nine direct records exist in the **unsealed failed registration**, never as a
successful owned artifact. Tracing leaves the original static archive unchanged
and reproduces the same public refusal and unchanged semantic snapshot.

With replay OFF, first refusal is the ninth direct replace, **232:49–116**,
bytes **14661–14728**. Historical unchanged PR326 code reproduced that same
refusal and a same-context actual-owned8 retry (see `pr326-refusal-retry.jsonl`).
Parser-profile rejection and semantic precision are distinct from illegal
ownership and backend unsupported. No backend emission was performed.

## Real owned calibration and missing composition proof

The unchanged legacy calibration really generates owned main. Its five public
captured closure validators, two source-defined scoped helper validators and
one requiring-two-root terminal validator all return0; caller tape has8 direct
Changes and helper bodies separately have1 each. Original B R3/root31/inc34,
A29/D34 and original dst R5/root57/inc60,A57/D62 remain identifiable. PURE whole
call235 returns complete original owner174, receiver/detached inputs170/172;
release order R2,R4,R1 then R3,R5 is donor-first/receiver-late. See the exact
PR331 public observer output, extracted unchanged apart from its include path.

New public-only `owned_edges.c` compares all five heap roots and all20 fixed
field Place/Value views, including payload:u8, at four fragment boundaries.
Those heap views are equal at D6→C1, C1→C2, C2→D7 and D8→J.entry. Nonetheless
B Allocation29/Domain34 current owner places change **30/33→95/96** through the
source whole moves. At owner assembly entry they become AGGREGATE-carried by172;
dst Allocation57/Domain62 become AGGREGATE-carried by170. Seventeen nonCopy
checked source nodes occur in the D6→C1 gap. Raw heap equality plus isolated
validator success does not rederive their chronological responsibility transfer.

The existing finite replay rejects registered calls and requires original A/D
PLACE custody. Simply admitting recognized calls or ignoring changed A/D carriers
would leave the Issue362 source-operation/callee-continuation/M/J proof contract
unproved. Existing structural owner shape checks are useful input, not a joined
temporal proof. No sound finite private custody-and-effect rederivation was
established in this pilot; consequently no compiler extension or permissive
fallback was made. This is not a claim that a later bounded private witness is
impossible or requires a new language rule.

**No composed ninth owned positive exists.** Public missed-second-callee poison,
hidden same-byte scalar Reset, joined owner mutations and composition-stage OOM
are **UNTESTED**; they were not represented by forged host positives. A future
bounded proposal needs explicit source-issued custody steps M/J, validated known
callee splice and caller continuation, full20-field/occurrence adjacency, actual
five+two+one public revalidation, then both mandatory public poison rejections.

## Controls, rollback and OOM

Three source-derived negative controls produce registration semantic status1,
unchanged snapshot/no owned artifact: wrong original dst Domain and wrong moved
B Domain (`ALLOCATED-DOMAIN-MISMATCH`), read ref to write formal (`P3-TYPE-MISMATCH`).
Source hashes, exact spans and CLI diagnostics are retained. Failed ninth legacy
registration followed by the complete owned8 calibration in the same context
succeeds. Legacy sampled registration/owned acquisition and four public validator
OOM routes also preserve rollback/retry. These samples do **not** exercise the
unimplemented joined composition.

P352 W1/W2 actual9 owned/public controls pass. The immutable independent R359
PR360 inputs and public observer are extracted into the build directory and run
without modification: two distinct actual9 owned/public positives; five semantic
negative rejections and two unsupported profile negative rejections. This rerun
is P regression evidence, not a newly launched independent Track R.

## Reproduce and CI

```
git fetch --no-tags origin 46a02e60cabce1b63cc3db4bc9035fe06f5f8009 276ca88d34ef1d54ee0ccda4c9a4f47956ddb26f
python3 scripts/bootstrap.py
. .deps/activate.sh
bash evidence/p362/run.sh build-g3
```

The runner builds three profiles, checks the fixed source hashes, executes
public boundary/owned controls and legacy sampled OOM, traces a copied TU only,
and reruns immutable R359. Output: `build-g3-observations.json`,
`build-g3-both/p362-trace.json`, `build-g3-new-only/p362-r359.json`.
Local actual records are in `evidence/p362/`; per-configuration CI uploads fresh
outputs. Diagnostic JSON records observations and **grants no ownership/Domain
authority**. Dedicated CI covers GCC Debug/Release, Clang, ASan/LSan and UBSan;
ordinary inherited CI is unchanged. Actual job statuses/IDs belong in the terminal
Issue362 report; CI does not establish composed success.

No H1 PASS/FAIL, native execution, second adoption, merge, selection or new Track
is inferred. Work stops with the first authentic composition proof barrier.
