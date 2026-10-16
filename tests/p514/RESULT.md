# P514 — finite pure Copy choice → original Pair write candidate

This is compiler development / limited source-owned backend verification. DRAFT / DO NOT MERGE. Coordination owns adoption and any independent R; no Product GO, general effect join, H1/Array or ownership claim.

## Authority and reproduction

Fixed main `9e6389388a44950d17e19bb134cbfde37ac75f97`, CURRENT_SPEC → canonical Draft17.30. PR511 was merged as `e4af76784a6fe4174730af0671eb58bf606aac91`; PR510 experimental structural Pair join was NOT imported. Sources and runtime/negative oracle were committed before any compiler edit/run (`b4158a9`, full source hashes in `frozen.json`). TRUE/FALSE bytes differ only in b's 2/3 digit. No original source/oracle, checker, issuer, public API, toolchain, workflow, Canonical, Strategy, Process or Ledger changed.

```
cmake -S . -B build-fe -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-fe --parallel 4
ctest --test-dir build-fe --output-on-failure
python3 tests/p514/native_test.py build-fe/newlangc build-fe/p514_public_probe \
  build-fe/p514_gate_test gcc tests/p514 none
# Repeat last command with pinned clang; sanitizer CI passes address/undefined
# and instruments the generated guest C as well as the checker/driver tests.
```

Fixed-main baseline: configure/build the fixed main, run its newlangc --check and ordinary newlangc on the frozen files. `baseline.json` records binary SHA, exact CLI exits/diagnostics; `baseline-owned.log` records public actual-source observations. No modified compiler was used for baseline. The freeze commit changes tests only, production tree equals the fixed main.

## Measured frontier and implementation

All four positive programs already PARSE/CHECK0 and have source-issued owned IF/Pair/loan/replace evidence. Ordinary C17 stopped at CLI4 `V1-BACKEND-UNSUPPORTED`, stdout0. CLI gives only path, not a backend source span; retained checked IF21 span119–154 is body-relative. The original scalar-only `copy_if_selection` rejects the completed scope count1 / existing aggregate10, before scalar-only full-frame/prefix checks. This is backend admission precision, not syntax or semantic failure. Prediction confirmed.

Only `src/main.c` changes production behavior (~309 added/9 removed lines); CMake adds14 test wiring lines. One read-only finite private consumer, no new emitter rendering or semantic rule:

- Existing parent-qualified IF lineage, bool comparison/Copy origin, two connected pure arm tails and immutable source spelling checks remain required. The prior scalar-only path remains guarded separately.
- Exactly one original first lexical Pair, two fixed u8 fields, no domain/region/occurrence/active entry scope; only one post-IF implicit-local scoped write and two later original field reads. Calls, arbitrary effects/CFG, another Pair/write/IF, owner/ptr/Array are outside the consumer.
- Both actual arm worlds retain the same source-issued entry prefix, including complete parent and child packages, incarnations, ValueFacts and bindings. It does not pick one mutated arm as a public state. Byte comparisons are deliberately conservative for unchanged cloned metadata, not a semantic equality claim across fresh IDs. Pointer-containing binding entries are not byte-compared; only public binding views are compared.
- Declaration/field literal IDs and source bytes anchor the original entry; source Copy chains lead to THIS unknown IF result. Reference package matches actual child/incarnation/scope/provenance/write access. The source-issued loan/replace/reads agree on original root/child and fresh parent/left facts; right package/fact is stable. The retained recomposed root contains the actual new unknown package and original right.
- Fresh fact ordering is checked against the retained entry counter and actual intervening Copy bindings/ref binder. Arm-private fresh IDs never supply parent authority.
- Final function cleanup ends locals. Final root current_fact0 is not claimed live; post-write facts come from point-in-time checked evidence, and final ended recomposed package is used only for identity consistency.

This adds backend trust for composition: independent R before adoption is appropriate. Public accessors observe genuine checked-owned facts, but there is **no new public general Pair/IF certificate/validator**. The existing internal semantic invariant validator and private consumer predicates run read-only. This is not P506 Stage2 or a public structural Pair-join implementation.

## Owned trace and physical native result

`public_probe.c` uses only public loading/checking/accessors, destroying initial AST/source before observations. No host value/root/ref/permission seed. Both arms have values7/8, while joined and write-input u8 stay statically unknown, dependency-free, authority0. Actual condition bool is1/0 from original usize operands.

- Exact source: original Pair place1/inc1; left2/inc2; right3/inc3. At IF both arms rootFact1/left2/right3. Later write changes root1→10, left2→9; rightFact3 and original right package2 remain. Loan scope1 ended, old original left package1 is0. Recomposition uses new unknown package13 and same right2.
- Renamed/Copy source: same root/child incarnations and entry root/left/right facts; write root1→14, left2→13; right3 remains. Source conditional aliases/arm locals and copied selected carrier are real C locals.
- All four ordinary C outputs are saved byte-for-byte as `evidence/*.c17.txt`, hashes in `candidate.json`. Original physical `nl_local_1` is declared before comparison and IF. C evaluates `(nl_local_a == nl_local_b)`, then real `? 7 : 8` (or original arm locals), takes `&nl_local_1.f0`, captures `*ref`, writes the selected source carrier exactly once, forwards the actual old value, reads original f0/f1. No replacement Pair or host-assigned guest result.
- GCC14.2 and Clang23.1.2 local strict C17 `-Wall -Wextra -Wpedantic -Werror`, O0/O2: total32 plain/observed native runs exit0. Passive observations: TRUE bool1/selected7/old0/left7/right9; FALSE bool0/selected8/old0/left8/right9. Compiler CI uses repository-pinned Clang23.1.3 unchanged.
- The same observer rejects120 corrupted generated-C executions (exit91): swapped arms/direction, wrong selected/Copy, old, sibling, field, distinct original carrier. A different C Pair exists only in an explicitly corrupted-output control, never in accepted guest evidence. Observer adds reads/exit only; no original guest assignment is added.

## Negative and transactional evidence

Frozen negative first-frontiers remain distinct:

| Case | First measured boundary |
| --- | --- |
| wrong_condition | parser0, registration semantic CLI3 P13-IF-CONDITION, 7:22–23 |
| invalid_else even known TRUE | parser0, registration semantic CLI3 P3-WRITE-REF-REQUIRED, 7:54–59 |
| ref_escape original candidate | **parser source-profile** CLI3 unsupported LOCAL-LOAN-PROFILE, 7:42–43; semantic checker NOT ENTERED, no scope-escape proof claimed |
| branch_effect | parser0, registration precision CLI3 P6-JOIN-PRECISION, 7:18–113 |
| original P506 HOLD control | parser0, registration precision CLI3 P6-JOIN-PRECISION, 7:5–8:70 |
| wrong_field / disjoint_root / second_write | CHECK0, C174 V1-BACKEND-UNSUPPORTED: otherwise-valid broader programs, NOT safety violations |

`gate_test reject` observes unchanged public snapshots and no artifact at the first semantic/precision failure; parser-profile case leaves semantic state unentered. Separately unchanged `tests/p508/ref_escape.nl` reaches semantic P8-EXIT-DEPENDENCY and rollback (`evidence/supported-escape.log`), so an actual semantic escape refusal is retained without rewriting our frozen unsupported candidate.

Private test-owned artifact controls:144 refusals per exact source,142 per renamed source =572 program/mutation cases across4. Missing/duplicate/swapped/foreign arms including SAME retained source from a different real checked call, source/condition/result/Copy corruption, original root/field/incarnation/old/new facts including correlated post-fact changes, right currentness, closed scope, forged ref place/scope/access/provenance/Domain/Allocation reject. Every reject restores then revalidates original and fingerprints all relevant worlds/arrays unchanged. Public semantic snapshots also remain unchanged during valid consumption; gate allocations0 even with first wrapped allocation set to fail. No production test hook added.

OOM sweep: registration243 allocations, actual main138 allocations before first success; each OOM status OUT_OF_MEMORY, unchanged public state and no output artifact, followed by retry. Existing source/checker/validator/backend suite300/300 passed locally (33.22s); final focused P508/P5146/6 (5.13s before additional test-only correlated controls; latest log records final run). Local harness construction errors (diagnostic type, parser-vs-registration phase) were corrected before publication; no frozen source was changed or invalid/stale build used as success.

One natural full5+partial CI pair will be created only at the submitted final HEAD; terminal results/checkouts are recorded in PR/Issue after completion, not predicted by this commit. No rerun shopping or second candidate push.

## Goal/value, cost and remaining stop

The exact family is334bytes/11lines/8 ordinary let bindings plus1 ref binder,1 scoped write,1 replace,2 field reads. Renamed/Copy family444bytes/13lines/12 lets plus1 ref binder. No heap allocation, Holder or nonCopy responsibility is introduced. Still explicit: original receiver/projection/currentness, matching scoped write, old-value handling, nonescape and sibling preservation. This finite compiler path checks those obligations; it does not establish a hidden safe-library boundary or general caller effects.

Technical Exploration result: previously separate pure Copy IF and original field write now compose using existing source-owned evidence and the existing physical carrier. Cost forecast <=300–400 new production LOC; actual~309 additions plus14 CMake, no checker/API/issuer edits. The exact private consumer is bounded and not a replacement for arbitrary Pair memory-state phi. The first residual frontier is any branch-local mutation/general root/effect join (unchanged P6 HOLD), second write/disjoint original roots (finite backend rejection), or generic named API/Array/owner.

**Goal alignment for Coordination:** same topology and real conditional original-object update demonstrated; author+ordinary-safe caller audit/re-audit dependency closure improvement remains UNMEASURED. Equivalent hoisted pure choice is also available to well-factored C, Zig and same-topology Rust; no superiority is inferred. Alternatives remain direct existing scoped API, faithful hoisting under pure/no-intermediate-observation assumptions, or defer complex effects/Array/Pair Stage2. Local unchecked is an alternative only where authentic source authority/true prerequisites exist, not a workaround implemented here. Additional source-specific proof investment requires evidence of reusable capability/value and independent R as Coordination decides; Product GO/H1/STB/Array gates unchanged. Stop after one terminal Issue report.
