# P440: production source checking without LLVM

Authority: `c6771a029062292c915ac2b8f5e66045661eec3f`, CURRENT_SPEC Draft17.30.
Branch: `p440-frontend-only-check`. DRAFT / DO NOT MERGE; Coordination reviews
integration. This is developer usability and checking capability, not Product
PASS, native execution evidence, or a replacement for the five-job gate.

## Preflight and bounded change

The original CMake LLVM references were discovery/exact-version/import at
lines 32–51 and `llvm_smoke` at 228–230. Only that smoke includes LLVM C headers.
`newlangc` links the C17 checked-C library, semantic/checked/parser,
syntax/lexer/source/diagnostic libraries; no LLVM symbols enter this graph.
The pre-edit GO was recorded on Issue440 before modifying code.

`--check` uses the existing source load, function-unit parse/registration,
ordinary `main()` expression check and owned-fragment cleanup. It returns only
after semantic success and before any emitter admission/output. Error rendering
and destruction order stay the same. There is no export/serialization of the
artifact and no new proof authority. Help/version/no-argument output remains
byte-identical as requested; README documents the additional invocation.

`NEWLANG_FRONTEND_ONLY` defaults OFF. ON skips only LLVM discovery/import and
the LLVM C API smoke. Native C17 emitter libraries and existing native tests
remain because they do not use LLVM. No public API, semantic checker, Node
proof/validator, bootstrap, lock or ordinary workflow changed. PR438's Node
native candidate is not part of this branch.

Historical audit §4.2: N/A, build/CLI separation of existing selected semantics.
KEEP DI-001/006/007 and adopted ownership rules; no Ledger update. Compass →
Product Strategy §§1–6, Roadmap §4, Process §§4.3/4.3.1: checking remains separate
from codegen/native/product evidence. Waiting for dependency recovery is an
alternative, but does not enable development/checking during external failure.
Any hidden LLVM dependency or changed semantic admission was the first falsifier;
neither was observed. No goal-alignment concern requiring new semantics arose.

## Actual source boundaries

Exact bytes, SHA256, exits, diagnostic spans, stdout sizes/hashes are in
[`evidence/p440/check_results.json`](evidence/p440/check_results.json).
Temporary sources are exactly the strings in `check_cli_test.py`.

| Actual input | Ordinary unchanged/candidate | `--check` | First boundary |
| --- | --- | --- | --- |
| Unmodified `node_join_same_locator.nl` | 4, C0 | 0, stdout0/stderr0 | backend only |
| `let x=u8(7);x;unit` | 0, C277 bytes | 0, stdout0/stderr0 | checked/emittable |
| `let x=;` | 3, C0 | same | parser `P5-EXPECTED-EXPRESSION` |
| `u8(256)` | 3, C0 | same | semantic `V1-U8-LITERAL-RANGE` |
| `fn bad()->u8{unit}` | 3, C0 | same | semantic `P8-BODY-RESULT` |
| escaping scoped write ref | 3, C0 | same | semantic `P8-EXIT-DEPENDENCY` |
| `u8(7)+u8(2)` | 3, C0 | same | syntax `P5-EXPECTED-ITEM-END` |
| unit without `main` | 3, C0 | same | entry `P3-UNKNOWN-CALLEE` |

Missing file stays exit1/`V0-SOURCE-IO`; check-option misuse stays exit2.
Tests invoke each source repeatedly, compare invalid-source diagnostics to the
ordinary driver, and exercise cleanup under the configured sanitizer. Existing
C unit tests retain shared-context transactional rollback and OOM injection;
separate CLI processes are not a proof of shared-context rollback.

## Reproduction and measured local scope

```sh
CC=gcc cmake -S . -B build-frontend -DCMAKE_BUILD_TYPE=Debug \
  -DNEWLANG_FRONTEND_ONLY=ON
cmake --build build-frontend --parallel 2
ctest --test-dir build-frontend --output-on-failure
build-frontend/newlangc --check tests/fixtures/node_join_same_locator.nl
```

Debian13 x86_64: GCC14.2.0-19, CMake/CTest3.31.6, Python3.12.14.
Fresh no-LLVM-tool build: **275/275 PASS, 52.62s**. Default full build with
the preexisting reviewed LLVM23.1.2 prefix: **276/276 PASS, 53.14s**.
Clang23.1.2 frontend-only build: **275/275 PASS, 60.88s**.
Baseline has274 tests; add2 focused tests, omit only `llvm.c_api` in opt-in mode.
Inventory comparison explicitly confirmed this sole difference. Source/checker,
negative semantics, OOM/rollback and public Node validator tests are retained.

This host already had LLVM in a different prefix; it is not claimed to be
offline or globally LLVM-free. For the fresh no-LLVM build, only preexisting
`cmake`, `ctest`, CMake data, `librhash` and `libuv` were copied to
`/tmp/p440-host`, with PATH `/tmp/p440-host/bin:/usr/bin:/bin`,
LD_LIBRARY_PATH `/tmp/p440-host/lib`, explicit system GCC and Python.
No LLVM executable/header/library is in that tools prefix; no bootstrap or
network command is part of configure/build/test. The new compiler's `ldd`
contains only libc/loader and its link command contains only project C archives.

`frontend_build_policy_test.py` additionally supplies a wrong-version
`llvm-config-23` with an invocation marker: frontend ON never invokes it;
default OFF rejects22.1.8. With discovery paths disabled, default OFF rejects
missing `LLVM_CONFIG`. The existing exact23.1.2 check has not been relaxed.

The ordinary five-job PR CI remains unchanged and mandatory. Its actual run
status is reported separately in the PR/Issue; local frontend success cannot
hide locked-package acquisition failures or constitute a full CI PASS.
