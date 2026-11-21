# C1306 original independent source-law oracle v1

Status: PREP ONLY. No compiler, CHECK, public proof, C17, native, Lean, build, SDK, Actions, CI, Git object/ref, branch, PR, harness, observer, adapter, or target execution has been performed. All implementation observations below are DESIRED / NOT_RUN. Target is NOT_RELEASED; execution budget is 0. Future execution requires a separate release for GitHub-dedicated-branch plus Actions ONLY; a P-equivalent local execution environment is forbidden.

Actor: /root/r1595_nested_actual_prep. Own Issue: https://github.com/wakairo/NewLang_Compiler/issues/1595
Independent selection: 2 positive complete units; 3 controls (1 lawful coverage control and 2 true-invalid units). No worker/descendant was created.

## 1. Authority and reading order

Canonical repository commit: 3d2c0e05a206c905d5f5cfab12659d1b90b63456.
Tree identity supplied by own Issue: 77b12ba519cc7dfde1e28bc369bd4b6fa4b369a1. No Git tree/object/ref operation was used to verify or change it.
CURRENT_SPEC at the fixed commit was read and selects NewLang_v0_spec_Draft17_38.md.

Read order: own Issue latest individual GET; ACTUAL START/exposure POST then individual GET/body comparison; canonical directory filename indexes for navigation; Compass; Product Value Strategy sections 1.1 and 1.3 (the displayed section-1 context also exposed 1.2/1.4); Process section 7; CURRENT_SPEC; relevant Draft17.38 clauses; Process 4.3.1 to resolve the investment/alternative boundary.

Canonical references:
- Compass, navigation only: https://github.com/wakairo/NewLang_Compiler/blob/3d2c0e05a206c905d5f5cfab12659d1b90b63456/docs/NewLang_Project_Compass.md
- Value sections 1.1/1.3: https://github.com/wakairo/NewLang_Compiler/blob/3d2c0e05a206c905d5f5cfab12659d1b90b63456/docs/NewLang_Product_Value_Strategy.md
- Process sections 7 and 4.3.1: https://github.com/wakairo/NewLang_Compiler/blob/3d2c0e05a206c905d5f5cfab12659d1b90b63456/docs/NewLang_Project_Development_Process.md
- CURRENT_SPEC: https://github.com/wakairo/NewLang_Compiler/blob/3d2c0e05a206c905d5f5cfab12659d1b90b63456/docs/reference/CURRENT_SPEC.md
- Draft17.38, the source-law authority for all derivations below: https://github.com/wakairo/NewLang_Compiler/blob/3d2c0e05a206c905d5f5cfab12659d1b90b63456/docs/reference/NewLang_v0_spec_Draft17_38.md

Relevant source-law clauses:
- Sections 4.2, 4.4, 4.7, 4.9: integer values are Copy and Discardable, value-use copies an existing quantity binding without consuming it, and unit is a separate singleton type/value.
- Sections 5.1, 5.2, 5.4 and 6: usize is a target-sized nonnegative quantity scalar; typed integer literals are range-checked without wrap/saturation; division/remainder require a nonzero divisor. For unsigned quantity values, q=floor(a/b), a=q*b+r, and 0<=r<b.
- Sections 7.1 and 18.2: evaluate actual expressions completely in source order before entering the callee; a nested call's normal result is the outer call's actual value. The receiving formal is a fresh initialized binding.
- Section 7.4 and section 27.4: if is conditional, its bool condition is evaluated once, exactly one arm is selected, and normal arm results have exactly the same static type.
- Sections 8.1 and 5.2: unmet operation preconditions are not permission to wrap, synthesize zero, or execute UB through an ordinary checked operation.
- Section 13.5c: known-call reference semantics substitutes actual relations and applies the body semantics in strict source order. It does not require literal inlining or a particular summary representation.
- Sections 18.1 and 28.1/28.2: these top-level ordinary non-generic signatures are available before ordinary body name resolution, independent of declaration order within one semantic unit. There is no local function or overload here.
- Sections 18.5 and 19.1: a normal function body is an exactly-once lexical block; its unterminated tail expression is its result. Non-tail lets end with semicolons. Explicit unit tails in main provide no quantity result.
- Sections 27.1 and 27.2: initialized single-assignment ordinary lets and repeated Copy uses remain legal.

These clauses, not producer examples or implementation coverage, are the basis of eligibility and predictions. Current compiler support remains unknown.

## 2. Scope and mathematical conditions

Each artifact is one independent physical source input and one semantic unit, with a complete main() -> unit and the ordinary unary usize -> usize functions it calls. No declarations or state are shared between artifacts, even where their spellings coincide.

The call graphs are finite and acyclic:
- p01 and c01: main calls sift and quarter; neither helper calls another function.
- p02: main calls route; route calls no ordinary function. route(route(...)) is nested evaluation, not recursion or a call-graph SCC.
- c02: the same declared helper graph as p01, with an ill-typed inner actual.
- c03: main calls splitzero and sift; neither helper calls another function.

There are no general CFG/early-return/loop/SCC extensions, multiple-formal functions, runtime divisors, new range mechanisms, heap operations, pointers, loans, owner/world selection, authority issuers, or ABI choices.

Let U be the target's greatest representable usize value. The common package condition is U>=146. This suffices for all typed literals and positive/control intermediate values. More narrowly, p02 needs U>=65; c02 uses only literals <=11 but is invalid independently of range; c03 needs U>=146 just to isolate its division-by-zero defect from a literal-range failure.

For every representable x:
- quarter(x)=floor(x/4), with divisor 4>0 and 0<=quarter(x)<=x<=U.
- sift(x)=x mod 11, with divisor 11>0 and 0<=sift(x)<=10<=U.
- route(x)=x mod 5 when x<9; route(x)=floor(x/6) when x>=9.
  Divisors 5 and 6 are positive. In the first arm 0<=route(x)<=4; in the second 0<=route(x)<=x<=U.

All three helper definitions are mathematically defined for every representable usize formal under the relevant literal-range conditions. No call-site numerical fact is needed to excuse an otherwise unsafe helper body. No addition, subtraction, multiplication, implicit numeric conversion, or unchecked operation is used. The inequalities above are mathematical justifications for selected arithmetic; they do not request a new compiler range-analysis mechanism.

If an eventual target does not satisfy the applicable literal-range condition, record that target/range boundary. Do not substitute smaller literals after observing output and do not grade it as a nested-call defect.

## 3. p01: direct nested arithmetic and repeated Copy

Artifact: c1306_p01_nested_arithmetic.nl
Classification: source-law eligible positive, conditional on U>=146; current compiler acceptance NOT_RUN.

Source occurrence labels below are this oracle's labels only, not prescribed compiler IDs. Each row lists the inner call first and the dependent outer call second:

| Main binding | Inner occurrence: actual -> normal result | Outer occurrence: actual -> normal result | Binding value |
| --- | --- | --- | --- |
| first | p01.first.inner quarter: 146 -> 36 | p01.first.outer sift: 36 -> 3 | 3 |
| second | p01.second.inner sift: 146 -> 3 | p01.second.outer quarter: 3 -> 0 | 0 |
| repeat | p01.repeat.inner quarter: 146 -> 36 | p01.repeat.outer sift: 36 -> 3 | 3 |
| copied_round | p01.copied_round.inner sift: 3 -> 3 | p01.copied_round.outer quarter: 3 -> 0 | 0 |

Exact arithmetic:
- 146=4*36+2.
- 36=11*3+3.
- 146=11*13+3.
- 3=4*0+3.
- 3=11*0+3.

The logical evaluation sequence is:
1. Evaluate typed 146, enter the first quarter occurrence with fresh amount=146, complete its division/body result 36, pass that result as the first outer sift actual, enter fresh amount=36, return 3, initialize first=3.
2. Evaluate the second statement's typed 146, complete sift(146)=3, then quarter(3)=0, initialize second=0.
3. Evaluate a new quarter(146)=36 occurrence and a new sift(36)=3 occurrence, initialize repeat=3.
4. Copy first into copied=3. first remains Available and retains mathematical value 3.
5. Copy-use copied as the actual of a new sift(3)=3 occurrence, then pass that result to a new quarter(3)=0 occurrence, initialize copied_round=0.
6. Evaluate the explicit unit tail of main.

There are exactly eight logical ordinary helper invocation occurrences along main's normal source evaluation. first and repeat have equal values but distinct call occurrences. second and copied_round both equal zero but have different producing contexts. copied is a Copy of a scalar value; it does not merge either call occurrence or create runtime/owner/world authority.

The different compositions discriminate value substitution: sift(quarter(146))=3, whereas quarter(sift(146))=0. Treating the outer actual as the original literal, stale sibling result, a placeholder zero, or the wrong inner result can be detected by this oracle where it changes those values or the claimed source relation.

sift is declared before main and quarter after main. Both names must resolve according to ordinary unit visibility; textual position is not a source-law prohibition.

## 4. p02: nested normal-if results

Artifact: c1306_p02_nested_branch_result.nl
Classification: source-law eligible positive, conditional on U>=65; current compiler acceptance NOT_RUN.

Each route invocation evaluates its own amount<9 condition once, selects one lexical arm, returns that arm's usize result, and ends before the dependent outer invocation receives its actual.

| Main binding | Inner route: actual, condition, selected arm, result | Outer route: actual, condition, selected arm, result | Binding value |
| --- | --- | --- | --- |
| low | 17, false, 17/6, 2 | 2, true, 2%5, 2 | 2 |
| high | 65, false, 65/6, 10 | 10, false, 10/6, 1 | 1 |
| boundary | 9, false, 9/6, 1 | 1, true, 1%5, 1 | 1 |
| zero | 0, true, 0%5, 0 | 0, true, 0%5, 0 | 0 |

Exact arithmetic:
- 17=6*2+5; 2=5*0+2.
- 65=6*10+5; 10=6*1+4.
- 9=6*1+3; 1=5*0+1.
- 0=5*0+0.

The full source order is low.inner, low.outer, high.inner, high.outer, boundary.inner, boundary.outer, zero.inner, zero.outer, followed by main's unit tail. Each source invocation has a fresh formal binding. The same lexical route body is instantiated in eight call contexts; equal actual/result numbers do not erase those contexts.

The boundary input 9 takes the false/else arm because the predicate is strict <9. The outer route in high receives 10, not the original 65 and not the later result 1. The zero pair is a real defined zero result from literal zero and nonzero divisor 5. It must not be confused with the unconstrained formal of route at definition time or with an implementation's unknown placeholder.

Both arms produce usize by normal completion and all divisors remain statically nonzero. There is no hidden rejection requirement on the unused arm and no implicit extra Copy, synthetic unit, early return, or general CFG mechanism in this case.

route is declared after main. Definition placement does not make these calls unregistered under the source law.

## 5. Controls

### c01: ordinary intermediates are a lawful alternative

Artifact: c1306_c01_explicit_intermediates.nl
Classification: LAWFUL COVERAGE CONTROL, source-law eligible under U>=146. A rejection is not a mandatory oracle requirement.

The explicit intermediate sequence is:
quarter(146)=36 -> first_argument; sift(36)=3 -> first;
sift(146)=3 -> second_argument; quarter(3)=0 -> second;
quarter(146)=36 -> repeat_argument; sift(36)=3 -> repeat;
Copy first=3 -> copied;
sift(3)=3 -> copied_argument; quarter(3)=0 -> copied_round;
then unit.

The four final quantity values are (first,second,repeat,copied_round)=(3,0,3,0). All intermediate and reused lets are Copy/Discardable, with no reassignment. There are eight logical helper invocations, in the same mathematical dependency order as p01, but a separate semantic unit with separate source occurrences.

This control permits a future report to localize a coverage boundary: if ordinary intermediates are supported while the equivalent direct nested actual is not, that is a nesting/composition support distinction, not proof that the nested source is illegal. If both are unsupported, report the first actual boundary rather than claiming a runtime defect. Equal values or coincident public IDs across these units do not establish common identity.

### c02: unit passed to a usize formal

Artifact: c1306_c02_unit_actual_invalid.nl
Classification: TRUE INVALID, assuming relevant numeric literals are representable.

quarter requires a usize actual. Its supplied inner actual is unit, the distinct singleton type/value. There is no implicit unit-to-quantity rule, no quantity zero hidden in unit, and no valid inner usize normal result to forward to sift. The surrounding functions' signatures and arithmetic are otherwise ordinary.

Required semantic outcome: reject the ill-typed unit. It must not receive accepted CHECK/public evidence asserting this whole source is well formed. There is no lawful scalar value for bad, no legitimate outer call execution, and no positive C17/native result prediction.

The parser may recognize the call shape before a type rejection; diagnostic stage/code/span/spelling is observational and not fixed by this oracle. An earlier sound rejection does not prove that the intended type boundary was reached.

### c03: zero divisor inside the nested producer

Artifact: c1306_c03_zero_divisor_invalid.nl
Classification: TRUE INVALID, assuming U>=146.

splitzero's body uses amount/usize(0). The selected division precondition divisor!=0 is false for every formal amount, including the supplied 146. This is not an imprecise unknown divisor, an uncovered runtime-divisor feature, an overflow convention, or a totalized error value. The helper is invalid independently of the outer sift call.

Required semantic outcome: reject the unmet arithmetic precondition. It must not yield an accepted CHECK/public certificate for the entire unit or synthesize a successful inner quantity (especially zero) for the outer actual. There is no mathematical value for bad and no positive native expectation.

No diagnostic code/location or mandatory parser-versus-CHECK split is prescribed. If a future compiler unexpectedly certifies this source as valid, preserve the observation and STOP C before attempting unsafe native execution.

## 6. Knowledge, occurrence, and identity boundaries

A source quantity is mathematically exact for each listed concrete call. A compiler's abstract knowledge representation is a separate observation. The oracle does not force a public enum, integer field layout, serialized schema, value ID numbering, or particular proof strategy.

If available evidence claims Known(n), it must justify n for the correct source occurrence, actual/formal relation, and result. Unknown is not Known(0), even if an unknown payload storage field happens to contain zero. Conversely, an implementation may carry a sound symbolic/unknown summary for an arbitrary formal while preserving valid checked semantics; that alone is not a wrong-value finding.

A scalar equality, repeated source spelling, equal machine numbers, shared IDs, or matching unit exit status is not sufficient evidence of source/call/result/world identity. Source occurrence labels in this oracle are explanatory tags, not instructions to allocate a new runtime identity, world, owner, validator, or issuer.

The source evaluation order is fixed, but these functions are pure and have one formal each. Equal arithmetic outcomes alone do not empirically distinguish every legal optimization or establish a runtime trace of invocation order. Literal C calls, branch instructions, temporaries, stack frames, or execution counts are not required if an allowed transformation preserves source-observable semantics. The eight-occurrence counts are source-model counts, not required machine instrumentation.

## 7. Desired observations by layer

All rows are NOT_RUN in this PREP phase. These are evidence goals, not an interface/harness/adapter design or an assertion that the present compiler exposes the data.

| Layer | Desired future evidence | Limits |
| --- | --- | --- |
| SOURCE | Exact frozen complete input and per-case source-law classification; declarations resolved in the correct unit; actual nesting retained in provenance | Parsing alone does not establish semantic validity or implementation support of every legal source |
| CHECK | For p01/p02/c01, ordinary checked arithmetic/Copy/call/result semantics with satisfied preconditions; for c02/c03, fail closed at a justified boundary | Legal but unsupported or conservatively rejected input is not automatically a language-law defect; a fixed target and expected support must be released separately |
| Public source-issued evidence | Whatever genuine existing public evidence is available must remain tied to the exact input, callee declaration, actual/formal substitution, and result; scalar-knownness claims must be honest | Do not fabricate evidence, force an ID/schema, infer a new world/owner, or use private headers; missing observation is NOT_OBSERVED rather than invented PASS |
| C17 | If later reached under release, emitted accepted C17 preserves the defined positive/control computations and selected dependencies; no accepted translation may silently normalize the invalid controls into valid quantities | No exact C spelling, helper count, temporaries, ABI, ID, or optimization strategy is prescribed |
| Native | If a separately authorized observation route genuinely exposes quantities, compare each observed quantity and its provenance with the tables above | The original main returns unit and discards all quantity locals; no quantity I/O is present. A successful unit exit by itself establishes neither those runtime quantities nor evaluation/call/result/world identity |

No target/public-interface inspection, observer/harness construction, or source adaptation is part of this preparation. If later execution reaches only a unit exit, record quantity observation as NOT_OBSERVED; do not promote it to quantity-runtime PASS. Any later adapter and its source/hash must be separate and may not replace this original FIRST.

## 8. Independent exposure ledger

Classification: CANONICAL-FIRST with declared canonical-embedded metadata/snippet exposure; no producer-original calibration.

Observed before FIRST:
- Own Issue #1595 body and own ACTUAL START, plus the delegation's scope and prohibitions.
- Root and docs directory filename indexes at the fixed canonical commit. These included implementation/test/report directory or file names; their file bodies were not opened.
- Compass and Product Value Strategy text, including embedded references to historical Coordination, P/R/V work, named finite capabilities, historical IDs, and summary snippets. The displayed Value section-1 context included 1.2/1.4 in addition to the required 1.1/1.3.
- Process section 7's canonical operational rules and embedded historical FAST/CI examples/metadata, and section 4.3.1.
- CURRENT_SPEC's selected Draft pointer and its embedded historical M/P/R/PR summaries, capability counts and source-shaped names.
- Draft17.38 heading inventory (including historical topic/Issue headings), then only the relevant canonical type, numeric, evaluation, precondition, ordinary function/summary, block, binding/normal-if and unit/name/visibility clauses and their normative examples.

No linked historical Issue/PR/report/source/fixture/oracle/result/CI original was followed. No P/M/F/other-R original, candidate or implementation code/header, aggregate-C, shared scratch, worktree, actor inventory, or current CI result was read. No source values or names were copied from producer inputs. Choices and predictions in this package were formed before implementation exposure and before all execution.

## 9. Goal alignment and stopping rule

The useful question is whether ordinary expression composition can carry a genuine returned quantity into another ordinary call while preserving checked semantics and honest evidence. This can support reusable compiler capability. A plain intermediate-let form is a legitimate, low-cost alternative; the lawful control records it.

Dedicated fixture profiles, duplicated worlds/validators, or hidden author/caller audit work would be separate cost/value concerns. They are not invented semantic failures of these pure quantity programs. No audit-locality improvement, safe owner API, general soundness, or Product GO is demonstrated by this preparation or by a future finite numerical PASS. ProductGO PAUSED; H1 HOLD; author-plus-ordinary-safe-caller audit-locality UNMEASURED.

Source-law questions for C after these readings: NONE at preparation time. Unknowns are current support and available honest observation at a not-yet-released target, not guessed language law. If C identifies a real canonical/source-eligibility ambiguity, ask only the minimum source-law question and retain this original.

Freeze this exact source/oracle package in FIRST, POST then individually GET it, re-extract all artifact text, and compare UTF-8 byte lengths and SHA-256. Post PREP FINAL, individually GET it, then STOP C.

No current execution, target, branch, FAST/FULL budget, public-interface implementation planning, source correction after FIRST, or verification equipment is authorized by this package. A later release and prompt must both name the GitHub-dedicated-branch plus Actions ONLY mode and their finite scope/budgets. P-equivalent local execution remains forbidden.
