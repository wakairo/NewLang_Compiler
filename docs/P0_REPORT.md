# P0 — Production Compiler Bootstrap report

Status: LLVM 23.1.2 local validation and fresh Ubuntu PR CI **green**.
P0 review gate only. P1 work is not authorized by this milestone.

## 1. Repository, branch, PR

- Repository: https://github.com/wakairo/NewLang_Compiler (existing empty remote).
- Minimal seed `18d2107` creates main with README/.gitignore only.
- Production work: `p0-bootstrap` -> `main`; [PR #1](https://github.com/wakairo/NewLang_Compiler/pull/1)
  is open, ready for review. No production work is committed directly to main.
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
C17 and CMake/CTest selected; extensions disabled. LLVM C API **23.1.2** and
Clang **23.1.2** pinned by exact package versions, URLs and SHA-256.
Signature-verified metadata was used to create the lock. Ordinary builds cannot
change pins. Bootstrap is rootless on the supplied Debian 13 Cloud base.

Observed Cloud host: Debian GNU/Linux 13, Linux x86_64, **GCC 14.2.0**,
**Clang 23.1.2**, **LLVM 23.1.2**, **CMake 3.31.6**, **GNU Make 4.4.1**,
**Python 3.12.14**. Updated Ubuntu 24.04 CI selects GCC 13 and the same pinned
Clang / LLVM 23.1.2; all four jobs succeed in the run recorded below.
Host OS runtimes follow signed distro packages; this is not bit-identical OS
pinning. WSL2 execution is not independently validated.

### LLVM baseline additional review — 2026-10-04

Evaluated **19.1.7, 22.1.8, 23.1.2**, confirming official stable releases,
signed package indexes, complete SDK artifacts, representative C declarations,
release/API policy, and M7 compatibility. Final selection: **23.1.2**. The
older initial selection lacked a comparison against current stable LLVM; it
has been superseded, not retained merely because it already worked.

23 provides the required C API without any smoke-source adjustment. Its known
branch opcode/operand churn does not affect P0. 22 also runs the unchanged
smoke and is the valid fallback; 19 has no P0 host/API advantage over 23 and
would incur a later migration. Detailed comparison, rejection reasons, support
limits and dedicated reviewed upgrade policy are in `P0_TOOLCHAIN_DECISIONS.md`.

Source/package identity matters: Noble's 23.1.2 packaging build is the release
tag's immediate parent; selected Trixie SDK source revision
`85ac560262434c9ccfc0c183ec22d4138ed647fb` matches `llvmorg-23.1.2` exactly.
Both hosts use that same SDK in a private prefix. Ubuntu also receives locked
Z3 4.13.3 and must provide system libstdc++6 >=14 from signed Ubuntu updates.
SDK headers/shared library/config/Clang/sanitizer artifacts remain complete;
TLS, signed metadata provenance, per-artifact SHA-256 and rootless extraction
are preserved. No system runtime replacement, source build or compatibility
layer is introduced. Ordinary bootstrap/build never discover or update pins.

`llvm-config-23 --version` prints **23.1.2**. GCC, Clang 23, ASan and UBSan were
rebuilt with fresh CMake dependency/compiler discovery to remove the old LLVM 19
cache. All **40/40 CTest executions PASS** with the existing ten-test suite.
All normative/policy/oracle snapshot hashes still match; tests and production
C source are unchanged by this review. Bootstrap repeated successfully without
changing pins. Fresh Ubuntu PR-triggered GCC/Clang/ASan/UBSan all pass. CMake
caches/compile commands confirm the LLVM 23 link target, strict C17 flags and
the intended sanitizer flags, so the old LLVM 19 outputs are not reused as
evidence. The existing Debian auxiliary tool pins are unchanged.

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

`p0.yml` requires PR-triggered Ubuntu 24.04 jobs for GCC 13, Clang 23.1.2,
ASan and UBSan. Each starts from checkout, installs explicit base prerequisites
from signed Ubuntu repositories, downloads checksum-locked LLVM binaries,
configures/builds C, and runs all 10 CTests. Checkout action pinned to verified
v4.2.2 commit `11bd71901bbe5b1630ceea73d27597364c9af683`. No auto-merge action.
The initial LLVM 19 [run 37196049092](https://github.com/wakairo/NewLang_Compiler/actions/runs/37196049092)
is historical evidence only, not validation of the new baseline. LLVM 23
PR-triggered [run 37198087232](https://github.com/wakairo/NewLang_Compiler/actions/runs/37198087232)
for baseline implementation commit `8349940ba6b0e4857b82faa255a5a7ab6b90c94b`
completed **success** with all four **GCC, Clang, ASan, UBSan** jobs and all
bootstrap/configure/build/test steps successful (GitHub Actions API evidence).
Each job runs all ten CTests. Subsequent report-only revisions also require
their own current-head PR CI; the PR check panel and final PR description
record that latest commit/run rather than treating this earlier run as enough.

Reproduce from repository root with README commands:
`python3 scripts/bootstrap.py`, `. .deps/activate.sh`, CMake `--fresh` configure/build,
`ctest --test-dir build-gcc --output-on-failure`. Separate build directories
select GCC/Clang/address/undefined. Dependencies remain in filesystem snapshots;
activation must run again in future shells. No long-running service is needed.
Cloud `install_script` and `start_skill` were refreshed for LLVM 23.1.2 in the
environment draft and
the installation commands exercised successfully. They cover working directory,
bootstrap, per-shell activation, GCC build/10-test readiness, optional validation
configurations and use of the existing isolated checkout. Saving is not
publication or validation in a newly restored Codex task. Review/save and publish
remain user actions in environment settings. No application secret is required.

## 17. COMPILER-* findings

No COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY discovered or silently encoded:
P0 has no source-semantic implementation. COMPILER-IMPLEMENTATION decisions are
the toolchain selection, borrowed diagnostic records and archive-based oracle.
COMPILER-PORTABILITY limits: Linux x86_64 baseline only; WSL2/native other-host
validation remains future work. Additional review findings:

- **COMPILER-PORTABILITY:** reported 23.1.2 was not enough to establish exact
  stable source identity for the Noble package. Resolved using tag-matching
  Trixie SDK plus a locked private Z3 runtime and checked Ubuntu C++ runtime.
  Fresh Ubuntu PR CI verifies this userspace ABI reuse in all four jobs.
- **COMPILER-IMPLEMENTATION:** initial version rationale omitted current stable
  candidates; replaced with explicit release/package/API/contract comparison
  and dedicated reviewed upgrade policy. LLVM 23 branch opcode changes are
  recorded for future review, not implemented speculatively.
- No COMPILER-LOWERING, SPEC-HOLE or SPEC-AMBIGUITY requiring NewLang contract
  changes was discovered. Draft 17.4 and Backend Contract v0.4 are unchanged.

## 18–19. Non-goals and P1 assessment

No full lexer, parser, AST semantics, type checker, dependency/value-flow/ownership
engine, generics, aggregates/sums, dynamic container bridge, broad FFI, persistent
function pointers, concurrency, separate compilation, optimizer, self-hosting,
embedded/bare-metal or M8+ speculative semantics. No IR hierarchy or LLVM facts
are inferred from source capabilities. M7 constraints are documented intact.

P0 is suitable for architecture review once PR CI is green. P1 may be scoped
after human/ChatGPT review, not automatically started. This task stops at P0.

## Completion gate evidence

User P0 criteria **A–AB are met with the reviewed LLVM 23.1.2 baseline**:
dedicated repository and committed references (A–D), C/reproducible toolchain
(E–I), GCC/Clang/strict warnings/sanitizers (J–N), CLI/diagnostics/LLVM/test/oracle
infrastructure (O–T), green PR-triggered CI and open unmerged branch PR (U–W),
and all non-goal/scope/semantic-review boundaries (X–AB).

Updated LLVM 23 fresh Ubuntu bootstrap/CI and current Cloud setup are verified;
publication and restoration in a new Cloud task are not claimed. WSL2 execution
and the full 466-test oracle suite remain optional unrun checks. P1 requires
review approval and a separately chosen scope; this task has stopped at P0.

The additional LLVM baseline review is complete. P0 is technically ready for
merge once the final report revision's current-head PR CI is green and the
reviewer accepts this selection. Codex does not merge or begin P1. Normative
documents, oracle, semantic implementation and P0 scope remain unchanged.
