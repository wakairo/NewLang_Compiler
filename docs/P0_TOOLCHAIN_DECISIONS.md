# P0 toolchain decisions

## C language standard

| Candidate | Benefit | Portability / support cost |
|---|---|---|
| C17 | Stable C baseline across GCC, Clang, Ubuntu 24.04 and WSL2; enough for P0 | No P0 feature needs a newer standard |
| C23 | New language conveniences and library additions | Compiler/library support differs; no concrete bootstrap benefit justifies it |

Decision: **C17**, required, compiler extensions disabled (`-std=c17`). Production
implementation is C, with synchronous borrowed-data interfaces and explicit
resource cleanup. No C++ frontend dependency or macro pseudo-language.

## Build system

| Candidate | P0 tradeoff |
|---|---|
| CMake | CTest, compiler/sanitizer configurations, imported LLVM C library, compile_commands.json and IDE support in common Linux/WSL/CI tools |
| Meson | Clear native testing/dependency model, but adds tooling with no P0 benefit over the available CMake ecosystem |
| plain Make | Small initial surface, but discovery, separate compiler builds, sanitizers, testing and IDE metadata would need custom orchestration |

Decision: **CMake >=3.28,<4.0**, CTest as the single test entry point, Unix
Makefiles default generator. Debian package version **3.31.6-2** (CMake 3.31.6)
is checksum-locked by bootstrap. Ubuntu uses its signed distro CMake package
(3.28.x policy), not an unpinned latest CMake release. Build parallelism 2 is
sufficient for four small C targets and keeps CI/resource use modest.

## LLVM 23.1.3 curated static SDK candidate (2026-10-15, Issue #449)

**Proposed for the full Ubuntu 24.04 five-job CI, subject to one actual first-attempt
Actions run and independent Coordination review; do not treat this DRAFT PR as an
already adopted main baseline.** Draft17.30, language C17 and Production C
semantics remain unchanged. This supersedes the **Ubuntu** acquisition path
described in the historical LLVM 23.1.2 review below; the Debian 13 legacy
package entry remains separate and is not used in Ubuntu Actions.

- **Exact original LLVM**: official `llvmorg-23.1.3` tag
  `0d261d1ca552c95a8f007e061c787ac7132fbcbc`, upstream signed
  `LLVM-23.1.3-Linux-X64.tar.zst` digest
  `14d2f701eb68fb799001bdea6231048f3990690fa2f406d563555ff8f744daba`,
  original release-manager GPG fingerprint
  `D574BD5D1D0E98895E3BF90044F2485E45D59042`.
- **Actual CI origin**: *unofficial NewLang curated* public GitHub **prerelease**
  `llvm-23.1.3-newlang-sdk-linux-x64-v1`, asset ID `642393404`,
  `newlang-llvm-23.1.3-linux-x64-final.tar.zst`, **126,520,077 bytes**,
  committed SHA-256
  `5369d292ef513d32758634f98ec7cf62c52f82a0a333eb330ecf9a325648b133`.
  Compressed asset expands to a pinned 448,040,960-byte tar. Lock contains the
  *exact* HTTPS tagged asset URL and the upstream identity. Its checksum is
  verified for **both Actions cache restores and direct GitHub downloads**,
  before any tar header is trusted. Cache is disposable; the published fixed
  Release is the enduring source. No mutable latest tags or fetch mirrors.
- **Provenance scope**: the upstream asset was GPG verified and user-local
  selected 496 SDK binaries/headers/libs/symlinks were identical after curation,
  but **the curated archive has no independent GPG signature**. The signed
  upstream file signature does not authenticate the curated bytes; the curated
  archive is authenticated against the exact committed **Release asset SHA**.
  `PROVENANCE.md` and `licenses/` are mandatory unpacked contents. Included
  LLVM/Clang/compiler-rt licenses, Unicode/OpenBSD regex/BLAKE3/xxHash/MD5 notices
  are part of a **bounded, non-exhaustive FOSS review, not legal certification**.
  The Unicode notice date discrepancy (README says Oct 14; retrieval Oct 15 JST)
  is disclosed in the original prerelease notes. No Source-to-SBOM claim.
- **Static-only LLVM C API**: the curated SDK has `libLLVMCore.a`, not
  `libLLVM-23.so`; `llvm-config-23 --shared-mode` must report `static`.
  The existing unchanged `llvm_smoke.c` and `llvm.c_api` CTest use
  `llvm-config --link-static --libs core analysis --system-libs`, with **host
  C++ linker only for that LLVM smoke**. Production remains C17, strict warnings
  and unchanged checked-owned/C17 emitter. Requires normal Ubuntu GCC13/G++13
  libstdc++ and the SDK's selected Clang/clang-format/compiler-rt resources.
  The correct SDK `llvm-config-23` must be under activated `NEWLANG_SDK_ROOT`;
  a random host/system LLVM executable is not an accepted fallback.
- **Fail closed**: Ubuntu 24.04 x86_64 only for curated mode, rootless temporary
  atomic extraction, verified cache/download bytes, explicit `zstd --long=30`
  decode with strict **448,040,960 B** decompressed tar size and a minimum
  3 GiB free disk preflight, safe tar member types/path/symlink/hardlink
  validation plus Python `tarfile.data_filter`. No privilege preservation,
  scripts, unsigned mirror or silent external-lib substitution. LLVM tools
  are version-checked and written into `.deps/activate.sh`.
- **Cache trust**: Actions restore a single exact archive via a pinned cache
  action, key `ubuntu-24.04-x86_64` plus the full archive SHA; no restore key,
  PR/fork saves or extracted-prefix caching. Cache publication is restricted
  to successful trusted `main` pushes. Cache hit **always** rehashes its
  restored archive and then uses the same safety gate as a cache miss.
- **Workload & alternatives**: 126.5 MB per cold job, 632.6 MB across five
  independent cold runners; ~427 MiB unpacked per job, vs original upstream
  1.19 GB / ~12 GB and ~5.94 GB across cold five jobs. The old 23.1.2
  twelve-package set was ~140 MB but original apt.llvm.org URLs recurrently
  failed HTTP404. An unmerged NO LLVM/PARTIAL source/C17 job cannot replace
  five-way LLVM C API/sanitizer/native evidence. Actual runner/cache timings
  and acceptance remain to be measured, not predicted success.
- **Stop / ownership**: first SHA, tar, SDK version/layout, C++ closure,
  Clang warning, sanitizer, native/CTest or ABI failure stops the one candidate
  attempt. PR stays DRAFT pending Coordination independent review; no automatic
  PR438 merge/R437 start, Production soundness or Product GO.

## Historical LLVM baseline re-review (2026-10-04; LLVM 23.1.2)


**Historical selection: exact stable LLVM 23.1.2**, Clang/compiler-rt 23.1.2, C API only.
The original 19.1.7 rationale compared only older releases and was insufficient
for a new compiler in 2026. This dedicated review supersedes that selection.
P0 code has no semantic lowering or large LLVM surface to migrate, so avoiding
an unnecessary later migration is useful now, subject to actual compatibility
and reproducibility checks rather than assuming newer is safer.

### Candidates evaluated

| Release | Official stable publication | Availability and compatibility evidence | Decision |
|---|---|---|---|
| 19.1.7 | 2025-01-14 | Original signed/checksum-locked packages, LLVM C headers/shared library/config and full P0 matrix work | Reject as the new baseline: no required capability or host advantage over tested 23; outside apt.llvm.org's stated last-two-releases maintenance focus; adds a later migration across already-known C API changes |
| 22.1.8 | 2026-06-16 | Signed Noble/Trixie indexes contain all ten SDK packages; 24 representative C declarations present; unchanged P0 smoke compiled/linked/verified against Noble 22.1.8 on Cloud | Valid fallback, rejected because 23 also meets the requirements with no P0 API adjustment, and starts on the current stable major with a longer relative maintenance horizon |
| 23.1.2 | 2026-09-22 | Official latest non-prerelease at review time; signed indexes contain all ten SDK packages; 24 representative C declarations present; tag-matching SDK and unchanged smoke verified on Cloud | Selected: GCC/Clang/ASan/UBSan and fresh Ubuntu PR CI all pass |

Selection criteria: supported C API, exact stable source identity, complete
reviewable package set, Linux x86_64/Ubuntu 24.04 runtime compatibility, rootless
extraction, signed metadata/checksums, sanitizer behavior and future maintenance.
20/21 need not be additional candidates: 22 is an available newer fallback and
23 passes the P0 API checks. There is no technical requirement uniquely favoring
19 over those intermediate majors. No blanket LTS guarantee or calendar support
end date is claimed for any LLVM major.

Sources: official releases [19.1.7](https://github.com/llvm/llvm-project/releases/tag/llvmorg-19.1.7),
[22.1.8](https://github.com/llvm/llvm-project/releases/tag/llvmorg-22.1.8),
[23.1.2](https://github.com/llvm/llvm-project/releases/tag/llvmorg-23.1.2),
and [apt.llvm.org package/support policy](https://apt.llvm.org/).
The apt site has stale stable/qualification/development labels; release status
comes from the official releases, package identities from signed indexes, and
compatibility from executing binaries. Transient proxy 403s were not treated as
proof that a package/version does not exist.

### Exact package/source identity and rootless hosts

Both host entries use the same **Trixie SDK** package version:
`1:23.1.2~++20260920034005+85ac56026243-1~exp1~20260920034024.76`.
Its upstream source revision **85ac560262434c9ccfc0c183ec22d4138ed647fb** matches
`llvmorg-23.1.2`, confirmed through the official GitHub commits API. A Debian
packaging rebuild is not claimed to be bit-identical to the official tarball.

The ten artifacts are clang-23, libclang-common-23-dev, libclang-cpp23,
libclang-rt-23-dev, libclang1-23, libllvm23, llvm-23, llvm-23-dev,
llvm-23-runtime and llvm-23-linker-tools. They provide Core.h, Analysis.h,
llvm-config-23, shared libLLVM, compiler/headers and sanitizer runtimes.

Why this SDK for Ubuntu: the signed Noble 23.1.2 build uses source
`4b19252104764e9e2ccd25c545eb6b99309a2444`, the release tag's immediate parent.
It lacks the final Clang bodyless-destructor/MSVC fix (#218830). That fix does
not affect our C smoke, but a version string alone is insufficient evidence of
the exact stable source release. We therefore do not adopt that pre-tag build.
Noble 22.1.8 uses its actual tag revision `ca7933e47d3a3451d81e72ac174dcb5aa28b59d1`
and remains the fallback if the 23 runtime route fails validation.

The Trixie SDK requires glibc >=2.38, libstdc++ >=12 and Z3 >=4.13.3. Cloud
Debian 13 supplies these. Ubuntu 24.04 supplies glibc 2.39 and the remaining
libedit/libffi/libxml2/libzstd/zlib/libpfm runtimes. Bootstrap additionally locks
Debian **libz3-4 4.13.3-1** into the private Ubuntu prefix; its
**GLIBCXX_3.4.32** dependency requires Ubuntu's **libstdc++6 >=14**, installed
from signed standard Ubuntu updates and checked before extraction. GCC 13 is
still the CI C compiler. The C++ runtime dependency comes from LLVM/Z3 internals,
not a C++ production interface. No system libc/libstdc++ or LLVM installation
is replaced, no maintainer script runs, and no source build is needed.

This is a deliberate Linux userspace ABI reuse decision, verified by fresh
Ubuntu CI, rather than an assumption that arbitrary Debian packages work on
Ubuntu. Its extra cost is one checksum-locked Z3 artifact and an explicit system
runtime prerequisite. WSL2 Ubuntu 24.04 has the same userspace prerequisites;
native WSL2 execution remains untested. Fail the bootstrap on older runtimes
instead of substituting artifacts, weakening checks or changing the release.

`scripts/toolchain-lock.json` pins every package version/URL/SHA-256 and source
revision. Existing Debian CMake/libpfm/librhash/libuv pins are preserved.
Signature-verified trixie APT metadata supplies the auxiliary package hashes.
SDK Release.gpg is verified with fingerprint
`6084F3CF814B57C1CF12EFD515CF4D18AF4F7421`, then the exact Packages index is
verified against its signed SHA-256. Trixie 23 and comparison Noble 22/23
metadata are retained under `scripts/toolchain-provenance/`, together with the
public key and review identity. Ordinary bootstrap requires TLS and SHA-256
for all downloads, without signature/checksum bypasses or pin rewriting.

### C API maturity and churn

| Surface | Review result / risk |
|---|---|
| Context/module, IRBuilder, scalar types, functions/basic blocks, verification | Required smoke APIs exist in all three; 22 and 23 run the unchanged source. Explicit-context APIs already used avoid LLVM 22's deprecated global-context functions |
| Attributes and metadata | Enum attribute name lookup/create/add and context-aware metadata APIs remain available; names, parameter contracts and LLVM IR meanings still require version-specific review and proof |
| Target machine / object emission | TargetMachine.h, LLVMCreateTargetMachine and LLVMTargetMachineEmitToFile available in all three; emission is not implemented or functionally tested in P0 |
| Debug information | DIBuilder APIs available; higher churn risk due to debug records/format evolution; no P0 debug-info claim |
| Branch inspection | LLVM 23 splits LLVMBr into LLVMUncondBr/LLVMCondBr and changes CondBr operand order. P0 uses neither branch opcode dispatch nor raw branch operands; future lowering must use successor APIs and reviewed tests |

LLVM's [C API policy](https://github.com/llvm/llvm-project/blob/llvmorg-23.1.2/llvm/docs/DeveloperPolicy.md#c-api-changes)
is best-effort stability, with release-branch stability, not a permanent ABI
promise. [LLVM 22 notes](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.8/llvm/docs/ReleaseNotes.md)
and [LLVM 23 notes](https://github.com/llvm/llvm-project/blob/llvmorg-23.1.2/llvm/docs/ReleaseNotes.md)
make these changes reviewable. No ifdef framework or compatibility layer is
introduced. The existing LLVM smoke source is unchanged.

### M7 contract compatibility

The release-tag [LLVM 23 LangRef](https://github.com/llvm/llvm-project/blob/llvmorg-23.1.2/llvm/docs/LangRef.md)
retains noalias dynamic-access obligations, proof-sensitive alias metadata,
conservative omission of unproved promises and stack-only lifetime intrinsic
operands (alloca/structured.alloca). It does not force source write/exclusivity
into noalias, or semantic object lifetime into LLVM lifetime. Native layout
opacity and backend-owned target ABI classification remain NewLang obligations
independent of LLVM version. No M7 constraint is weakened; Draft 17.4 and the
Backend Contract snapshots remain unchanged. Future attribute/debug/object
emission behavior is not established by this scalar P0 smoke.

### Upgrade policy

**LLVM baseline = a reviewed exact stable release.** Ordinary bootstrap and
ordinary builds never change the version or rewrite pins. Upgrades require a
**dedicated reviewed PR** with source/package identity, signed metadata, exact
URLs/hashes, C API/contract review, all four validation configurations, and
PR-triggered CI green at the current head. Never follow `latest`, unversioned
meta packages, a development branch or an automatic version-discovery script.
This explicitly requested baseline review is performed within the existing
unmerged P0 PR; later upgrades use separate dedicated PRs.

LLVM links via llvm-config-23 paths to its shared C API library. CMake rejects
every version except 23.1.2. The compiler CLI remains backend-independent.

## Host policy and external prerequisites

GCC major 13 (Ubuntu CI) and 14 (Debian Cloud) are the validation baseline.
Clang is package-pinned 23.1.2. All project C targets use
`-Wall -Wextra -Wpedantic -Werror`; no speculative compiler-specific warnings.
Separate Clang address and undefined sanitizer builds use
`-fno-omit-frame-pointer -fno-sanitize-recover=all` and halt-on-error runtimes.

Python **>=3.12** executes standard-library test adapters and the M7.5 checker.
The archive contains its own `vendor/newlang_checker`; no pip installation is
needed. Oracle project metadata says `newlang-frontend-m7` **0.7.0**, while the
snapshot milestone is **M7.5**. Both identities are recorded without modifying
the archive. Pytest is needed only for the optional historical full suite.

System libc, libstdc++, libz3, libxml2, libedit, libffi, libzstd, zlib and libpfm
are host-runtime dependencies, installed from signed OS repositories. Host GCC,
make, Python, and these OS libraries follow distro security updates; exact
observed versions are reported per validation run. This is reproducible setup
with exact LLVM artifacts, not a bit-for-bit OS/image claim. Linux x86_64 is P0's
only target. Native WSL2 execution, other architectures and cross-compilation
remain unvalidated. Lean is deliberately absent from build dependencies.
