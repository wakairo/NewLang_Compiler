# Stage2A preimplementation design decision

Track P, fixed0eb528e / Draft17.34, RELEASE6356093556. No production edits at
this checkpoint. Source/oracle freeze53476d5c53239648409dc68992c67d51c867f239.
Both independent order sources: CHECK0, owned Copy views present, validators3,
H TypeId11 layout0/0/0 ->1/24/8, same body/caller world and genuine source graph.
Existing T facts are old32/new91/sibling168 and old204/new75/sibling17;
second B call transfers actual old204 to another original, returning6.
All H cleanup worlds close in actual semantic arms; this is not native freeing.

## Compare A/B/C before changing code

A. **Choose stable target layout preparation before body snapshots.**
`nl_allocated_registry` already contains a private Linux/x86_64 representation
plan for the exact completed H category (one-link24/8, three-link56/8). Its own
comment explicitly distinguishes type metadata from live allocation authority.
Extract exactly that existing layout/compatibility calculation to a private
candidate-only helper. On mixed-unit registration, after actual H header has
completed and before any signature/body snapshot, use its exact returned TypeId
to prepare the same supported-target layout. Later actual allocation reuses the
same helper and all existing H/Raw grant, region/domain and cardinality checks.
Do not pre-create OneBacking/option types, add TypeIds, mint values or resources.
On other execution targets the preparation introduces no layout fact; the old
actual allocation platform fence remains unchanged. Keep single-nominal source
registration scheduling unchanged.

This is metadata scheduling using the existing trusted target plan, not a new
layout issuer or normative ABI. It is reusable for every eligible mixed source,
independent of H/T names, field labels, literals or call order. The enclosing
cloned registration transaction rolls preparation back on all failures. Public
validators and `nl_copy_same_types` remain byte-identical: they must compare
fully identical facts, not tolerate unknown/arbitrary layouts. An out parameter
from the real H completion identifies the original registered nominal; do not
search by equal numeric IDs or import facts from another world.

B. **Defer source-issued completion equivalence bridge.** It could relate old
and refined type knowledge, but would need new authenticated cross-world receipt
and independent validator rules. That is larger and may cross the Issue's new
proof-authority STOP condition. No need for it if A suffices.

C. **Defer new immutable world/type-epoch representation.** Versioned type tables
could model knowledge changes precisely, but touch all existing snapshots, type
identity and lifecycle/proof ownership. It is substantially broader than the
finite metadata scheduling correction and unnecessary for this exact category.

## Expected scope and gates

Production: existing allocation target-plan helper, its private declaration,
registration staging. No Copy issuer/validator/public API/emitter changes.
Migrate only the P752 unit expectation that explicitly recorded the now-targeted
precision refusal; retain all old source, oracle, first failure logs and README
as historical evidence. The no-heap Copy validators must remain valid. The two
new mixed sources and old two-original P752 source should now validate existing
Copy receipts, with stable H metadata and original T facts/scopes. C17 stays4,
zero output, sentinel intact. No H dual-carrier certificate is claimed.

After the first implementation, an additional unexplained proof, semantic,
rollback, OOM or CI failure is a STOP: save it and report rather than broaden the
fix. Do not simply relax any equality check. If preparing the existing target
plan changes source meaning or requires new authority, stop before that change.
Tests will challenge source owner/world, root/child/index/fact, old Copy origin,
H/T nominal/layout/self target, original Allocation/Domain and OOM recovery.

Historical audit: N/A faithful implementation of adopted §16.3.8 / DI-018,
keeping DI-001/006/007/017 and existing H target/ownership rules. No source rule,
new world equivalence, public proof authority or target model is selected.

Goal alignment: capability is a real source-issued T Copy receipt in a mixed
semantic program. This does not certify physical H cleanup, dual-carrier native,
ordinary-safe library audit locality or Product GO. Split units/scalar replacement
do not preserve the chosen user program. Product GO624 PAUSED, H1 268 HOLD.
