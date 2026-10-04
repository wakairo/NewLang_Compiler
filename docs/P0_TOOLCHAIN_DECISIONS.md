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

## LLVM

Exact upstream **19.1.7**, including LLVM C headers, shared library, llvm-config,
Clang 19.1.7 and compiler-rt sanitizer runtimes. Mature packaged release on both
supported Linux baselines; no expensive source build required. LLVM 18 is also
available on Ubuntu, but would require a different Debian package source;
19.1.7 provides one explicitly validated upstream baseline across both hosts.
No dependency on newer capture attribute spellings is encoded in P0.

`scripts/toolchain-lock.json` contains exact package versions, immutable artifact
URLs and SHA-256 hashes for Debian 13 and Ubuntu 24.04 x86_64. Debian LLVM packages
are **1:19.1.7-3+b1**. Ubuntu LLVM packages are
**1:19.1.7~++20250804090312+cd708029e0b2-1~exp1~20250804210325.79**.
These are distinct packaging builds of upstream 19.1.7, not bit-identical LLVM.

Pin provenance: Debian APT verified the signed trixie InRelease and package
checksums before download. Ubuntu apt.llvm.org Release.gpg was verified using
key fingerprint `6084F3CF814B57C1CF12EFD515CF4D18AF4F7421`; its Packages.gz hash
was checked against that signed Release. The latter small evidence files are
retained under `scripts/toolchain-provenance/`. Bootstrap preserves TLS and
checks every artifact against the committed hash; there is no insecure mode.
Expired/removed artifacts fail clearly rather than changing pins. Ordinary
configure/build/bootstrap never discovers or rewrites new versions.

LLVM is linked via `llvm-config-19` include/lib paths to its **shared C API**
library. CMake rejects every LLVM version except 19.1.7. The compiler executable
remains backend-independent; only the dedicated LLVM smoke test uses LLVM in P0.
LLVM's implementation is C++, but our interface and compilation language are C.

## Host policy and external prerequisites

GCC major 13 (Ubuntu CI) and 14 (Debian Cloud) are the validation baseline.
Clang is package-pinned 19.1.7. All project C targets use
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
