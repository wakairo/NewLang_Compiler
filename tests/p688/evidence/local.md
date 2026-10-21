# Local verification before candidate CI

Production delta: src/main.c +80/-5 and one 18-line target C header. CMake adds
two tests/targets (14 lines), with no existing expectation migration required.
The Stage0 estimate (~60-100 production lines plus fixed header) held; no new
semantic state, public issuer, parser, source rule, proof registry, per-record
validator or fixture name/value table was needed. New target ABI policy is real
but narrow: one opt-in symbol/header/compatible C17 ABI, not permanent support.

- GCC13 Debug, frontend-only: **377/377 CTest PASS**; raw log retained.
- New positives GCC13 / retained Clang23.1.2, strict C17 O0/O2: **8/8** separate
  object producer/consumer link+run pairs, consumer exit0/no stdout.
- Wrong consumer expectations:8/8 actual exit1 controls.
- ASan/LSan and UBSan, retained Clang23.1.2 frontend-only:2/2 each new tests PASS,
  including sanitized separate-TU native and driver fail-at-N allocations.
- Shared gate per source:7 corrupted original-artifact cases refused, restored
  control accepts, no validation allocation/public snapshot change. Fresh
  selected-call OOM23/74: rollback and NULL artifact. Entire driver injection
 169/326 failing positions: no C, eventual recovery (gate_oom.txt).
- Private static symbol link refusal, duplicate producer symbol link refusal,
  conflicting prototype with header, synthetic CHAR_BIT16 compilation refusal;
  eleven typed CLI/source/profile controls (native_*.json).
- Normal C output equals baseline bytes for both positives. main()->u8 remains
  CHECK0/normal emit4. New output failure `/dev/full`:CLI1; no arbitrary stream
  transaction claim.

Local toolchain is retained Debian13 x86_64; pinned normal Actions remains
Ubuntu24.04 / LLVM23.1.3 unchanged. Local frontend-only is PARTIAL evidence,
not the FULL5 gate. Independent R/F and other target/compiler ABIs untested.

Preserved first local test issues, before CI (not production findings):
1. Frozen rich.nl omitted the mandatory `if (...)` parentheses, giving syntax3
   P13-IF-OPEN instead of planned CHECK0/backend4. Original retained unchanged;
   separately frozen rich_calibrated.nl confirms the intended profile boundary.
2. Public probe compilation initially needed `<stdlib.h>` for strtoul; fixed
   test include before running the probe. No source/oracle change.
3. First new integration run overbroadly replaced `17` in the wrong-oracle C
   control, changing its `c17` header spelling to `c99`. Compiler correctly
   refused the nonexistent header. Original consumer untouched; test now only
   substitutes a standalone numeric token. First CTest failure log retained.

All local corrections preceded candidate CI. No CI retry/repair is authorized.
Technical success does not resolve the VALUE-ALIGNMENT-RISK in Stage0/README.
