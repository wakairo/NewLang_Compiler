# P589 bounded source-issued usize addition — preflight

Start 2026-10-18 01:01:32 UTC. Base and origin/main at start:
`12c2c8e3338201ec3bda00a63ddac841ed80e574`; canonical CURRENT_SPEC → Draft17.32.
No feature source edits preceded `frozen.json` (01:02:42 UTC) or baseline CLI
runs in `evidence/baseline.json`. No P585 source/oracle changed.

Integration gate: actual post-merge **push** FULL run 39080512560 and PARTIAL
39080512572, exact base SHA, attempt1 SUCCESS (not PR-head CI). Frontend Debug
build, targeted `p572_|c17_target_model` baseline: 5/5 PASS, 1.17s. Thus no
pre-existing integration regression blocks this feature.

Historical gate: read Compass and linked Product Strategy §§1.1/1.3,
Development Process §§4.2/4.3.1/7, Decision Procedure/Ledger, Draft17.32
§§0/5.2/6/7/8/9/13.5a, P585, and M587. C's explicit scoped §4.2 adoption
https://github.com/wakairo/NewLang_Compiler/issues/587#issuecomment-6243326895
selects only one flat atom+atom alongside unchanged flat equality. Keep old
mathematical no-overflow/type/Copy/dependency/LTR semantics. Defer nested,
chained, mixed operators/precedence, pointer-coordinate arithmetic source,
dynamic range proof, general CFG/effect/owner systems. No specification edits.

Baseline: all arithmetic candidates stop at SOURCE (literal forms on `+` with
P5-EXPECTED-ITEM-END; name forms P5-EXPRESSION-UNSUPPORTED). Literal MAX+1
retains USIZE-LITERAL-RANGE; old equality succeeds. These are parser refusals,
not overflow-proof or ownership successes. Full diagnostics retained.

Implementation forecast: one small-to-moderate scalar path analogous to P504,
plus one compact immutable source event per addition, not a world snapshot or
large parallel proof framework. Trusted issuer captures actual current binding
place/incarnation/ValueFact and resolved Copy operand packages. Independent
public validator authenticates the retained registered source/operands/spans,
actual packages/type/dependencies and guarded mathematical sum against that
issued event. The receipt's historical currentness is trusted source-checker
issuance, not a claim of present live access or a cryptographic certificate.
Before computing a+b, prove b<=SIZE_MAX-a. Emit actual original C carriers.
Rollback, OOM, foreign origins and corrupt actual nodes/packages must fail closed.

Goal: reusable checked quantities for later allocator/container/index work;
no Product GO claimed or required. Alternatives: literals/equality only;
selected flat addition (chosen); broad expression interpreter (deferred).
Stop on grammar collision, integration regression, missing genuine issuer,
unsound validator, broad arithmetic/authority machinery or budget exhaustion.
No goal-alignment concern identified within this selected bounded investment;
product audit-locality and dynamic arithmetic remain unestablished.

Frozen prediction correction (source retained): `escape.nl` predicted an exit
error, but a plain usize is not a supported scoped-loan root, so its first
observed boundary is LOCAL-LOAN-PROFILE. This does not test a successful loan
followed by semantic escape rejection; no new loan profile is introduced.
