# R675 fresh BLIND source/oracle freeze (PRE-COMPILER-RUN)

Authority:
- main (canonical Draft17.32): a0de7a86d04d00d75a2484dfed4184ccb177e06c
- proposed SOURCE rule (NOT canonical): Draft17.33 HEAD 389d9b71f4b21418d1a79a77846d27916889dae8
- implementation under test: 7189475f6aec9e7ccaa1edda4db3ebf6b9b58665
- execution: GitHub-only Actions, original genuine .nl via normal newlangc; no P-like local env.
- independent freeze performed BEFORE reading candidate implementation and BEFORE any compiler run.
- The Git SHA entries below are Git blob object hashes of the exact source bytes. A changed byte requires a separately archived successor and original finding preserved.

| Test | immutable source git-blob SHA |
| --- | --- |
| positive_index0.nl | 6933757b5a7caf9ac992a5c815fe1a448c0a7e4a |
| positive_index1.nl | 446af647eb8c57ac75bd554107193f5471c4c97a |
| negative_unknown.nl | 32375ece21ce4a8c7b5816ade5b7232ccda65481 |
| negative_initializer_type.nl | 92be89cb34f3081d52f1f734bc8ae60b0a6241b9 |
| negative_decl_type.nl | bd644b3541c35a0ba23a418943efb96fec5b01ce |
| negative_escape.nl | aaf62ac02e88c07ca6e247343364d8618d5c56a8 |
| negative_replace_type.nl | 3c52bd0029e9e7ee3ea15a05a71db52301095c2e |

Independent semantic and numerical oracles:

1. `Fossil { seam:u8, quartz:u8 }`, one live direct original lexical root `shard`, declared index0=seam, index1=quartz. Constructor lists quartz FIRST but semantically index0=91 and index1=37. `before=91`, `neighbor_before=37`, `replace(shard@seam,146)` returns old Copy `91`; `after=146`; `neighbor_after=37`. One completed scoped write; original physical record address remains constant; its field0/field1 addresses must both belong to this same original, be distinct, and map to declared indices; old Copy and physical new value must agree.
2. `Quiver { glow:u8, depth:u8 }`, one direct original lexical root `arrow`, declared index0=glow, index1=depth. Initially field0=29, field1=173. `before=173`, `neighbor_before=29`; `replace(arrow@depth,61)` returns old Copy `173`; `after=61`; `neighbor_after=29`. One completed write to DECLARED INDEX 1. SOURCE semantic acceptance is expected under proposed Draft17.33; a separate genuine `CHECK0 + explicit backend unsupported4 / zero C` is not a semantic error. Do not secretly substitute an index0 write to obtain C17; if C17 is emitted, native/physical oracle as above.
3. Unknown member `specimen@missing`: reject at resolved-field semantic lookup; nonzero check and emit, zero emitted C. No fallback to nominal of same layout.
4. `Kestrel{upper:usize(11),lower:u8(17)}`: reject wrong initializer exact field type (semantic/type, if admitted to AST), zero C.
5. `Clasp{one:u8,two:usize}`: reject two-field declaration for bounded-two-core-u8 eligible category (semantic SOURCE eligibility rather than backend acceptance), zero C.
6. Escaping `ref<write,u8>` from `loan_write(chime@signal)`: reject scoped ref escape/dependency at semantic stage, zero C.
7. `replace(w,usize(3))` on `ref<write,u8>`: reject exact replacement type at semantic/type stage, zero C.

Gate expectations:
- Parse every syntactically well-formed source through actual registered function; all candidates must be ordinary source-issued, not test-seeded IDs.
- Positive: `newlangc --check` exit=0 and empty stdout/stderr (SOURCE/CHECK). For `newlangc SOURCE`, either real C17 (native required), or explicit backend4 with empty stdout. Semantic CHECK3 on a candidate-valid positive is a source implementation gap; a hidden broad workaround is prohibited.
- Negative: `newlangc --check` and `newlangc SOURCE` fail with classified parse/semantic diagnostics, nonempty stderr and NO C17 stdout. Any CHECK0 for these semantic-invalid questions is a finding, even if backend later refuses.
- If real emitted C17 is present, build STRICT C17 via compiler provided by Actions matrix. Passive observer may read original C record and original member addresses/values before/after the generated store, never replace values, create a proxy, fabricate root/ValueID, or alter emitter internals. No numeric address is preregistered: require runtime root/member relational identity and before/after stability.
- Do not infer source-issued value authenticity solely from public view or C number equality. Real SOURCE, ordinary CHECK, public read-only owned inspection if a safe API exists, backend and native are distinct observations; provenance integrity not universally proved.
- Any formatter/test-driver/observer repair preserves these original files and oracle, previous CI logs and attempts; no compiler/issuer/backend/spec edit. Stop if actual safety/identity mismatch, algorithm fix, CI gate failure, or scope escape.
- No P670/P671 source, oracle, rationale, report or other producer result has been consulted before this freeze. Existing main §17 source syntax and ordinary main-based fixed field test-driver mechanics may be read.
