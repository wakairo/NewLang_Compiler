# P0 architecture and M7 constraints

This is Class I implementation design. It cannot override Class N Draft 17.4
or Backend Contract v0.4. Class A closure, O oracle and F formal evidence inform
review only. The attached policy and reference snapshots remain unchanged.

## Current small boundary

`main.c` owns CLI dispatch and requests diagnostics through `diagnostic.h`.
`diagnostic.c` renders validated records. All strings, ranges, note arrays and
the FILE stream are borrowed for a synchronous call; it neither retains nor
frees caller storage. Records carry severity, optional category/code, message,
optional one-based exclusive-end source range, and optional located notes.
Absent source information uses NULL, not fabricated parser coordinates.
Invalid records emit nothing; stream errors may leave partial output and return
false. The caller owns the stream and its final flush/close responsibility.
There is no mutable global compiler context or allocator subsystem yet.

The sole LLVM integration file is `tests/integration/llvm_smoke.c`, which owns
and disposes a context, module, builder, verification message and IR string.
Types/values/blocks are module/context-borrowed handles. It builds a scalar
function returning 42, verifies the module and checks its textual IR. This
demonstrates **C compiler -> LLVM C API -> valid module**, without implementing
NewLang semantics or claiming machine-code execution.

Future architecture direction (not an implemented IR hierarchy):

```text
source -> syntax/AST -> semantic checking -> checked semantic IR/MIR
       -> backend contract -> LLVM lowering
```

LLVM headers do not appear in the CLI or diagnostics interface. Future semantic
subsystems should emit structured diagnostics rather than print directly.
No speculative directories/types are created for later compiler passes.

## Binding M7 boundaries for P1 and later

Draft 17.4 §§13.5c, 23, 31, 34 and Backend Contract §§1–11, addenda A–F, H–M,
R–T constrain later lowering:

- `ref<write,T>` does not automatically prove LLVM `noalias`.
- Source exclusivity does not automatically prove LLVM `noalias`.
- Source ref nonescape does not prove captureless backend pointer behavior.
- NewLang semantic lifetime does not equal LLVM lifetime markers; heap/raw
  typed transitions must not become general `llvm.lifetime.*` markers.
- Alias scopes require separately proven disjointness over the relevant extent.
- Native aggregate/sum layout is opaque and has no implicit C ABI identity.
- Target ABI classification is backend-owned; generated C shims are acceptable.
- Foreign signatures/calling conventions are not semantic summaries.
- Missing semantic/backend facts produce conservative fallback, not promises.

Future checked state must preserve independent proof origins for source
access/lifetime/dependency effects, access exclusivity, capture, extent/alignment,
target null-address policy and storage kind. Foreign ABI, semantic effects,
backend memory access, retention, callback, control transfer, construction and
external-backing facts cannot be collapsed into one boolean or inferred from
each other. P0 does not materialize these records before a concrete need.

No C struct layout, LLVM attribute or Lean Finset/ghost identity representation
is a source-language rule. The F0 bridge and live proof repository are sibling
evidence; no Lean build or copied proof runtime representation is introduced.

## Oracle/differential interface

The original M7.5 ZIP is committed, with its SHA-256 in `oracle/identity.json`.
Keeping the archive avoids accidentally editing the historical implementation,
preserves its test corpus, and requires no remote registry or git submodule.
The harness checks the hash and safe paths before isolated temporary extraction;
sets explicit Python module paths; invokes checker/CLI in subprocesses with
timeouts; and removes extraction afterwards. The adapter uses standard library
only plus the oracle's bundled dependencies.

`Outcome(supported, accepted, diagnostic_code)` provides a minimal differential
seam. `run_oracle` returns checker behavior, `run_production` recognizes the exact
P0 unsupported result. `compare_outcomes` refuses unsupported results. P0 smoke
checks four historical fixtures and one CLI invocation; it does not claim any
production semantic agreement or 466 C semantic test results. Add future
fixtures only with a reviewed normative basis and explicit diagnostic mapping.

## Discrepancy procedure

Use COMPILER-SPEC-HOLE, COMPILER-SPEC-AMBIGUITY, COMPILER-LOWERING,
COMPILER-IMPLEMENTATION, COMPILER-DIAGNOSTIC, COMPILER-PERFORMANCE or
COMPILER-PORTABILITY. Spec holes/ambiguities require a minimal example, Draft
17.4 section, relevant backend section, oracle behavior, formal evidence if
relevant and candidate resolutions. Pause only the affected feature; continue
unaffected infrastructure. Never edit normative/history snapshots to resolve
implementation pressure. No semantic feature is implemented in P0.
