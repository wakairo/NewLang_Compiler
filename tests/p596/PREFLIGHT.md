# P596 fixed-main gate and pre-edit reuse decision

Work started 2026-10-18 18:50 JST (09:50 UTC), 90–150-minute ceiling.
Compiler/main fetched and fixed at fdae44e62e39bc521fc4bcab70909d4d7e1d6155;
canonical CURRENT_SPEC → Draft17.32. Actual post-merge push workflows:
FULL5 39116344227 and PARTIAL 39116344177, both attempt1 SUCCESS.
Local unchanged-main frontend build and P589 source/public/native/OOM + P584
exact-u8 guard: 3/3 PASS (baseline-focused.log). No integration regression.

Original 30 inputs/oracles frozen at frozen.json timestamp, before production
edits; SHA256 and complete baseline SOURCE/CLI diagnostics retained. Subtraction
first stops at SOURCE: typed-literal forms P5-EXPECTED-ITEM-END; name forms
P5-EXPRESSION-UNSUPPORTED. This is NOT semantic underflow proof. Old +/== and
function -> smoke SOURCE/CHECK succeed; oversized literal is already semantic RANGE.
Calibration: the frozen arrow-control native0 prediction was too broad: the
unchanged baseline already returns C17 CLI4/V1-BACKEND-UNSUPPORTED. Only its
SOURCE/CHECK0 is a success; preserve and compare this exact old backend limit.
Historical P589 files and public plus header hashes captured separately.

Authority/history: Compass → Product Strategy §§1.1/1.3, Process §§4.2/4.3.1/7,
Design Decision Procedure, Ledger and canonical §§5.2/6/7/13.5a. Draft17.4
already selects mathematical usize subtraction: KEEP type/range/Copy/LTR and
no dependency laundering. M593/C comment6248105142 selects precisely one flat
atom-atom (existing names or typed u8/usize literals), excluding all precedence,
unary/nested/mixed syntax, ptr/field/call/loan operands and uintptr subtraction
SOURCE. R592/C comment6248370657 adopted plus with explicitly limited residual
risk; no new R safety finding was reported. R's foreign registered identity and
fail-at-N gaps remain independently untested, not promoted to PASS by P work.
No new semantic/source decision or Ledger edit is needed for this implementation.

## Chosen route B: small PRIVATE tagged +/− core

A would duplicate most of the 270-line validator. Reuse the existing one list,
issuer, destructor and source/binding/ordered-operand validator instead. A tiny
private event has an explicit ADD/SUBTRACT tag and neutrally named result number.
Legacy NLUsizeAdditionView and signatures/layout remain byte-for-byte unchanged,
.sum means SUM ONLY. Plus view/validator must reject minus records even for NULL
candidate. A separate subtraction view exposes .difference; public wrappers
explicitly convert metadata to/from the private core. No casts/type-punning,
second proof list/world clone or generic operator framework.

Math stays operation-specific: prove a>=b BEFORE a-b, as plus proves
b<=SIZE_MAX-a BEFORE a+b. Authenticate the exact source/AST/checked kind and
operator token corresponding to the immutable tag. Emit original C carriers.
Only pure typed-literal/name atoms qualify, preserving observable LTR evaluation.

Forecast incremental production code ~180–240 lines including the new public
header/wrappers. One tagged receipt expected 264 bytes vs256 on this 64-bit host;
existing fragment storage and 216-byte plus view unchanged. Same bounded
receipt/registry/source/world scans, no new allocations during validation.
Stop before a duplicate full validator, public plus reinterpretation/ABI break,
general arithmetic/CFG/range proof, unsafe source admission, or budget overrun.

Goal: reusable checked distance/capacity quantity groundwork, not Product/H1 GO.
Alternatives: thin specialization if validation reusable; chosen tiny shared
core; no-code HOLD if too expensive. Unknown dynamic ranges deliberately fail
precision. Independent review of the shared new trust boundary is C's decision;
P will not start R or another Track. No goal deviation identified within scope.

During validation, live main advanced to262f4bddb9055a6cf9067a639cbe44fa72b703a4
through Docs595 only (C Replacement Coverage Map and Development Roadmap).
The implementation branch remains based on fixed fdae44e; no rebase or code
change was taken from that docs commit. PR Actions may check the merge tree
including those two docs; report its exact relationship to the frozen head.
