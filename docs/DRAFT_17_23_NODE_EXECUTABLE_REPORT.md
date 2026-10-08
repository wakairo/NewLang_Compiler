# Draft 17.23 lexical two-root native gate — Issue #166

Track: P

Base main: `86927422c1f1d14eabc1c012ca08bf2eb12195a0`.
Canonical: CURRENT_SPEC → Draft 17.23. Branch:
`draft-17-23-node-executable-topology`. Prerequisites: accepted/merged #159 / #160;
Track V #163 GO and Coordination GO. Preflight:
<https://github.com/wakairo/NewLang_Compiler/issues/166#issuecomment-6051124994>.
Exact PR head / PR URL / exact-head CI run are recorded in the final Issue/PR
handoff, avoiding a self-referential report commit SHA.

Historical design audit: N/A — faithful Draft17.23 implementation.
Process, Review Guidelines, Testing Strategy, Design Decision Procedure,
Ledger DI-001/002/007 and product boundaries were checked. Semantic delta = 0.
No source grammar, canonical text, oracle classification or corpus change.

## Implementation / observable result

`src/checked_c_node.c` is a separate bounded module, selected only after the
existing Checked-C route does not support the checked entry. It produces real
lexical C objects, genuine pointer/ref carriers, Option tag/payload copies,
ordinary link replacement with retained old value, runtime match and scoped
read reacquisition. It never reparses source or performs runtime safety checking.
Existing u8/Pair/V0/V1/AVS/local-root lowering remains unchanged.

The checker additionally publishes its **existing** unchanged-public-frame join
proof/prefix and actual Some pattern binder into owned checked evidence. A
read-only registry getter exposes the already-committed completed recursive
shape and pointer target. These changes widen no semantic acceptance.

[The contract](DRAFT_17_23_NODE_EXECUTABLE_CONTRACT.md) records one-to-one rule,
artifact, generated operation and native assertion, ownership and exact limits.

| Actual source test | Native observations |
|---|---|
| unchanged `node_link_semantic.nl` | tail_ptr = emitted tail address; actual head Some(tail); independent copied observed; executed Some q/reloan = tail; final head None; old_none None / old_some Some(tail); siblings 1/2 |
| Cell/link/data, payloads 19/42 | same identity/tag/copy/reloan/old-package assertions derived from checked metadata, no hardcoded nominal/field names or scalar constants |
| extra Some→Some replacement | returned old Some retains pointer, new Some and later None transition observed |
| reversed Some/None source arm order | same runtime Some branch and reloan, no source-order branch substitution |

The separate probe supplies checked-derived expected events; the separate native
observer only reads real emitted carrier addresses. Ordinary inert-hook output
also compiles/runs. Ten generated-code perturbations must fail native assertions.
An exit 0 without all events/assertions is rejected. No host-created graph,
seeded pointer, fixed-value substitute, skipped operation or runtime safety
substitute is counted as positive evidence. This is **not North Star PASS**.

## Rejection / rollback evidence

All earlier Node source negative controls are preserved: wrong/missing field,
ptr/ref/general/nested base, whole-root write, wrong type, dot/separator mistakes,
loan escape and stale pointer. They reject with empty compiler stdout. One-root,
three-root, whole Node Copy and nonempty-loan language-valid fixtures are explicit
backend unsupported with empty stdout; they are not language-invalid.

Backend unit tests corrupt completion/provenance/join/binder/loan/field metadata,
exercise OOM and a traversal resource fence, and verify unchanged outputs/state
and successful retry. Existing Node semantic failure/OOM/stale-incarnation/
dependency suites remain intact. No partial C is materialized on rejection.

## Validation / reproducibility

Locked bootstrap verified the unchanged exact packages and pins:
Debian 13 x86_64; GCC 14.2.0; Clang/LLVM/clang-format 23.1.2;
CMake 3.31.6; Python 3.12.14. CI retains Ubuntu 24.04 / GCC 13 and locked
Clang 23.1.2. No lock, provenance, workflow or oracle changes.

Run `python3 scripts/bootstrap.py`, `. .deps/activate.sh`, then existing README
CMake build/CTest commands for GCC Debug, GCC Release/NDEBUG, Clang Debug,
Clang ASan (`-DNEWLANG_SANITIZER=address`) and UBSan (`undefined`).
`bash scripts/check-format.sh` remains required. Native topology integration uses
strict C17 warnings/errors, `-O2 -DNDEBUG`, and matching sanitizer instrumentation
in the ASan/UBSan configurations. The full regression suite includes
`oracle.adapter`, `oracle.smoke`, `artifacts.integrity`, V0/V1/AVS/Pair/local-root,
function/control/sum/match and Node semantic tests.

Local full CTest passed **186/186** in each of GCC Debug, GCC Release/NDEBUG,
Clang Debug, ASan and UBSan; formatter and locked bootstrap also passed.
Native assertion failures were observed for all ten perturbed-code controls.
The exact-head PR CI run/SHA is finalized in the Issue/PR handoff after validation.
Review-ready is claimed only after all required jobs succeed for that head.

## Findings / stop

No new spec hole or ambiguity. COMPILER-IMPLEMENTATION-LIMIT: exact closed
backend profile and explicit carrier/step/depth/output budgets. Richer valid
programs remain backend unsupported; no semantic rules are invented to admit them.
The negative typed-literal deferred surface, frozen oracle/corpus and all other
tracks remain untouched. Allocation/lifecycle, detach/free, cJSON and general
recursive backend remain unauthorized.

PR must stay OPEN/unmerged for independent Coordination ACCEPT/BLOCK.
Do not close Issue #166 or start another slice.
