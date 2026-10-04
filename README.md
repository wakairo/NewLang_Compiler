# NewLang Compiler

NewLang production compiler, written in C17 with LLVM as the primary backend.

**P0 bootstrap is complete and merged.** The repository is now in the pre-P1
foundation stage. It still does not compile NewLang source; substantial frontend
implementation begins only after the pre-P1 foundation is reviewed.

## Authority

In order: **N** Draft 17.4 (language semantics), **N** Backend Contract v0.4
(backend obligations), **A** M7 adjudication/closure, **O** frozen M7.5 Python
oracle, **F** F0 bridge / [NewLang_FormalProof](https://github.com/wakairo/NewLang_FormalProof),
**I** this C implementation. The Charter and Handoff Manifest are project
policy, not language semantics. All input snapshots are byte-preserved under
`docs/` and `docs/reference/`; `INPUT_ARTIFACTS.json` records their SHA-256 hashes.
M7 is closed. No M8+ semantics are anticipated here. Lean is not a dependency.

## Fresh setup

Supported hosts: Linux x86_64, Codex Cloud Debian 13 and Ubuntu 24.04 (including
WSL2 userspace). WSL2 itself has not been tested. Python >=3.12 is required.
Use the existing task checkout; tasks are already isolated. Do not create an
additional Git worktree unless explicitly requested.

On Ubuntu 24.04, install base prerequisites from its signed package repositories:

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends -y \
  gcc-13 g++-13 make cmake python3 dpkg \
  libstdc++6 libz3-4 libxml2 libpfm4 libedit2 libzstd1 libffi8
```

The Codex Debian 13 base already supplies GCC 14, make, Python, dpkg, and LLVM
runtime dependencies. Bootstrap supplies the missing Clang/LLVM development
tools, sanitizer runtimes, and CMake there without root privileges. Both hosts
use the tag-matching Trixie LLVM SDK in the private prefix; Ubuntu additionally
receives checksum-locked Z3 4.13.3. Its system `libstdc++6` must be >=14 (from
normal Ubuntu updates), independently of using GCC 13 as the C compiler.
Bootstrap checks that prerequisite; it never replaces system libc/libstdc++.

From the repository root:

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake --fresh -S . -B build-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-gcc --parallel 2
ctest --test-dir build-gcc --output-on-failure
./build-gcc/newlangc --version
```

Bootstrap downloads reviewed binary packages into ignored `.deps/`, verifies
each SHA-256, and extracts a content-addressed toolchain. Exact LLVM **23.1.2**
and OS-specific package versions/hashes live in `scripts/toolchain-lock.json`.
It does not build LLVM from source, run package maintainer scripts, or update
pins. Activation is needed in each new shell/task; processes are not retained.
The SDK source commit matches the official `llvmorg-23.1.2` tag. Upgrades require
a dedicated reviewed PR; ordinary bootstrap/build never change the release.
`cmake --fresh` refreshes compiler/LLVM discovery when upgrading an existing
build directory from LLVM 19; build/test commands themselves reuse valid outputs.
Network destinations: `deb.debian.org` (auxiliary OS packages), `apt.llvm.org` (LLVM SDK),
Ubuntu package mirrors (Ubuntu prerequisites), and GitHub / `api.github.com`
for repository/PR operations. No application credentials or services are needed.

The same locked LLVM toolchain provides `clang-format-23` 23.1.2. Mechanical
C/H formatting is repository policy:

```sh
bash scripts/format.sh        # rewrite tracked C/H files
bash scripts/check-format.sh  # verify without modifying
```

CI runs the non-modifying check.

## Other validated configurations

```sh
. .deps/activate.sh
CC=clang-23 cmake --fresh -S . -B build-clang -DCMAKE_BUILD_TYPE=Debug
cmake --build build-clang --parallel 2
ctest --test-dir build-clang --output-on-failure

CC=clang-23 cmake --fresh -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=address
cmake --build build-asan --parallel 2
ctest --test-dir build-asan --output-on-failure

CC=clang-23 cmake --fresh -S . -B build-ubsan -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=undefined
cmake --build build-ubsan --parallel 2
ctest --test-dir build-ubsan --output-on-failure
```

Each configuration runs the same **10 CTests**: diagnostic unit checks, six CLI
cases (each invoked twice), valid LLVM C API module/IR, artifact integrity, and
frozen-oracle smoke. ASan includes leak detection; UBSan stops on the first
failure. Imported LLVM binaries are not rebuilt with sanitizers; our C targets
are instrumented. Required checks are these four configurations and PR CI.
The historical 466-test oracle suite and formal proofs are optional evidence;
P0 does not claim to reproduce them in C.

## CLI contract

`--version` prints `newlangc 0.1.0 (P0 bootstrap)`. `--help` (also no arguments)
prints usage; both exit 0. Invalid options / argument counts exit 2. A source
path exits 3 with `P0-COMPILE-UNSUPPORTED`; no source is read and no output
artifact is produced. Output I/O failure exits 1.

## Layout and review

- `include/newlang/diagnostic.h`, `src/`: small borrowed-data diagnostic API and CLI.
- `tests/unit/`, `tests/integration/`: C unit / LLVM checks and CLI/artifact tests.
- `oracle/`, `tests/oracle/`: untouched M7.5 archive, identity, and isolated adapter.
- `scripts/`: reproducible bootstrap, dependency lock, formatter/fmt-check helpers.
- `.github/workflows/compiler-ci.yml`: PR-triggered GCC, Clang, ASan, UBSan validation plus
  pinned clang-format checking.
- `docs/NewLang_Aware_C_Guidelines.md`: production C implementation discipline.
- `docs/NewLang_Compiler_Testing_Strategy.md`: unit/integration/oracle/end-to-end test policy.
- `docs/NewLang_Compiler_Review_Guidelines.md`: shared review contract.
- `docs/PRE_P1_SOURCE_INPUT_AND_LOCATION_CONTRACT.md`: preserved-byte and canonical source-span
  foundation.
- `docs/PRE_P1_FOUNDATION_DECISIONS.md`: pre-P1 decisions and explicit deferrals.
- `docs/`: historical [P0 toolchain decisions](docs/P0_TOOLCHAIN_DECISIONS.md),
  [P0 architecture](docs/P0_ARCHITECTURE.md), and [P0 report](docs/P0_REPORT.md).

There is still no lexer/parser/type checker, checked IR, optimization framework, broad FFI,
concurrency, separate compilation, self-hosting, or speculative M8+ implementation. P1 scope
must be chosen and reviewed separately.
