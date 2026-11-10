# Surviving original after a closed H match

The source creates `retained` before either fallible H allocation. All normal
H0/H1/H2 worlds preserve it. The returned real u8 result drives a scoped write
to that same original after all H owners and loans have closed. The source then
reads its current field and sibling and retains the real old value.

The C17 consumer reuses CHECK's sealed `normal_frame_unchanged` and binding-prefix
relation. The exact outer None arm supplies an observation of the unchanged T
prefix, not an H-success capability. Existing allocation-arm cleanup validation
must finish on every normal path before the root caller can use its original
again. Existing field/loan/current checks and physical local storage perform the
write. No CHECK issuer, source rule, public API, ABI or memory-state join is added.

This finite admission covers a literal flat two-u8 lexical T immediately before
the typed outer allocation-match, the existing nested H producer/owner and Copy
caller, and a scoped postmatch direct replace and ordinary scalar reads. It does
not add changed-memory joins, owner/ref escapes, arbitrary intervening effects,
loops, multiple surviving roots or general control flow. The existing in-arm
write and pure scalar consumers remain cheaper choices for simpler operations;
they do not implement this postmatch effect.

The original three sources and oracle were frozen before execution. The two
source negatives test a wrong declared field and missing H1 deallocation. The
prefix probe checks actual source-owned identity, current facts and loan closure.
The emitter probe corrupts twelve real root caller fields and requires refusal
without output clobber, then exact restored retry. The existing Copy and canonical
ownership probes also run on this source. The native probe runs strict untouched
C first and then passive physical checks under all three allocation quotas. OOM
uses a fresh bounded CHECK selection, every emitter allocation, and named-output
sentinel/inode/mtime checks. Historical sources and FIRST evidence remain in Git;
no new multigeneration SHA protocol is introduced.

This is bounded compiler capability evidence. It does not establish general
soundness, Product GO, or a smaller library-author plus ordinary-safe-caller UB
audit/re-audit closure than well-factored C or a safe Rust facade.
