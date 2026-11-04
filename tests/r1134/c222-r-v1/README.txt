# C222 Track R V1 - independent source freeze

Authority: Compiler/main 28726f933ff4885c000e0247cf4fd9ac1859c5b1.
CURRENT_SPEC selects Draft17.34. P candidate HEAD: none.
This bundle contains predictions only. No source, compiler, native, build,
test, Lean, branch/ref, Git object, push, PR, or Actions execution occurred.
Pure JavaScript was used only for string/data construction, UTF-8 byte
materialization, SHA-256 calculation, and fence round-trip consistency.

## Input boundary

Read: Issue 1134 and its C222 launch comment; fixed-main Compass;
Value Strategy 1.1; Process 7/8; CURRENT_SPEC; canonical quantity arithmetic,
typed literals, T/Copy/Change/dependency, ordinary functions, source scope,
if, loop, Continue, Break, and no-normal-exit rules.

No P1133 body/comments, P implementation/tests/fixtures/oracle/reports,
M/F/B concrete examples or rationale, or former R artifacts were retrieved
or reused. Canonical and pointer files contain historical cross-references;
those linked reports and examples were not followed. Ordinary syntax was
read from canonical text itself. All names, numbers, case choices, and
expected traces in this bundle were selected independently here.

## Bounded cohort and mathematics

p01: 2 -> 4 -> 6 -> 8; three Continue edges, then one Break.
p02: 8; zero Continue edges, first-entry Break.
p03: 6 -> 8; one Continue edge, then Break.
The increment is evaluated only in the true arm of quantity <= usize(6).
There 0 <= quantity <= 6 implies 2 <= quantity + 2 <= 8.
The Break arm performs no quantity arithmetic.

p04: 11 -> 8 -> 5 -> 2; three Continue edges, then Break.
The decrement is evaluated only in the true arm of quantity > usize(2).
There quantity >= 3, so 0 <= quantity - 3 <= quantity <= USIZE_MAX.
This variant changes declaration-index 1 (glow); its named initializer
lists glow before clay to preserve the distinction between source order
and declared field identity.

These facts are independently derived concrete traces and arm-local
representability arguments. They are not compiler measurements. Header
analysis must cover every reachable later iteration, not just entry.

All four primary inputs declare one complete nonrecursive Pebble nominal
with exactly two u8 fields, derived Copy and Discardable. There are two
separate lexical originals of that same nominal. All Continue edges leave
both originals and both fields unchanged. At Break, loan_write on one
fixed field passes an explicit ref<write,u8> to an ordinary acyclic helper.
The helper returns replace's old u8 Copy. Its return is forwarded through
the loan and Break; the ref itself does not escape. The later caller writes
the same field again. Plain integer copies carry no added blocking
dependency on a source current-value fact. They retain the old numeric
value; the original current field and ancestor facts change. The
known-disjoint sibling and the separate original remain unchanged.

p05 is a separately identified legal precision control: a total,
arm-guarded ordinary helper computes the next quantity. Its main header
trace is 3,4,5,6,7, with four Continue edges. The helper is called even on
the final iteration and returns 4,5,6,7,8. A compiler may soundly reject
unavailable body-sensitive/cyclic precision. That is different from an
invalid source or an implementation defect within a declared scope.

c06 repeats quantity * usize(1) forever. This operation is representable
for every usize. The loop has no normal Break exit and produces no
synthetic unit result. It must never be run without a separately released
finite interruption budget.

## Meaningful negatives

n07: guard > 0 permits 7 -> 4 -> 1, then 1 - 3 is outside usize.
Two initially safe backedges do not make a later unsafe state legal.
n08: guard > 6 supplies only a lower bound. For unrestricted seed
USIZE_MAX (the corpus target represents 11), the evaluated +2 overflows.
n09: a usize loop parameter cannot receive a u8 Continue argument.
n10: Continue targets the nearest inner two-parameter loop; one outer
binding supplied as the argument neither changes the target nor arity.
n11: a loan body blocks inheritance of the enclosing loop-control target.
Without an inner loop its Continue has no active target.
n12: the arithmetic loop is locally safe, but Break returns the loan-bound
write ref and the enclosing loan would forward it into an outer binding.
Scope dependency survives; it cannot be erased by loop result forwarding.

All six are source-invalid in their indicated obligation, even though
some are grammatically well formed. They require rejection before native.
No invalid case is given a native success oracle. Diagnostic wording and
the earliest implementation rejection layer are not frozen.

## Observation and identity limits

oracle.json fixes source-binding values, count/trace predictions, final
quantity, helper old result, later caller old/current values, sibling and
separate-original state, and same-execution address/incarnation relations.

The primary function result directly returns the old Copy. Local binding
values and counts are semantic observation points, not promises of an
existing public native ABI. No source instrumentation or runner has been
invented here. Address checks are relations across phases of one execution;
absolute addresses, field offsets, and comparison across executions are
not predicted. Both live originals remain distinct; Change does not make
a fresh original or rebind a pointer to a copied proxy.

A later C release must settle a faithful observation adapter and concrete
P scope before execution. This is an unmeasured observation-contract item,
not permission to edit the frozen source to fit P.

## Verdict boundaries and return to C

Canonical legality, compiler acceptance, public owned, C17/native,
general soundness, and product value are separate verdicts. Every
execution layer is NOT_TESTED. Finite traces do not establish library
author plus ordinary-safe-caller UB audit locality. Human burden,
reaudit scope, and comparative C value are unmeasured; GO624 PAUSED and
H1 HOLD remain unchanged. These small sources add no new authority or
language mechanism and do not request a new track.

No unresolved core syntax/semantic ambiguity was identified by this
independent reading of the selected forms. If a source/admission doubt
arises during review, hold the affected input and ask C using canonical
text; do not repair meaning by assuming legality or by consulting P.

Future execution mode, if separately released: a dedicated GitHub branch
and Actions, never an environment equivalent to P's. The present freeze
authorizes neither that branch nor an object publication nor a run.
After this frozen handback the track stops.

## Canonical basis

- 5.2 / 6: quantity representability and typed literals.
- 13.5a: semantic dependencies, Change overlap, scope-exit compatibility.
- 13.8: exact-once loan and unchanged normal-result forwarding.
- 16.2 / 17.1: construction by field name and exact-two-u8 T direct fields.
- 17.4: lifetime-preserving replace, returning old value.
- 18.1-18.5 / 18.7-18.8: explicit ref arguments, Copy result, caller state.
- 19.1: lexical sequencing and loan/function control boundaries.
- 27.4-27.8: arm selection, all-reachable header states, exact Continue
  type/arity/target, Break post-state, and no-normal-exit behavior.

Canonical: https://github.com/wakairo/NewLang_Compiler/blob/28726f933ff4885c000e0247cf4fd9ac1859c5b1/docs/reference/NewLang_v0_spec_Draft17_34.md
Value: https://github.com/wakairo/NewLang_Compiler/blob/28726f933ff4885c000e0247cf4fd9ac1859c5b1/docs/NewLang_Product_Value_Strategy.md
Process: https://github.com/wakairo/NewLang_Compiler/blob/28726f933ff4885c000e0247cf4fd9ac1859c5b1/docs/NewLang_Project_Development_Process.md
