# P501 pre-code / pre-execution freeze

Exact compiler authority0d29d61c114f179af8e7300096fd85ec485ad2e7 (merged P500),
CURRENT_SPEC -> Draft17.30. FormalProof5437f92d... is informational only.
Read Issue501 and Coordination6205546443. No candidate run before this commit.
All12 original source texts / SHA256 / expectations are in frozen.json.
Full P497 Pair programs copied byte-for-byte; they remain ultimate target
controls, not replacements for minimal equality sources. No oracle rewrite.

Historical Gate A: canonical §§5.2/6/7/9/27.4 and fixed main above.
Gate B: current and historically normative Draft17.4 §5.2 both explicitly spell
usize == usize -> bool. Checked Ledger DI001/006/007/008/011 and old Surface1/1.1
operator/usize/precedence search. No dedicated numeric-equality ledger entry;
this is targeted history, not exhaustive archive/chat. No conflicting selected
operator spelling found. Existing expression_extension rejects =; no equality
AST/checker exists. Two adjacent lexer punctuation tokens can implement the
already selected single == without changing lexer keyword/token contracts.
Gate C: KEEP existing single operator spelling and nominal exact usize/bool,
Copy/dependency and left-to-right rules. Only flat names/typed literal operands;
no precedence/associativity is selected. Chaining/compound operands stay profile
unsupported, not language-invalid declarations. DI001 field punctuation, DI006
loans, DI007 ptr/ref modes, DI008 numeric address != authority, DI011 known-call
requirements KEEP. DEFER other operators/overload/cast, unknown-value reasoning,
if effect consumer, ABI/CFG/effects/owner/Array/loops.
Gate D: N/A faithful §5.2 implementation, no normative source rule replacement;
no ledger/spec edit. If ambiguous source grammar emerges, return to C before code.
Gate E: no self-accept/merge; C selects any candidate and independent review.

Predicted first baseline: all equality sources CLI3 parser
P5-EXPRESSION-UNSUPPORTED at first =, not semantic safety evidence. Scope plan:
append syntax/checked comparison kinds; actual LTR expression checking, exactly
usize scalar packages; semantic type mismatch; known nonnegative mathematical
scalar equality issues bool0/1 with actual child IDs. Require dependency-free
values; unknown/dependent values precision-reject rather than guess or erase
requirements. Source-generated binding copies and renamed values must work.
No name/value allow-list, host semantic seeds, bool formal injection, root/D/A
registry or authority creation. Existing Copy eval/end temporary behavior stays.
A public comparison view links real operands/results; public observation is not
an independent validator/theorem. Test retained source and original actual
symbols/values after source AST destruction; no mutable candidate authority.

Existing source_if should accept the genuine bool in pure-unit control cases.
Its checked arms are both hypothetical; CHECK is not runtime branch selection.
Try full immutable Pair guards at CHECK separately, record precision if any;
no broad widening or IF emitter even if CHECK succeeds. Default emission must
fail closed at existing V1-BACKEND-UNSUPPORTED rather than silently discarding
comparison/if. Nonbranching bool C emission is optional and deferred here:
primary artifact truth/type and negatives buy the next decision without a
second backend operation or new target/runtime proof. No emitter code planned.

Freeze negatives: u8/usize mismatch -> semantic equality type error; bool as
write-ref -> existing P3-WRITE-REF-REQUIRED; unknown formals -> precision or an
earlier independent signature barrier. Chained/split = = -> parser unsupported.
Full Pair source fixed old0/true7/false8/sibling9 oracle stays UNTESTED at native.

Budget90–150min max; one branch/DRAFT and one natural FULL+PARTIAL CI pair.
Target source/public tests, OOM/semantic rollback, adopted u8/P500/P471/P490
and borrow/Option/nonCopy regressions. No changes to adopted tests/docs,
workflow/toolchain/LLVM/canonical/other experimental PRs. Stop at first unsafe
acceptance, semantic oracle surprise, new source selection, or independent
broad checker/issuer/emitter mechanism; preserve partial evidence.

Compass -> Product Strategy§1.1 and Coverage Map§§3.1–3.3/Process§4.3.1:
this is Exploration toward quantity guards, not Product GO or an audit-locality
win. Alternatives: literal-only P500, ordinary u8 (not usize quantity), separate
if consumer investment. Bounds/Array/Ring/H1/STB and C/Zig/Rust burden unmeasured.
