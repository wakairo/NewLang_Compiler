# Draft 17.23 bounded Node executable topology — Issue #166

Track: P

Authority: CURRENT_SPEC → Draft 17.23. Historical design audit: **N/A — faithful
Draft17.23 implementation** (§§16.3, 17.1, 13.7/13.8, 10.1/10.2, 26, 17.4).
DI-001 (`@`/`::`/reserved dot), DI-002 (future privacy), DI-007 (ptr/ref-base
projection remains deferred) were checked. Semantic delta = 0; no specification,
Ledger, FormalProof or process changes.

## Exact backend profile

One completed §16.3 recursive nominal, its declaration-order link at index 0
(`Option<ptr<H>>`) and u8 sibling at index 1. No spelling is special. Exactly two
lexical roots in one no-argument, unit-result ordinary entry body; each root is
constructed with None and a checked u8 initializer. Option/ptr/u8 local Copy,
link reads, empty-item scoped loan bodies with ptr_from_ref / replace / unit
as tail, and the existing unchanged-public-frame, two-normal-arm unit match
are supported. A Some payload binder remains local to its owned arm context.

Extra roots, whole Node copying, nested-root scopes, nonempty loan bodies,
additional direct calls in a Node body, returns/IF/loops and richer match results
are outside this **backend** profile. Existing non-Node V0/V1/AVS/Pair/local-root
routes retain their implementations and supported sets. A language-valid
unsupported construct returns `V1-BACKEND-UNSUPPORTED` with no C output.
This restriction is not a language rule or proof that such programs are invalid.

## Rule → checked evidence → emitted operation → native assertion

| Existing canonical rule | Checked evidence | C17 operation | Read-only native assertion |
|---|---|---|---|
| §16.3 exact completed H | registry recursive-header/completion predicate and committed field type/index | backend-private forward-declared `nl_node`, tagged `nl_node_option`, two emitted lexical locals | distinct real Node addresses, initial None, checked sibling values |
| §§13.7/13.8 read loan; §10 ptr_from_ref | implicit-local place/incarnation, ref binder/type, nonescape/forwarding, scope-independent readable provenance | `const nl_node *r = &local; ptr = r` | pointer local equals the address of the registered emitted root |
| §17.1 link selection | nominal/base/index, parent/child incarnation/fact, access, dependency compatibility | `local.f0` Copy / `&local.f0` ordinary write ref | actual head link, independent copied Option storage, unchanged siblings |
| §17.4 replace | old/new value, fresh parent/child post-facts, exact link child identity | save `old = *w; *w = replacement; result = old` | None→Some, Some→Some, Some→None; old None/Some values remain independent after subsequent mutation |
| §26 Option Copy/constructor | registered Option target, constructor variant and checked ptr operand; Copy identifier/read | tagged struct value copies with a real Node pointer payload | tag and pointer equality with distinct Option storage, retained values at function end |
| §26 match | every arm owned; actual payload binder; existing checker proof of unchanged incoming public frame plus its binding prefix | runtime switch on copied tag, Some `q = scrutinee.ptr`, both source arms emitted | actual selected arm and actual q pointer; source arm order does not select runtime branch |
| §§10.1/10.2, 13.8 reloan | checked from-ptr loan, live source authority proof, place/incarnation, ref binder, nonescape | `const nl_node *tail_ref = q` inside the checked loan extent | executed reloan address equals tail, actual tail sibling remains intact |

No machine layout or ABI claim follows from this C representation. No source
AST/text is reparsed by codegen; final-context liveness is not replayed as a
point-in-time proof. No branch-local IDs become public semantic IDs. Ancestor C
carrier lookup in a hypothetical arm is fenced by the published unchanged-frame
prefix; every other local is qualified by its separate backend frame.

## Observation boundary

`node_checked_probe` independently runs actual source registration and the
actual main call, then reads checked nodes/type/value/place facts. It creates
expected event records from operand identities, checked literal values,
constructor/old-package variants, field selection and the already-proved arm
reloan. It does not call the emitter. Source spans label test diagnostics only.

Generated C has inert observer macros by default. The integration test alone
supplies `NEWLANG_NODE_OBSERVER` as a host C preprocessor include. That separate
header receives **const** addresses/pointers of emitted carriers and observes
binding, replacement, selected arm/q, reloan and the final live lexical frame.
It never initializes, writes, repairs or substitutes source topology. No source
assertion/I/O/equality primitive, runtime provenance check or lifecycle policy
is added. Assertions remain active with NDEBUG. The observer is test evidence,
not part of language semantics or a compiler safety authority.

The test uses the unchanged primary fixture, renamed Cell/link/data with other
checked payload literals, Some→Some, and reversed source arm order. Both inert
ordinary output and observed output are strictly compiled and executed. Ten
negative controls corrupt generated operations (pointer, tag, Copy, arm, binder,
mutation, reloan presence/target, returned package, sibling); the native observer
must reject every corruption. These perturbed files are tests, never compiler
success artifacts or topology repairs.

## Failure / ownership / limits

`nl_checked_c_node` borrows an immutable checked entry/context, owns a bounded
staging buffer, and transfers it only on complete success into a NULL owner slot.
Caller frees it. No C bytes are published before validation/emission completes.
OOM, missing/incompatible evidence and bounded traversal/output limits free the
buffer and leave output slots and semantic state unchanged. One allocation path
is fault-injected; malformed proof/shape/binder/field/reference evidence and a
cyclic corrupted list resource fence are tested. No hidden mutable production
state is introduced. Limits (128 carriers/frame, 512 steps, 16 depth, 128 KiB
output) are implementation budgets; resource failure is explicit.

Source-checker rejection remains prior to both backends. Existing Node semantic
rollback/OOM/dependency/stale-incarnation suites remain authoritative for cases
outside the bounded actual source surface. Codegen does not grant new authority.

No allocation/lifecycle, detach/free, general recursive backend, cJSON, ptr/ref
field projection, general Option/record emitter, LLVM, FFI or next slice.
