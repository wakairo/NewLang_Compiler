C137 prospective harness v2. All v1 code, sources, oracles, freezes and first
logs remain unchanged. Production bytes are identical to candidate v1.

The v1 direct ASan/LSan invocation passed every check in 102.50 seconds while
Clang's old full suite and the UBSan build competed for local CPU. It was not
run through CTest's 90-second limit. Treating that successful direct run as
proof that the new combined CTest fits its inherited limit would be unsound.

New run_v2.py selects one of the same three source profiles per invocation.
CTest registers profiles0/1/2 separately under the existing 90-second policy;
profile0 also retains the once-per-suite five source negatives,32 exact old-C
identities and10 history counterexamples. Every profile retains its genuine
current/native/physical/host-negative/original-owner/reached-OOM/no-clobber
checks. No check is skipped from the combined three-test suite. run.py remains
the exact v1 original. Old577 tests, workflows, T/R enrollment, sanitizers,
timeouts and toolchains are unchanged. Current inventory becomes580 partial,
581 full, with only the existing llvm.c_api difference.

The follow-up validation targets this new dispatch/registration and 90-second
budget with the sanitized build. Already-green strict GCC/Clang/O0/O2 runtime,
old577 and114 sanitizer regressions need not be repeated for this partition.
The final evidence-only commit will refresh prospective v9 expectations and
freeze hashes, retaining original v1–v8 bytes and all earlier evidence.
