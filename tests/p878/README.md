# P878: source-issued unit loan bindings, bounded C17 candidate

Base: P872 a8057d1f82e1c6ca498d1ff43860fdf9822b97a0, Draft17.34.
New branch p878-unit-loan-binding, child PR against the exact historical P872
branch only. No current-main rebase or independent R evidence imported.

Before production changes, independently freeze source/native oracles in
3c369ca84abdfa784a18ea1d22c464ce10f1a9fb. Twelve positive sources compare
unit binding with discarded loan statement: two new nominal types, field
indices 0/1, equality true/false, both normal-arm ordinals, and existing less-than
controls. Relay old82/incoming156/sibling199/receiver[23,141]; Audio old44/
incoming87/sibling206/receiver[119,13]. These are P-owned expectations, not R875
oracles. All source bytes and scalar expectations remain unchanged.

The first baseline passed all12 actual-source CHECK/public probes. Six bindings
failed closed CLI4/C0, six statement twins emitted C17. The baseline script then
failed its P-owned malformed-source exit assertion: expected CLI2, actual CLI3
error(syntax)[P5-EXPECTED-NAME], still proper syntax rejection with C0. Original
freeze and failure remain immutable. Separate PRE-PRODUCTION checkpoint
9655a62ca4346158bad89d8fa304e4f94075526f corrects only that diagnostic taxonomy.
This counts conservatively as local correction1; the single production cycle
counts2 of at most2. No initial complete-baseline PASS is claimed.

Only production src/main.c changes: a shared unit binding shape predicate keeps
receiver/symbol/type/zero-result/source LOAN_HEADER constraints aligned. Admission
actually recursively validates the initializer via existing gate_loan, registered
call, public Copy/IF-return validators and original carrier checks. Emission
executes that loan once at its lexical point, returns without any unit/u8 value
carrier or scalar loan-result read. Unit literals/calls/general IF initializers
are not made valid by unit type alone. No issuer/schema/semantic rights change.

Five additional registered CTests (expected FULL5 462/PARTIAL461):

- p878_unit_binding_public.unit: existing immutable public probe on all12 NEW P
  inputs; authentic unit/parameters/IF-return/events, old/incoming/root/child/
  sibling/scoped loan closure and conservative caller postcall Unknown.
- p878_backend_guards.unit: test-private actual driver validation on genuine
  source-owned graphs;11 binding/receiver/result/scope/place/fact/source
  mutations per case, refusal and restored public/backend recovery.
- p878_prefix_integrity.unit: unchanged P866 integrity machinery on two NEW P
  sources,16 mutations per case plus foreign owner/root/formal/old Copy.
- p878_reached_prefix_oom.unit: unchanged P866 reached-prefix allocator harness
  on two NEW P sources,2 real calloc failures each, OOM/unchanged-world/retry0.
- p878_source_cli.integration: all12 normal source/plain C17 and separate passive
  observer, strict O0/O2 with the configured host compiler/sanitizer; actual
  helper called once, caller continues once, expected normal store0/1, original
  field and sibling/receiver state. All pointer dereferences occur while the
  original owning C function's locals live; addresses clear before scope ends.
  Wrong argument/return/syntax and unsupported unit-literal binding retain
  rejection, zero C, named-output sentinel and temporary cleanup.

Local focused9 (including all4 unchanged P866), GCC13/Clang23.1.3 O0/O2 native,
Clang guest ASan/UBSan and pinned formatter passed in the first production cycle.
All6 existing P866 and6 new statement-twin plain C17 outputs are byte-identical
to their pre-change captures. No R source was substituted as a P positive.

The first multi-target incremental build regenerated CMake during newlangc and
then its already-loaded old Makefile could not find newly added p878_guard.
The production target built successfully; invoking the now-registered new target
compiled it successfully without source changes. Both command logs are preserved;
this is incremental build ordering, not a compiler semantic/native failure.

Full local frontend:461/461 PASS (63.89 seconds). All5 new names plus all456
prior frontend names actually executed; pinned formatter passed after staging.

Local/CI outcomes and immutable full logs/C17 manifests are recorded in the P
preservation commit message and Issue878. CI may be opened only after full local
frontend succeeds: one natural FULL5/PARTIAL attempt1, no retries/post-PR pushes.
R875 original BLIND INCONCLUSIVE, source/oracle and first failed CI stay fixed.
No R875 guest-native success or universal soundness is asserted here.

Historical gate: KEEP Draft17.34 unit/call/scoped Change and source-issued old
Copy vs caller conservative join (DI017/018); no new source/ABI/FFI/owner/effect.
Capability evidence is not library-author/ordinary-safe-caller audit-locality
Product GO (UNMEASURED). Product GO PAUSED/H1 HOLD; DO NOT MERGE.
