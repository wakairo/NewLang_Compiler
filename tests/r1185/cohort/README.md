# R1185 V2: pre-execution scope calibration

**pre-execution frozen independent V1 / scope-calibrated V2; C-supplied finite-profile boundaries**

Original V1 remains byte-identical and NOT_RUN. All independent numeric values, semantic traces and dangerous questions are retained. The V2 differences are fixed before any compiler/AST/native run and before reading candidate public API/CI context. `calibration.json` includes every V1-to-V2 source diff and each removed alias-specific claim; `oracle.json` retains all numerical trace predictions. No result is known at this freeze.

Fixed compiler: `d7436585b74d0d6564df7cdf77da280903fb5234`; tree `38070e6b209168c251ee427cc2a716aebc932fb8`. Authority: adopted main `63658cd546be8b99599baf19476486a475adb320`, CURRENT_SPEC-selected Draft17.34. Execution release: https://github.com/wakairo/NewLang_Compiler/issues/1185#issuecomment-6654354882

## Scope calibration

Every loop initializer is the same independent usize literal. Caller helper actuals use the loop phase directly, except the intentional N03 invalid u8 actual and the N08 shifted second-call attack. Helper LT/EQ conditions read their fresh formal directly. Caller u8 result Copy and callee u8 normal-result binding/Copy remain. No explicit RETURN was introduced. Canonical meanings and all independent predictions remain the same.

The removed outer-to-loop initializer Copy, caller usize alias composition and formal-condition usize alias composition are NOT tested by V2. P06 retains outer phase=2, fresh loop phase and fresh helper formal with the same spelling; its literal initializer does not establish the V1 initializer-Copy path. C01 uses the same actual/formal=5 and result=162 but removes its usize actual/formal aliases. N04/N05 escape operations, N06 nested IF and N08 second/shifted call are preserved.

## Finite matrix

All numeric sequences below are predictions from static reading and manual arithmetic, not observations. `old` means the immediately preceding current selected-child value returned by `replace`. The full case-by-case trace, fields, canonical clauses, and post-state are in `oracle.json`.

| Case | Selected index | Helper predicate | Header phases | Helper results / store inputs | Old results | Sibling | Actual Break |
| --- | ---: | --- | --- | --- | --- | ---: | ---: |
| P01 | 0 | tick < 4 | 5 | none | none | 174 | 5 |
| P02 | 1 | tick == 1 | 1, 3 | 67 | 42 | 155 | 3 |
| P03 | 0 | tick < 3 | 0, 2, 4, 6 | 109, 109, 231 | 14, 109, 109 | 208 | 6 |
| P04 | 1 | tick == 5 | 1, 3, 5, 7, 9 | 187, 187, 76, 187 | 33, 187, 187, 76 | 120 | 9 |
| P05 | 0 | tick < 2 | 3, 4 | 198 | 198 | 49 | 4 |
| P06 | 1 | phase == 5 | 2, 5, 8, 11 | 18, 244, 18 | 77, 18, 244 | 221 | 11 |

P01 has zero helper calls, stores, or Continue edges. P02 and P05 each have one; P03/P06 have three; P04 has four. The last header is the false outer-guard / Break iteration and has no helper call or store. P03's second store and P05's only store preserve the same numerical bits; they still exercise replacement, old-value forwarding and the real selected destination. P04 crosses false→true→false. P06 separately distinguishes outer `phase=2`, fresh loop `phase`, and fresh helper formal `phase`: the post-loop outer phase copy must remain 2 while the real loop result is 11.

P02/P04/P06 put the helper after main, exercising canonical ordinary-function visibility. P02/P03/P06 reverse named-initializer source order; names must still resolve to declared field identities. Index 0 and index 1 updates are covered equally. Caller u8 result Copy aliases are explicit; P02/P04/P06 also copy the helper normal u8 result. Caller usize aliases and helper formal-condition aliases have been removed and are not tested. All arithmetic operands/results are small nonnegative usize values with positive constant steps.

C01 is one ordinary helper control: actual/formal 5, condition false, normal/copy result 162, caller phase still 5. It has no T, store, or loop, and cannot establish helper-to-loop connectivity.

## Negative and coverage classification

| Case | Canonical classification | Intended first semantic boundary / predicted coverage gate |
| --- | --- | --- |
| N01 | Invalid | usize IF condition must be rejected as non-bool |
| N02 | Invalid | normal u8 and usize IF arms cannot join |
| N03 | Invalid | actual u8 cannot satisfy formal usize |
| N04 | Invalid | loan-generated write ref escapes its ending field-loan scope through an alias |
| N05 | Invalid; deliberately outside scalar-helper slice | ordinary ref function result cannot launder a scope dependency; if not rejected at definition, call/loan exit remains invalid |
| N06 | Legal, outside one-IF helper slice | nested second IF may be refused by finite body-analysis / owned / C17 eligibility; canonical runtime values are frozen as 41,93,157 |
| N07 | Invalid | joined normal usize result violates explicit function result u8 |
| N08 | Legal, outside single phase-to-formal flow | two helper call sites and transformed second actual may be refused at finite composition eligibility; second result feeds the sole store |

N06 and N08 are coverage probes, not unsafe source. Their exact implementation rejection stage cannot be known without inspecting the candidate. The prediction is conservative refusal at the first unsupported finite body/call representation gate; CHECK may legitimately accept, and any later unsupported owned/C17 path must fail closed. Faithful acceptance is not itself a safety finding and goes to C for coverage calibration. The full hypothetical canonical trace is fixed in the oracle so an implementation cannot substitute a different legal meaning.

For each invalid negative, an earlier parser/profile/analysis-precision failure is recorded as the actual first barrier with **target NOT_REACHED**. It does not prove the deeper type, dependency or scope rule. No diagnostic code is guessed. Invalid CHECK=0, invented authority, partial emission after failure, or unsafe lowering triggers affected STOP→C; source/oracle must not be changed to make that result pass.

## What values and identities mean

`R` is the original T root in main, `L` its selected declared fixed child, and `S` its sibling. R/L/S incarnations remain the same across replacement and backedges; only L's current package and the compositionally containing root current value change. S's value/current fact is preserved. The returned old u8 is an ordinary dependency-free Copy and remains distinct from the subsequently installed current package. Every helper actual is the corresponding caller phase, received in a fresh callee formal, and the true normal result must pass through caller binding/Copy into the single scoped replace. Value equality and equal source spelling do not establish binding, root, owner, or field identity.

Within each observed run, the selected and sibling addresses must identify distinct child places and remain attached to the same original root; every real store destination must equal the selected-child address and must not equal the sibling address. No absolute address, field offset, cross-run address equality, or inequality between root address and its first child is predicted. Fresh semantic formals can reuse native stack addresses between calls. A scalar result carries no original root, Allocation, Domain or owner authority.

Same-bits cases require evidence of the replacement path and actual store where observable. Equality of pre/post values alone cannot show that a store or real helper result was used. A compiler's legitimate optimization may remove native observability; such a missing event is **NOT_OBSERVABLE/NOT_RUN for that observation**, not fabricated evidence and not, by itself, a language-semantic defect.

## Execution and evidence contract

C already authorizes this V2 freeze -> server readback -> new R equipment freeze -> R-FAST -> ordinary Actions measurements without an additional GO between stages. The sole R branch is `r-fast/r1185-helper-result-current`. The first ref must point to a complete R commit whose sole parent is fixed P; no empty P-head ref. FAST maximum 6 actual jobs (including branch creation); natural ordinary CI maximum 12 actual jobs; manual rerun/dispatch 0; DRAFT/NEVERMERGE; stop additional push at first trustworthy all-green. All executions are on GitHub Actions; no P-equivalent local environment.

Use ordinary CLI CHECK, authentic source-issued public owned facts where applicable, ordinary C17, strict GCC/Clang O0/O2 plain and passive native, and separately accounted guest ASan/UBSan modes. The public whole-loop validator is explicitly NOT_RUN/NO_APPLICABLE_PUBLIC_API under C release. No private artifact/authority fabrication. Do not send invalid negatives to native execution.

The new observer receives only actual emitted C and its authentic compiler-produced AST. It must identify actual declarations, control, formal/actual/result relations, old load, store destination/incoming/current/sibling and continuation/break boundaries from those artifacts. It receives no guest source or oracle and must not invent guest calculations, calls, stores, control or replacement main. Only after raw observation is captured does a separate comparison use the frozen oracle. Semantic inlining does not prove a machine CALL; native address reuse does not deny semantic formal freshness; missing optimized events remain NOT_OBSERVABLE. Plain native success alone is insufficient hidden-value evidence.

R-owned formatting, registration, argv, logger or recognizer repairs may proceed within budget while preserving FIRST/RED and every old input/equipment version. Any semantic/owned/native discrepancy not explained solely by R equipment, authority/owner/issuer doubt, substantive source/oracle change, isolation/evidence loss or budget overrun is affected STOP to C. No output-driven oracle revision.

Legal ordinary-Copy composition outside this finite profile remains visible coverage debt in V1. Product audit locality remains UNMEASURED; GO624 PAUSED; H1#268 HOLD; P1036-PR1050 STOP.

## Recovery

MANIFEST.sha256 is SHA256 TAB decimal-bytes TAB relative-path, covering every other payload file. The Issue freeze includes the full gzip tar as base64, its byte count and SHA256. No compiler execution is required to restore these bytes.
