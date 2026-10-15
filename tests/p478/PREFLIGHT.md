# P478 Gate0 pre-execution / pre-edit freeze

Compiler/main d6e603e12591c2ff39e3e1b77c784681fec66033; canonical Draft17.30.
Exact original P475 target and unchanged adopted P471 A control hashes frozen.
One positive and two safety negatives only, frozen.json. No compiler run or
Production edit yet. No source/oracle rewrite after measurements.

Goal: faithful ordinary body-visible safe call, preserving original pointer
 topology and actual checked effects, a necessary engineering step toward the
Product Strategy1.1 audit-dependency-closure value (not unchecked token count).
Compass -> Product Strategy1.1, Process4.2/4.3.1, P8 body-slice contract and
canonical13.5c/17.4/18.7/18.8/20.4 read. P471 inline positive is adopted;
P475 signature precision is historical current-source evidence; G1 PR384 was
CLOSED/UNMERGED, not a transplant base. No product/heap/Ring claim.

Historical A: authority/sections above. B: Design Decision Procedure, Ledger
DI007/DI011/DI012 call/field/identity guidance, old P8 contract actual-context
body interpretation and source-issued P471 proof checked. C: KEEP known body
substitution, independent definition checking, original ref/Copy/hidden deps,
Change/Reset and safe API no-human-proof semantics. N/A faithful finite compiler
implementation, no new source/API semantics or Deferred promotion.

Existing mechanics: parser already admits exact typed signature; run_body binds
actual argument packages (ref place/incarnation/scope, Copy sum including pointer
payload), runs immutable body under namespace floor in the SAME transaction,
forwards real tail package, uses existing primitive write_ref/conflicts and
nl_fixed_change, updates caller bindings, checks exit and rolls back on failure.
Independent definition formal contexts clear caller state and NEVER grant actual
root authority. Existing nl_sum_unknown_node_option supports opaque Unknown for
completed two-field Node Option without inventing Some/pointer/current authority.

Finite Gate1 plan: structural signature predicate for exactly ordinary write-ref
of that registered Copy Option + same Option argument/result (no names); represent
formal Option as Unknown rather than a favorable None; independently check new
profile bodies before any caller walk; preserve existing actual body interpreter,
primitive, root/field/occurrence/dependency checks and transactional lifetime.
No allow-list-only patch and no new fake provenance for formal payloads. Edits
initially confined to registration/formal-state functions in semantic_check.c.
Do not admit generalized sums, refs/heap/generic effect or new language rules.

Prediction AFTER P8 fix: direct-only nl_node_join_arm_check currently rejects ALL
REGISTERED_CALL nodes before examining their owned body; actual field snapshots
also live in the child body, whereas Node join issuer/validator requires a direct
arm REPLACE, local incoming constructor, and lexical loan in that same artifact.
Existing nl_checked_call_body exposes source-owned body but has NO independent
ordinary call entry/argument/formal/post-effect validator. Emitter local loan also
requires direct REPLACE. These are potential later barriers, not measured yet.

Bounded stop: try the coherent definition+actual-call checker step; if public
proof can only be obtained by a NEW cross-artifact effect-certificate mechanism
(qualified call/body path, argument/formal mapping, source-issued before/after,
return/nonescape/hidden effects), STOP at that first real frontier rather than
copying a field annotation, flattening source, ignoring REGISTERED_CALL, bypassing
normal_frame_unchanged, or hand-coding set_link. Matching proof/emitter adaptation
only if genuine existing evidence suffices locally; no fresh general framework.
Any unsafe authority acceptance stops immediately. A changed-head CHECK0 cannot
be called feature success without public proof and actual C/native evidence.

Frozen normal oracle: actual Some q from head.next, same original tail write ref,
old None Copy, original tail self-link, head reset None, payload2/1, distinct roots;
no escaped ref or extra heap. Negatives are actual pointer-as-ref and overlapping
live read; if a profile guard stops them classify honestly, never safety PASS.
One frontend-only preflight; exact original baseline first. New unit additions
may test independent formal/transaction/OOM without inventing source-owned proof.
If code changes: one DRAFT/DO NOT MERGE PR, one natural FULL+PARTIAL; no retries
except at most one mechanical harness fix. No docs/toolchain/workflow/adopted-test
changes, no old-branch cherry-pick, new Track, main merge or Issue close.
One terminal report then STOP; C owns integration/independent R/next investment.
