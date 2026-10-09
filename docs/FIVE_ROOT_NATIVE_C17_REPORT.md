# Five-root / three-field native C17 report

Track: P — Compiler Issue #251. Candidate remains OPEN/unmerged for independent
Coordination review. Exact candidate SHA and PR-triggered CI URL are recorded
in the single final Issue handoff; this document does not self-ACCEPT or merge.

## Authority and implementation

Startup main: `d8765a5ae11919d3076cec0d1432973a0f18fdcd`; canonical Draft17.30
§3.2b.3. Process, Design Decision Procedure, DI-009–014, Compiler Testing
Strategy, Product Value Strategy and North Star anti-gaming §11 were checked.
Historical design audit N/A is justified in the
[contract](FIVE_ROOT_NATIVE_C17_CONTRACT.md). **Semantic delta = 0**.

The unchanged `tests/fixtures/five_root_three_field.nl` hashes to
`812042833072145f761ddc21cbf774d81b64da3a34f6d466ba7db01a6d704d7d`.
Public `newlangc` now returns **exit 0, real generated C on stdout, empty
stderr** for this complete source. Negative sources reject before C; accepted
constructs outside a supported emitter retain explicit backend unsupported.
No source, AST, checker, certificate engine, canonical document or toolchain
pin is changed. The adapter consumes existing owned source/semantic evidence.

## Native observations

The generated code actually allocates and initializes five independent
56-byte heap Nodes and performs all six scoped field Changes. The read-only
observer snapshots the complete actual objects while all five are live:

| Physical fact | Result |
|---|---|
| src.child | Some(actual A address) |
| A.prev | Some(actual C address) |
| A.next | Some(actual B address) |
| B.prev | Some(actual A address) |
| B.next | Some(actual C address) |
| C.prev | Some(actual B address) |
| dst.child | None |

All remaining sibling fields stay None; payloads remain source-derived
1/2/3/4/5. These are observed memory values, not synthesized semantic seeds.
After the live snapshot, the observer checks EndRoot/finalization/release
using saved integer addresses and never loads Node fields after EndRoot.

| Guest failure | Attempts | Successful mallocs / frees | Actual free order |
|---|---:|---|
| first | 1 | 0 / 0 | empty |
| second | 2 | 1 / 1 | src |
| third | 3 | 2 / 2 | A, src |
| fourth | 4 | 3 / 3 | B, A, src |
| fifth | 5 | 4 / 4 | C, B, A, src |
| none (all Some) | 5 | 5 / 5 | dst, C, B, A, src |

Each world runs observed and unobserved. There is also an unwrapped,
uninstrumented real-platform success. Two topology-preserving source
variants combine alpha/arm permutations with distinct field-constructor
argument permutations; both generate changed C and execute all six worlds.
Full canonical comment form also runs. Pure alpha/arm normalization may
legitimately emit identical C; that alone is not treated as data-spine proof.

The independently asserted changed-tail policy control is source-admitted,
emits a different physical A.prev write and exits 77 in the observer. Eleven
safe generated-C mutants cover wrong sibling, missed write, tag, governing
domain, owner, duplicate/missing/early free, post-free reloan request, false
fifth allocation observation and hidden cleanup. Every mutant must fail via
an observer assertion **before UB**, with no sanitizer diagnostic substituted
as success. No observer cleanup or topology repair exists.

Measured generated-C hashes, native binary hashes, checked expectations and
complete stdout/stderr/exit records are saved in
[measured evidence](evidence/five_root_native_c17.json). These are historical
run observations: addresses, binary hashes and sanitizer source paths can
vary on repeat runs. The integration test regenerates and remeasures, never
uses this JSON as a positive input. Exact head CI is separately required.

## Destructive tests and regression

- Existing public full-source positives/negatives, 0..5 semantic release
  worlds, seven checked facts, wrong D/Allocation, scope/stale ptr/conflict,
  branch-world/numeric-origin, Change/Reset and OOM controls remain.
- Existing 57 certificate poison controls now also exercise the ordinary
  backend API: unsupported, NULL output, unchanged length. Another 110
  lowerer controls attack traversal coverage and scalar/payload/entry/domain/
  loan correspondence. None can publish partial C.
- Exhaustive emitter/revalidator allocation injection: **845/845** malloc/
  calloc/realloc failure points. Every failure reports OOM with unchanged
  borrowed snapshot/view/output/length; clean retries match C byte-for-byte.
- Existing certificate construction/validation OOM remains exhaustive
  (141/844 positions). Existing whole-source checker OOM remains **sampled**
  (248 of 15468 opportunities); it is not relabeled exhaustive.
- All original **246 CTests** are retained; two new contract tests bring the
  suite to **248**. Old two-root custody/native behavior, oracle.adapter,
  oracle.smoke and artifacts.integrity are regression requirements.

Local GCC Debug, GCC Release/NDEBUG, Clang, ASan and UBSan full suites pass
248/248; clang-format passes. ASan/UBSan configurations instrument both the
compiler and the actually generated native programs. Native builds use
strict C17 warnings, active observer checks under NDEBUG, and sanitizer
`-fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie` on the
supported Linux x86_64 host. No sanitizer exclusion or waived native path.

Host: Debian 13 Linux x86_64; GCC 14.2.0, pinned Clang/LLVM 23.1.2. CI retains
the five Ubuntu24.04 PR jobs (GCC13 Debug, GCC13 Release, Clang23, ASan, UBSan),
fresh checksum-locked bootstrap and formatting. Ordinary build does not
change dependency pins. Final fixed-head results are in the Issue/PR handoff.

## Reproduction and stop

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-five -DCMAKE_BUILD_TYPE=Debug
cmake --build build-five --parallel 2
ctest --test-dir build-five --output-on-failure
./build-five/newlangc tests/fixtures/five_root_three_field.nl > five.c
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror five.c -o five
./five
NEWLANG_FIVE_EVIDENCE=/tmp/five-native.json python3 tests/integration/five_native_test.py \
  build-five/newlangc build-five/five_checked_probe gcc \
  tests/fixtures/five_root_three_field.nl none
```

For Clang/ASan/UBSan configure distinct directories with `CC=clang-23` and
`-DNEWLANG_SANITIZER=none/address/undefined`; Release uses
`-DCMAKE_BUILD_TYPE=Release`. Full CTest runs native tests with the configured
compiler and sanitizer, plus old source/oracle/artifact tests.

Classification: **COMPILER-IMPLEMENTATION** — faithful bounded execution
adapter. No new semantic blocker, Draft gap or safety rule change was found.
Private 64-bit C representation and finite adapter budgets are explicit
implementation limits, not general native/ABI support. This is pre-detach
five-root evidence; no B detach/adoption, cJSON PASS or subsequent task is
claimed. Independent Coordination review decides READY acceptance/merge.
