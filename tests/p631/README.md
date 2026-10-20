# P631 / USER-CLI-3 — HOLD on required wrong-arm provenance rejection

This is **evidence only**, not an implementation candidate. Production,
CMake and both historical Python tests are restored byte-for-byte to main
`032cf60aabf4997e672bc3607cf53775d91ececb` / Draft17.32. The old oracle migration
is authorized in Issue631; it was not performed because the new trust test
reached the Issue's earlier STOP. No candidate PR or CI was launched.

## Preserved authority and first boundary

P630 evidence tree at `d5c3788db30b7b03ec00bdc238e58d8ce61c9818` is copied intact,
including original manifests, raw source, failed old tests and unapplied patch.
`freeze.json` protects those files, both old native scripts, immutable old
complex sources/manifests and two additional pre-edit probes. It records the
new source/oracles before fixed-main baseline and prototype replay. The two
postmerge main CI runs39224748355/39224748336 were verified completed SUCCESS,
attempt1 on exact fixed main; no reruns.

Revalidated seven P630 positives + both old complex sources: ordinary CHECK0,
C17 CLI4 / V1-BACKEND-UNSUPPORTED / zero C, as originally measured. Type/range
negatives remain CLI3 and two-local/nested remain CHECK0/backend4. Additional
same-value-local and duplicate-outer probes also CHECK0/backend4. `git apply
--check --unidiff-zero` succeeded. Controlled replay used the exact P630 patch;
main.c SHA256 remained35d7f190a59c23207b20ca4fe4b2003c1b84160e60df64d6da9e4786910c32e6.
The historical two CTests again failed at their old CLI4 expectations; fresh
log preserved in `evidence/replayed-old-oracle-conflict.log`.

## New source and untrusted copied-artifact test

`same_value_local.nl` is a normal original Pair(18,209), FALSE57<45, with both
arms `let staged=u8(83); staged`. Both return83, so expected original left83,
old18, sibling209. `duplicate_outer.nl` pre-registers a separate duplicate83
outer binding; this later control was not run after STOP.

`world_origin_probe.c` registers the actual source, forks ONLY its value-free
registry (asserted no values/bindings/places/scopes/domains/backings), destroys
the original AST/parser/source, then issues two real `main()` artifacts. They
share the exact retained immutable source plan. One remains protected original;
the other supplies test-owned shallow arm copies. No Value, permission, root,
Domain, source span or scalar is created by the test.

The only corrupted field is `.context` on those arm copies. Their source
nodes, source pointer, parent/node/index metadata stay in the correct arm slots.
Control1 redirects arm0 to arm1's genuine context; control2 exchanges BOTH
contexts while keeping the two worlds distinct. After each probe the real
owned pointers are restored. Byte fingerprints of protected entries, nodes,
contexts/values/places/bindings/scopes and original revalidation remain unchanged.

Observed on GCC14 and Clang23 frontend-only builds:

| Actual arm results | Single wrong context | Both contexts exchanged |
|---|---|---|
| 83 / 24 (P630 both_false source) | REJECT | REJECT |
| 83 / 83 (same_value_local source) | REJECT | **ACCEPT** |

Both have original branch_value_prefix9/9, branch_fact_prefix6/6, local symbol5/5,
tail ValueId11/11, same source name `staged`, distinct owned arm contexts. The
same-value test asserts equal actual source names and numerical IDs. No worlds
or node data are normalized to obtain the result. `origin-reproduction.json`
contains real exits/output; helper exit0 means successful diagnostic execution,
NOT that a rejection obligation passed. The accepted boolean is the finding.

## Interpretation and STOP

The replayed consumer checks fragment→parent/IF/slot membership, distinct world
pointers, source spelling, scalar/consumed/ended facts and original prefix.
However, equal-valued structurally matching contexts can be exchanged together;
the existing borrowed lineage and equality checks do not establish which actual
arm originally issued that context. The requested exact wrong-arm rejection
is therefore not established by this prototype.

This is a **private proof-consumer authenticity finding**, NOT a demonstrated
invalid NewLang source acceptance, false native field value, memory UB, new
Allocation/Domain authority or general soundness failure. For dependency-free
Copy-only equal states, observational equivalence may be a relevant design
question; P does NOT silently waive Issue631's exact provenance test on that
basis. Coordination must decide whether an existing sufficient source-issued
correspondence can be identified, whether this equal-state permutation is an
explicitly permissible proof equivalence, or whether an immutable arm↔world
origin record/issuer contract needs separately authorized design. No new issuer,
registry, source rule, lifetime policy or general CFG is added here.

Per Issue's trust STOP, local-name/span/forged ValueId/consumed/ended-place/
Change-leakage/duplicate-binding controls beyond this earliest finding were
**NOT RUN**, not declared N/A or PASS. New admission/native integration,
old-oracle migration, OOM/full-suite/ASan/UBSan and candidate FULL5/PARTIAL were
not completed. P630's seven prototype native/OOM results remain historical and
do not substitute for P631 acceptance. The standalone C probe running natively
is NOT NewLang-generated native evidence.

## Reproduction and preservation

Production was restored after preserving the exact prototype driver/probe hashes
in `prototype-metadata.json`. Old two integrations return2/2 PASS again; new
source is CHECK0/backend4/zero C at evidence HEAD. No expected value, immutable
manifest, old source or old Python script was edited. Initial harness setup
failure (reusing an already-advanced registry) and output-format calibration are
recorded in `harness-first-failure.txt`; corrected setup reuses the established
P517 value-free-registry fork method and requires both unmodified artifacts to
validate BEFORE any corruption.

In a disposable checkout of this evidence branch:

```
git apply --check --unidiff-zero tests/p630/evidence/prototype.diff.txt
git apply --unidiff-zero tests/p630/evidence/prototype.diff.txt
git apply --unidiff-zero tests/p631/evidence/reproducer-build.diff.txt
CC=gcc cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2 --target p631_world_origin_probe
./build/p631_world_origin_probe tests/p631/same_value_local.nl
./build/p631_world_origin_probe tests/p630/both_false.nl
```

Repeat in a separate Clang build for the second compiler result. The patch is
research replay only; it is not selected or mergeable. No other Track is launched.

Goal alignment: Compass→Strategy1.1/1.3/current Roadmap/Process4.3.1 still favors
safe, reusable ordinary CLI progress; this concrete trust-boundary question
changes the next step rather than resuming Product GO scoring. No distinct goal
deviation, library audit-locality benefit or H1 PASS is claimed. Historical gate
N/A for faithful existing Copy/IF/fixed-field consumption; existing semantics
KEEP, new issuer/CFG/owner/lifetime changes DEFER for C's explicit decision.
