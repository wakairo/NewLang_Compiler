# P688: experimental C17 scalar export v0

**DRAFT / DO NOT MERGE. Technical exploration only.** Authority is
Compiler/main `92367144da5c672282b91fd291962e3b923492b3`, Draft17.33.
Stage0 and unchanged-main evidence are in [evidence](evidence/stage0.md).
The original P685 and archived P683 inputs/expectations are unchanged.

## Use and target contract

Build the normal compiler (or frontend-only build as documented in root README).
From the repository root, with compatible GCC13 C17 producer/consumer targets:

```sh
./build-frontend/newlangc --check tests/p688/threshold.nl
./build-frontend/newlangc --experimental-c17-u8-export low_threshold \
  tests/p688/threshold.nl > build-frontend/producer.c
gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude \
  -c build-frontend/producer.c -o build-frontend/producer.o
gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude \
  -c tests/p688/threshold_consumer.c -o build-frontend/consumer.o
gcc-13 build-frontend/producer.o build-frontend/consumer.o -o build-frontend/consumer
./build-frontend/consumer
```

Expected consumer exit **0**, no stdout. Consumer actually compares the returned
`uint8_t` to17. `limit.nl` / selector `upper_limit` / `limit_consumer.c` independently
checks53, from a Copy local plus explicit return. Expected literals occur only
in independently authored consumer assertions, never injected into the producer.

The sole external symbol is **`uint8_t newlang_c17_u8_export_v0(void)`**, declared
by [experimental_c17_u8_export_v0.h](../../include/newlang/experimental_c17_u8_export_v0.h).
Both TUs include that fixed header; the producer emits no C `main`, keeps source
helpers `static`, and returns the actual runtime `nl_fn_ID()` call carrier.
The compiler selects an exact registered source name, creates and parses a fresh
ordinary call, and semantic-checks it before reusing the unchanged body/source/
context/owner/result validator. Signature matching alone cannot authorize output.
Only reachable validated functions are emitted. The existing two scalar-frame
limit, Copy-only body admission, no hidden dependencies and effect bans remain.

This is a **new nonnormative target ABI policy**, explicitly authorized by #688,
not source `export`/`extern`, NewLang separate compilation, general FFI or I/O.
No source/foreign arguments, `main` export, pointer/ref/owner/Domain, original
record state or authority crosses this boundary. Pure scalar calls can repeat;
there is no retained foreign state or callback. The generated source plus header
must be compiled together with compatible C17 compiler/target ABIs. Actual tests:
Linux x86_64, GCC13 and Clang, O0/O2; exact 8-bit-byte/`uint8_t` assertions apply
in **both** TUs. No ABI promise across versions, arbitrary targets or toolchains.

One producer artifact per linked executable: two producers use the same fixed
versioned symbol and **must fail ordinary linking with duplicate definitions**.
Do not use linker options that silently pick one definition. Private generated
names/IDs are not an ABI. A mismatched declaration **with this header** diagnoses;
C TUs bypassing the header can still cause C UB. Caller must preserve the header,
prototype and compatible target/compiler contract. This does not make arbitrary
foreign C safe, provide module privacy, or change source ownership semantics.

## Diagnostics, regression and failure behavior

Default CLI/`--check`, unit-only process entry, generated guest exit0 and help
remain unchanged. Unknown/empty/injected selector, `main`, wrong formal or result
profile: **CLI4 `C17-U8-EXPORT-SELECTOR`, zero C** after normal unit registration.
Duplicate names and invalid function bodies still diagnose **semantic CLI3**;
syntax failures also use3 and remain classified as syntax. A legal richer body
passes CHECK but export gives **CLI4 `C17-U8-EXPORT-UNSUPPORTED`, zero C**.
`main()->u8` still CHECK0 / normal emit4. Wrong argument count remains CLI2.
I/O/resource failures remain CLI1 (some existing parser/checker OOM paths report3).
All admission precedes C output; write failure (tested `/dev/full`) reports1 but
**arbitrary FILE/pipe output is not transactional**, so consumers must check the
compiler status and discard failed output. No two-file transactional writer added.

Run `ctest --test-dir build-frontend -R '^p688' --output-on-failure`.
`native.py` checks frozen originals, baseline normal C byte equality, genuine
public call/body/result observations after input AST destruction, separate
objects, real consumer values17/53 and a wrong-oracle exit1 control at O0/O2,
private static link refusal, producer collision, conflicting header prototype,
synthetic `CHAR_BIT=16` refusal (not actual C28x), source/profile diagnostics and
output failure. `gate_test.c` uses the existing private gate, rejects seven
test-owned corruptions per source including foreign context/body owner and wrong
call result, and tests allocation-free/state-unchanged validation. Fresh selected
call OOM rollback:23/74 positions, NULL failed artifact; full driver fail-at-N
allocation paths:169/326 positions, zero C on failure. ASan/LSan and UBSan cover
these actual driver/parser/checker/emitter paths. None is independent R or formal
proof, and the scalar observation has no new public privileged proof API.

Raw generated C is preserved byte-for-byte as `evidence/*.c.txt` (not reformatted).
`freeze.json` was written before execution; `rich.nl` was a test-author syntax
mistake (missing condition parentheses), kept intact. Its separately frozen
`rich_calibrated.nl` verifies the intended valid-but-unsupported body. Public
probe's missing stdlib include and an overbroad wrong-oracle replacement were
local **test-tool** mistakes repaired before CI; neither changed frozen source,
consumer, oracle or production semantics. First local diagnostics are recorded
in [local verification](evidence/local.md).

## Historical and goal gates

KEEP Draft18 typed ordinary Copy return, 19 sequencing, 24 no authority from
bytes and 31 target shim/non-amplification. REPLACE no source rule; DEFER Draft34
source FFI, arbitrary ABI/arguments/owners and stable cross-target library support.
M687/C approval permits this target-only policy; historical M682 and rejected
PR684 status transport are not reversed, and PR684 code is not reused. Ledger
DI002/006/007/011 remain unchanged; no new normative source proposal is made.

**VALUE-ALIGNMENT-RISK:** well-factored C trivially supplies such a scalar helper.
This proves normal-success separate-TU integration only, not realistic C-class
library demand, stateful safe API, reduced library-author + ordinary-safe caller
UB-prevention audit/re-audit dependency closure, Rust superiority or Product GO.
Header/symbol/target compatibility adds real maintenance/caller obligations; a
manual C shim is simple and remains the cost baseline. Product priority for a
permanent export API remains HOLD; C decides whether this bounded cost is worth
adoption and whether independent ABI-focused R is needed. Product GO scoring
PAUSED, H1 HOLD and R655 limitations remain unchanged. No other Track launched.
