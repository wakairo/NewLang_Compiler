# P0 — Production Compiler Bootstrap report

Status: local validation green; PR/fresh-runner CI evidence pending below.
P0 review gate only. P1 work is not authorized by this milestone.

## 1. Repository, branch, PR

- Repository: https://github.com/wakairo/NewLang_Compiler (existing empty remote).
- Minimal seed `18d2107` creates main with README/.gitignore only.
- Production work: `p0-bootstrap` -> `main`; PR link and final CI evidence will
  be recorded after creation. No production work is committed directly to main.
- PR remains open and must not be self-merged.

## 2–3. Artifacts and authority

All attachment hashes verified before use. Required reading order followed:
Charter, Handoff Manifest, Draft 17.4 (P0-relevant scope/status/backend boundary),
Backend Contract v0.4, M7 adjudication, M7 closure, then M7.5 archive inspection.
Full language feature review is deferred until the corresponding semantic work.
M7 Closure MANIFEST.json is preserved. F0 Draft0 / formal-track pointer are
retained as Class F evidence; no live Lean proof is relied on in this milestone.

Authority: N language Draft 17.4 > N backend contract v0.4 > A M7 closure > O
M7.5 oracle > F formal evidence > I C implementation. Project Charter/Manifest
govern scope only. All versioned inputs and archive hashes appear in
`docs/reference/INPUT_ARTIFACTS.json`; originals have not been rewritten.

## 4–7. Toolchain

See `P0_TOOLCHAIN_DECISIONS.md` for C17 vs C23 and CMake vs Meson vs Make.
C17 and CMake/CTest selected; extensions disabled. LLVM C API **19.1.7** and
Clang **19.1.7** pinned by exact OS-specific package versions, URLs and SHA-256.
Signature-verified metadata was used to create the lock. Ordinary builds cannot
change pins. Bootstrap is rootless on the supplied Debian 13 Cloud base.

Observed Cloud host: Debian GNU/Linux 13, Linux x86_64, **GCC 14.2.0**,
**Clang 19.1.7**, **LLVM 19.1.7**, **CMake 3.31.6**, **GNU Make 4.4.1**,
**Python 3.12.14**. GCC 13 / Ubuntu 24.04 fresh CI evidence is recorded later.
Host OS runtimes follow signed distro packages; this is not bit-identical OS
pinning. WSL2 execution is not independently validated.

## 8–10. Structure, executable and diagnostics

Small `src/main.c` and `src/diagnostic.c` plus one public header; no prebuilt
frontend/backend pass hierarchy. `tests/{unit,integration,oracle}`, `oracle/`,
`scripts/`, `docs/`, and one Actions workflow form the development loop.

`newlangc --version` -> `newlangc 0.1.0 (P0 bootstrap)` (exit 0).
Help/no arguments exit 0; invalid option/argument count exit 2; source path
returns `P0-COMPILE-UNSUPPORTED` (exit 3) without reading/compiling the source.
CLI output is checked exactly and repeated per case to validate determinism.

Diagnostic records carry severity, optional category/code, message, optional
range and notes. All input pointers are borrowed, nothing retained/allocated;
validation precedes output, errors return false. A synchronous FILE renderer
keeps direct printing out of future semantic subsystems. See ownership contracts
in the header and `P0_ARCHITECTURE.md`.

## 11–14. LLVM, oracle, harness, sanitizers

LLVM smoke creates/disposes context/module/builder, builds an i32-returning
function, verifies the module and confirms `ret i32 42` IR. **PASS** in all four
Cloud configurations. It is not a NewLang lowering or machine-code test.

Oracle strategy: commit untouched ZIP and identity; extract into a checked
temporary directory per invocation. M7.5 project version 0.7.0, Python >=3.12,
bundled checker vendor, no P0 pip dependencies. Four actual historical fixtures
(three accept, one reject E_PRIVATE_FIELD) and the real CLI pass. Differential
adapter explicitly rejects the unsupported C result rather than claiming
semantic agreement. The 466-test historical oracle suite is not rerun by P0.

Same **10 CTests** execute in GCC, Clang, Clang-ASan and Clang-UBSan builds:
diagnostics.unit (10 unconditional checks), llvm.c_api, six CLI cases,
artifacts.integrity (10 frozen snapshots), oracle.smoke. All **40/40 Cloud
CTest executions PASS**, none skipped/disabled/expected-failure. Strict warnings
are active in every C target. ASan leak detection and UBSan halt-on-error pass;
third-party shared LLVM itself is not sanitizer-instrumented.

Bootstrap repeated successfully without changing pins/repository files.
Downloaded artifacts were SHA-256 checked; the bootstrap download path was
exercised directly. Dependency caches and outputs are ignored local state.

## 15–16. CI and reproducibility

`p0.yml` requires PR-triggered Ubuntu 24.04 jobs for GCC 13, Clang 19.1.7,
ASan and UBSan. Each starts from checkout, installs explicit base prerequisites
from signed Ubuntu repositories, downloads checksum-locked LLVM binaries,
configures/builds C, and runs all 10 CTests. Checkout action pinned to verified
v4.2.2 commit `11bd71901bbe5b1630ceea73d27597364c9af683`. No auto-merge action.
Fresh-runner CI has not yet been claimed; result/URLs will be recorded below.

Reproduce from repository root with README commands:
`python3 scripts/bootstrap.py`, `. .deps/activate.sh`, CMake configure/build,
`ctest --test-dir build-gcc --output-on-failure`. Separate build directories
select GCC/Clang/address/undefined. Dependencies remain in filesystem snapshots;
activation must run again in future shells. No long-running service is needed.
Cloud install/start instructions are saved separately in environment settings.

## 17. COMPILER-* findings

No COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY discovered or silently encoded:
P0 has no source-semantic implementation. COMPILER-IMPLEMENTATION decisions are
the toolchain selection, borrowed diagnostic records and archive-based oracle.
COMPILER-PORTABILITY limits: Linux x86_64 baseline only; WSL2/native other-host
validation remains future work. Rootless extraction replaces privileged system
installation on Cloud while preserving artifact verification.

## 18–19. Non-goals and P1 assessment

No full lexer, parser, AST semantics, type checker, dependency/value-flow/ownership
engine, generics, aggregates/sums, dynamic container bridge, broad FFI, persistent
function pointers, concurrency, separate compilation, optimizer, self-hosting,
embedded/bare-metal or M8+ speculative semantics. No IR hierarchy or LLVM facts
are inferred from source capabilities. M7 constraints are documented intact.

P0 is suitable for architecture review once PR CI is green. P1 may be scoped
after human/ChatGPT review, not automatically started. This task stops at P0.

## Completion gate evidence

Local implementation/toolchain criteria B–T and scope criteria X–AB are met.
Dedicated existing repository A is available. PR/CI criteria U–W will be recorded
after the actual PR and PR-triggered checks; no merge is authorized.
