# R1608 C1307 source-FIRST oracle, version 1

## Authority and status

Authority is fixed main 6287fcb4cb94df3da1c67f57d7d717b29c25ecf9, supplied tree ef3a5906c7c3b015073d363c965ac6edb1898c58. The fetched CURRENT_SPEC selects NewLang_v0_spec_Draft17_38.md. This anchor fixes source law only. Exact execution target, public-interface access, transport, branch/write paths and execution budgets are NOT_RELEASED.

Canonical references, all at that fixed commit:
- Compass: docs/NewLang_Project_Compass.md (navigation only).
- Product/Value: docs/NewLang_Product_Value_Strategy.md, sections 1, 1.1, 1.3 and 2-6.
- Independence: docs/NewLang_Project_Development_Process.md, section 7.
- Source authority: docs/reference/CURRENT_SPEC.md -> docs/reference/NewLang_v0_spec_Draft17_38.md.
- Draft source laws used: sections 1-2, 4.2-4.4, 4.7, 4.9, 5.1-5.4, 6, 7.1/7.4, 8.1, 13.5c, 18.1/18.2/18.5, 19.1, 27.1-27.4 and 28.1-28.2.

This is a canonical source/math judgment and frozen observation contract, not a compiler result. All SOURCE parser, semantic CHECK, public owned/proof, backend/C17, native, sanitizer and whole-call empirical observations are NOT_RUN / NOT_OBSERVED. No target acceptance is predicted as a measured fact. Lawful-source refusal by a finite compiler must be preserved and classified at its actual first boundary; precision/backend refusal is not a new source prohibition.

## Bytes and finite scope

Exactly two positive complete source units and four complete controls are included. Each has one physical source text representing one semantic unit. Exact payloads are ASCII, hence UTF-8; no BOM; LF line endings; exactly one final LF; four-space indentation where shown. Names and bytes are frozen before any implementation/API exposure.

Target range envelope: P1 requires usize to represent 0, 1 and 7. P2 and C1-C4 require usize to represent every literal they contain, at most 53 (including 0, 1, 2 and 7 as applicable). No target width, ABI or implementation target is assumed. Any future release must verify this literal-range requirement before treating a case as applicable. A target outside this envelope requires C adjudication, not silently changed source or a universal illegality claim. Every positive arithmetic divisor is the nonzero typed literal usize(7).

These units use finite ordinary normal IF arms, non-generic main/helper, ordinary let and Copy uses. No nested/general CFG, terminating arm, recursion/SCC, fresh-call numerator, named/runtime divisor, new range-inference mechanism, heap, owner, loan, assertion, I/O or ABI is introduced.

## Common source-law oracle

1. Integer and bool values are Copy and Discardable. A value-use of seed, copied, input, selected, value, echoed, result_copy or a bool binding copies its value and leaves the source binding Available. A fresh parameter is an initialized local binding. Later uses of the same Copy source are lawful. The expression statements discard only Discardable scalar results.
2. Typed usize literals are mathematical nonnegative values range-checked for the selected target; they do not wrap. usize equality produces bool. Thus the written IF condition in P1/C1/C3 is true and in C2 it is false. The flag bound in main of P2/C4 is false, so the actual helper IF condition is false.
3. For any representable unsigned quantity n and d=7, q=floor(n/7), n=7*q+r, 0<=r<7. Both q and r are nonnegative and no greater than n, so both fit usize whenever n does. The sole division/remainder precondition here is d!=0, proved by the literal 7 without a fact about n.
4. A helper parameter n need not have a compiler-known numeric value for n/usize(7) and n%usize(7) to be lawful. Definition-time checking must not require a positive numerator or caller-supplied hidden proof. Concrete arithmetic equality and the compiler's available knowledge are separate.
5. An IF evaluates its bool condition exactly once and exactly one lexical arm. Both normal arms are subject to the source formation/type/operation rules; lazy runtime selection is not permission to omit static checking of the other arm. Normal result types must be exactly equal without implicit conversion.
6. Evaluation is source-order, left-to-right. Within a selected arithmetic expression the named numerator value-use precedes the typed divisor operand, then the operation produces its result. Bindings execute in source order. Statement semicolons discard results; tail expressions forward them.
7. Ordinary non-generic functions use exact declared signatures. main can call select_quantity declared later in the same unit: ordinary signature visibility is independent of declaration order. Actual arguments are evaluated left-to-right and copied into fresh parameters; the known call applies callee body semantics to those actuals. This source law does not assert that a public generic whole-call proof API exists.

## Per-unit oracle

### P1: r1608_p1_main_zero_then.nl -- lawful main, selected quotient, genuine zero

seed is 0. copied copies that seed and remains 0. The comparison 1==1 is true. The then arm computes copied/7 = floor(0/7) = 0. The else remainder is not evaluated. selected and result_copy are 0. The later result_copy, copied and seed statements each lawfully copy/discard their scalar values. main returns unit.

Abstract source trace: bind seed; Copy to copied; evaluate condition once; evaluate exactly the then arm and one division; forward IF result; Copy to result_copy; three scalar copy/discards in their written order; unit completion. No remainder evaluation occurs on this path. This is a source evaluation trace, not a mandated native instruction count.

The unselected remainder would also have mathematical value 0 if separately evaluated. Consequently even a genuine observed numeric 0 cannot by itself prove which arm was selected, that division occurred, or the evaluation quantity/order. Known zero is a valid value; it is not an error sentinel.

### P2: r1608_p2_helper_else.nl -- lawful ordinary helper and call/result Copy chain

main binds seed=53 and flag=(1==2)=false. At the one source call, seed is copied first and flag second; the helper receives fresh input=53 and take_quotient=false. The helper copies input to copied. Its condition is evaluated once; only the else arm evaluates copied%7. Since 53=7*7+4, selected=4. result_copy is a Copy of selected and is the helper's normal tail result. main receives value=4, copies it to echoed=4, discards echoed and a fresh Copy of seed=53, and returns unit.

Exactly one helper invocation and one remainder evaluation belong to this actual source path; no division evaluation occurs in that call. The then arm is nonetheless a lawful usize result for every representable input and bool: its mathematical result at input 53 would be 7. Both operands and both arm result types remain usize.

The helper definition is lawful for the full usize input range because each divisor is the literal 7. Definition-time Unknown input is not known zero, and the actual call's value 53 must not be imported into an unrelated observation or used as an unrecorded replacement for parameter provenance.

### C1: r1608_c1_selected_div_zero.nl -- true-invalid checked division

The true condition selects copied/usize(0), where copied originates from seed=53. The explicit divisor 0 violates the required nonzero precondition. This source must not receive successful checked admission for that operation. There is no lawful quotient/result-value oracle for it. No diagnostic spelling or precise parser/check stage is required by this oracle. A refusal for a different finite limitation does not establish that the zero-divisor obligation was checked.

### C2: r1608_c2_selected_rem_zero.nl -- true-invalid checked remainder

The false condition selects copied%usize(0), where copied originates from seed=53. The explicit divisor 0 violates the remainder nonzero precondition. This source must not receive successful checked admission for that operation. There is no lawful remainder/result-value oracle for it. The same diagnostic/stage observation limits as C1 apply.

### C3: r1608_c3_unselected_unit_arm.nl -- true-invalid normal arm type join

Both arms complete normally. The then result is usize; the else result is unit. Section 27.4 requires exactly equal normal result types and supplies no implicit conversion. The IF therefore has no legal normal result type, even though its concrete condition is true and the unit arm is unselected at runtime. Successful checked admission as a lawful scalar IF would contradict this source rule. Do not erase the else arm, change it into a terminating arm or add an implicit numeric/unit conversion to make this control pass.

### C4: r1608_c4_hoisted_alternative.nl -- lawful coverage and independently compared alternative

This retains P2's main values, declared helper interface and result/caller Copy chain. Inside the helper, copied/7 is evaluated first and binds quotient=7; copied%7 is evaluated second and binds remainder=4; only then is take_quotient evaluated by the IF. The false arm copies remainder into selected; result_copy and the actual caller result are 4; seed remains 53; main returns unit.

Both operations have a nonzero literal divisor and are defined for every usize input. This is therefore a small ordinary source alternative with the same helper result for either bool and any representable input, using already selected laws.

It changes the abstract source trace: two arithmetic evaluations before the IF, rather than one arithmetic evaluation in the selected arm. It is a useful value-equivalent alternative and lawful coverage control, not borrowed evidence for P2's original arm occurrence, evaluation trace or world. Pure operation folding/elimination is a backend matter; this oracle does not demand two machine instructions or a physical branch. An alternative's successful result cannot repair or erase failure to establish the original P2 source's evidence.

## Genuine-origin and observation obligations

- Every observation must refer to these exact frozen source bytes, the actual later released target and actual source-issued objects for that unit. Keep source identity, operation occurrence, binding/value-flow lineage, function and actual invocation context together. Lexical names/locations here are review locators, not invented compiler IDs.
- P1's arithmetic numerator must be the value genuinely copied from its own seed through copied. P2's numerator must descend from its own actual seed argument through the fresh input parameter and copied. A matching literal, hand-filled observation, independent arithmetic run, C4 event or producer/other-R object is not that evidence.
- For the IF, connect the genuine condition/result to the selected arm, operation occurrence, operand values and result. For P2, separately connect the actual call arguments, callee occurrence/result and caller value/echoed. A lawful isolated arithmetic proof does not by itself establish the enclosing IF, call, returned result or complete execution world.
- There are no allocated-object/heap identities or ownership authorities in these sources. Do not invent physical IDs, an authority issuer, ABI names or a public proof interface. If a later allowed public interface cannot establish an obligation, classify it as NOT_OBSERVED or limited evidence. Do not widen access or design a mechanism under this PREP.
- "Known zero" in this report means numeric zero established by the actual source law/evidence; "Unknown" means that numeric knowledge is absent. These are conceptual classifications, not assumed public enum/field names. Never coerce Unknown, an absent result or a default-initialized numeric slot into zero. Conversely, do not reject genuine numerator/result zero merely because it is zero.
- No assertion, output API or entry-point return convention is provided. All main functions return unit. A process exit code of 0 could show normal process completion in a future released runner but proves none of the interior quantity values, selected-arm identity, count, order or call/result/world correspondence.
- Source evaluation order is distinct from whole-program observable behavior and native instruction order. A numeric result is weaker than authentic source-event correspondence; a structural/public proof is not automatically native execution evidence; a native result does not automatically prove the public issuer's authenticity.
- Freeze original lawful sources even if later refused. Record the actual first failing layer and preserve all outcomes. No case removal, oracle rewriting, substitution by C4, guessed diagnostic, borrowed result or silent source repair may promote a later claim.

## Technical conclusion, goal concern and handoff

Technical source conclusion: P1 and P2 are lawful in the explicit literal-range envelope; C1/C2 are invalid checked zero-divisor operations; C3 is an invalid normal arm-type join; C4 is a lawful hoisted alternative. No new source law is needed for this finite selection. No implementation finding, candidate gate or empirical acceptance claim is made.

Goal concern: ordinary scalar main/helper/let/Copy composition should contribute to reusable v0 capability under Value 1.3. A separate trusted profile or expanded hidden author/caller obligations for each frozen case would be a goal risk, not permission to change source law or this oracle. These finite quantity sources do not measure author plus ordinary-safe-caller UB-prevention audit/re-audit dependency closure against well-factored C. That comparison remains UNMEASURED; Product GO #624 PAUSED and H1 HOLD are unchanged.

General source permission, absence of a canonical conflict and this PREP do not release any code/API apparatus or execution. After complete FIRST POST and individual GET/re-extraction/hash-and-byte verification, post PREP FINAL and individually GET it, then STOP C. A future target/public-interface/raw-transport/write-path/finite-FAST-and-FULL release is separate.
