# T993 declarative R-cohort test bridge (tooling pilot; HOLD)

This is **not** an R first-pass result or a NewLang source-issued owned/C17/native
proof. It extends the T991 project hook only for reviewed, CHECK-only CTests.
`tests/t993` has two new independent **T-owned synthetic** NewLang sources.
No R979/R985 source, oracle, previous results or Production implementation is used.

## Trust boundary / activation

* Both pre-existing FULL5 and NO-LLVM PARTIAL workflows pass the **same**
  fixed `CMAKE_PROJECT_INCLUDE` path established by T991, without trigger,
  matrix, permission or original CTest changes.
* T-owned `cmake/independent-r-tests-hook.cmake` runs at `project()` and defers
  registration to the end of the original top-level CMake directory, after
  `include(CTest)`, the compiler target and Python interpreter exist. The
  original `CMakeLists.txt` and P982 entire-file SHA freeze remain unchanged.
* **Only** `tests/r/approved-cohort-manifest.txt` (future R) or
  `tests/t993/approved-cohort-manifest.txt` (THIS pilot) can be selected.
  Neither present means zero R-channel CTests; both present is a configuration
  error. The pilot manifest must be removed only by a separately authorized
  future migration before future R enrollment. There is no user-selectable
  plugin path or arbitrary CMake include from an R branch.
* The R manifest is **plain ASCII data**, not a CMake script. Fixed T-owned
  `cmake/independent-r-manifest.py` reads it with strict bounds and checks
  each source SHA256, regular-file existence and non-symlink path within the
  checkout. It emits a known CMake `add_test` template into the build directory.
  Only that **validated T-generated** file is `include`d; never R text.
  Existing and duplicate test names abort configuration rather than override.
* The runner rechecks the source digest **at CTest runtime** and invokes only
  the actual `$<TARGET_FILE:newlangc> --check source` in one of two modes:
  `check_accept` expects clean exit0; `check_reject` expects CLI diagnostic
  rejection (rc2,3,4 and `error(` stderr). Neither mode emits/executes C17,
  performs OOM tests or grants ownership authority; those require a separate
  independently frozen R-native runner and new Coordination release.

## Manifest V1 (one cohort per branch)

The file must be LF-only ASCII, <=4096 bytes, with the exact first line
`NL_R_COHORT_V1` followed by **1–6** lines, no comments or blank lines:

```
case_id|check_accept_or_check_reject|relative_source.nl|lowercase_sha256_64_hex
```

* `case_id` must be `r<issue>_<lowercase_identifier>`, where `<issue>` is at
  least three decimal digits and the full rendered CTest name is
  `<case_id>.integration`; all rows must use the same `<issue>`.
* Source path must be `tests/r<issue>/sources/<ascii_identifier>.nl` for a
  future R case. The T pilot is **explicitly exempted** only for issue 993
  under `tests/t993/sources/`. No absolute, `..`, symlinks, duplicate source,
  duplicate names, unsupported modes, invalid SHA or oversized source.
* The SHA embedded within a branch's manifest is **not an independent trust
  root**; the R author could update both source and manifest. A future C
  release must preapprove/freeze the exact **40-hex R commit HEAD**, parent
  Production/T hook SHAs, complete **manifest SHA256**, all source digests,
  independent predicted oracles and budget **before** the first R test.
  An independent R reviewer must own that evidence under Process §7. A green
  T tooling check never retroactively changes historical R985 first-pass
  INCONCLUSIVE or permits an R CI rerun.
* Real 5+1 PR CI must check checkout PR synthetic-merge parent/tree identity,
  registered CTest names, individual execution/pass/fail logs in each job,
  unchanged `p982_freeze.unit` PASS, native/sanitizer/OOM evidence separately.

## Pilot test commands

`python3 tests/t993/manifest-guards.py` runs bounded negative tests in disposable
source trees for malformed name, duplicate row, source/path traversal,
absolute path, missing source, wrong digest, unsupported mode, oversized/
CMake command input, symlink path and deliberately forced runner failure.

The T993 pilot PR **must remain DRAFT HOLD**. Independent real R native
results, true user UB-audit locality vs well-factored C (§1.1) and Product GO
remain unmeasured. Additional parser/CI maintenance is VALUE-ALIGNMENT-RISK.
