# P475 bounded result — CHECK/SIGNATURE PRECISION BARRIER

Compiler/main7139b8c64bf001001e4319c9a54bf9114e130f51, Draft17.30.
Pre-execution source/oracle freeze ee7aa11cb058bfe2fc8d9d94de22b20bdd4feed9.
One candidate execution, no source/oracle corrections or additional variants.

Existing P471 A control: CHECK0, public issued Node join/validator0, C17 CLI0,
2545 C bytes, SHA92359ec3c59d72c7286cd45fd28dd8f68302410d935d9417934609feeb123acc.
Existing public probes and existing P471 passive observer used unchanged;
GCC14.2 strict C17 O0/O2 plain/observed all4 native runs PASS. Original distinct
tail/head, actual q, self-link, old Copy and payloads2/1 were observed.

Candidate command: `newlangc --check tests/p475/named_link.nl`.
Exit3, stdout0 bytes, exact diagnostic:

```text
tests/p475/named_link.nl:2:1-4:2: error(precision)[P8-SIGNATURE-PRECISION]: signature needs unsupported body analysis
```

Parser passes: main.c998/1012 reaches semantic unit registration; no independent
second parser/candidate run. Source inspection locates the first guard at
semantic_check.c9024: result Option<ptr<Node>> fails body_signature_type because
sum payload ptr is not body_plain_type (8454/8472). Formals are checked afterward;
ref target Option and Option/ptr structural argument support are also excluded
by that predicate, but not separately executed as additional counterexamples.
The helper body, actual call/ref substitution, call effect certificate, emitter
and native candidate are NOT REACHED / UNMEASURED. No owned main entry is created
on the CLI failure path. This is implementation precision, not parser malformed
source, proven semantic invalidity, a backend finding or a safety acceptance.

Prediction of signature precision barrier confirmed. Historical G1 PR384 remains
closed/unmerged, ref<write,u8>,u8->u8 at a different fixed branch; it does not
establish current Node/Option call support. P471 inline is actual adopted PASS.
No general safe private unchecked library, module privacy, Ring or Product GO
is proved by either result. Canonical/H1/STB/V holds unchanged.

No new harness, API probe, CMake/workflow change, PR or CI run is needed for this
first CHECK stop. One frontend-only local configure/build using existing tools,
no LLVM download. Fixed-main FULL38718479149 and PARTIAL38718479062 were checked
as actual success/attempt1. Raw control/candidate records retained in evidence/.

Next map improvement for C only: scope and cost the exact ordinary signature +
independent definition/actual ref/Option post-state proof path; do not merely
remove this guard or infer backend support. Downstream layers remain unknown.
Production/spec/toolchain/adopted tests/docs/workflows untouched; no feature,
source rewrite, merge, Issue close or other Track. Final Issue report then STOP.
