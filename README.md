# NewLang Compiler

NewLang production compiler, written in C17 with LLVM as the primary backend.

**P0 through P2 are complete and merged.** P3 adds a bounded semantic fragment
checker over P2 types, expressions, single bindings and loan headers: explicit
context/package/root state, nine core operations, exact registered calls and
transactional failure handling. Loan bodies remain opaque. This is not a full
program checker or end-to-end compilation. P3 stops for review before P4.

## Authority

In order: **N** the canonical Draft selected by `docs/reference/CURRENT_SPEC.md`
(currently Draft 17.6), **N** Backend Contract v0.4 (backend obligations),
**A** adjudicated M8 surface decisions, then M7 closure,
**F** F0 bridge / [NewLang_FormalProof](https://github.com/wakairo/NewLang_FormalProof),
**O** frozen M7.5 Python oracle,
**I** this C implementation. The Charter, Handoff Manifest and project process
documents are policy/evidence, not language semantics. P3 is synchronized with
current `main`; Draft 17.6 preserves Draft 17.5's exclusive-reborrow clarification
and adds M8.3R raw-byte semantics outside the P3 implementation scope.
M7 is closed. P2/P3 implement only the explicit M8.1/M8.2 subset selected by
the P3 handoff; no M8.3/M8.3R implementation is claimed. Lean is not a dependency. See the
[P2 grammar audit](docs/P2_MINIMAL_SYNTAX_CONTRACT.md) and
[P3 semantic contract / exclusive reborrow clarification](docs/P3_SEMANTIC_SLICE_CONTRACT.md).

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

Each configuration runs the same **27 CTests**: source and lexer module unit
tests, six parser unit groups, file-source -> lexer/parser integration,
six semantic unit groups and source -> parser -> checker integration,
diagnostic unit checks, six CLI
cases (each invoked twice), valid LLVM C API module/IR, artifact integrity, and
frozen-oracle smoke. ASan includes leak detection; UBSan stops on the first
failure. Imported LLVM binaries are not rebuilt with sanitizers; our C targets
are instrumented. Required checks are these four configurations and PR CI.
GCC Release/NDEBUG is also validated locally using the same CTest suite (see
[P3 report](docs/P3_FIRST_SEMANTIC_VERTICAL_SLICE_REPORT.md)).
The historical 466-test oracle suite and formal proofs are optional evidence;
P0 does not claim to reproduce them in C.

## CLI contract

`--version` prints `newlangc 0.1.0 (P0 bootstrap)`. `--help` (also no arguments)
prints usage; both exit 0. Invalid options / argument counts exit 2. A source
path exits 3 with `P0-COMPILE-UNSUPPORTED`; no source is read and no output
artifact is produced. Output I/O failure exits 1.

## Layout and review

- `include/newlang/diagnostic.h`, `src/`: small borrowed-data diagnostic API and CLI.
- `include/newlang/{source,token,lexer}.h`, `src/{source,lexer}.c`: immutable
  owned source bytes/name, checked half-open spans, non-owning atom tokens and
  allocation-free streaming lexer. Keyword/comment/encoding rules remain open.
- `include/newlang/{syntax,parser}.h`, `src/{syntax,parser}.c`: owned immutable
  syntax trees borrowing source, one-token lookahead, checked failure cleanup,
  structured first diagnostics and four standalone fragment entries. Loan
  bodies retain balanced byte regions; their contents are not parsed.
- `include/newlang/{semantic,checked}.h`, `src/{semantic,semantic_check,checked}.c`:
  owning fixture context, canonical types/packages/places/scopes, transactional
  checker and immutable checked fragments; no LLVM linkage or syntax pointers.
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
- `docs/P1_LEXICAL_CONTRACT_AUDIT.md`: fixed/subset/deferred distinctions and
  lexical spec holes; the scanner subset is not normative NewLang grammar.
- `docs/P1_SOURCE_LEXICAL_FRONTEND_REPORT.md`: contracts, test evidence, findings
  and P2 readiness/adjudication requirements.
- `docs/P2_MINIMAL_SYNTAX_CONTRACT.md`: pre-implementation grammar audit,
  lexical/contextual-word profile, coverage/error distinctions and limits.
- `docs/P2_MINIMAL_SYNTAX_FRONTEND_REPORT.md`: ownership, tests and P3 handoff.
- `docs/P3_SEMANTIC_SLICE_CONTRACT.md`: authority audit, scoped reborrow rule,
  ownership/transaction contracts and conservative limits.
- `docs/P3_FIRST_SEMANTIC_VERTICAL_SLICE_REPORT.md`: operation/loan matrices,
  proved/unproved boundary, tests and P3 review handoff.
- `docs/`: historical [P0 toolchain decisions](docs/P0_TOOLCHAIN_DECISIONS.md),
  [P0 architecture](docs/P0_ARCHITECTURE.md), and [P0 report](docs/P0_REPORT.md).

There is still no full-program parser or semantic checker, typed MIR, LLVM lowering,
optimization framework, broad FFI, concurrency, separate compilation,
self-hosting, public token/AST-dump mode, or speculative M8.3 implementation.
