# Current u8 through an ordinary Copy call

C688 composes an actual current-field Copy with one ordinary readonly u8
helper and the same original's scoped Change inside a current loop. The original
P1366 A is byte-identical. B-field-zero-alias is a separate P positive with the
opposite field order, different values and longer callee/caller Copy chains;
it is not the excluded P1366 wrapper/inner-loop B.

The shared private scalar graph walk has two operand contracts: the existing
usize/IF family and a normal readonly-u8 formal/Copy chain. Every actual helper
node is consumed; source, registered plan, signature origin and the existing
finished-result custody seal authenticate the call. The current walk follows a
validated exact header FIELD_READ through real bindings, then admits that u8
actual and its returned Copy into the existing Change/frame/closure checks.
The body shares the final iteration world; it is never treated as an entry
snapshot. No CopyBody current-loop/INITIALIZE tag, synthetic IF arm, independent
certificate, new world, public issuer/API/ABI or source rule is introduced.

GCC Release, ASan+LSan and UBSan each pass eight new units, 119 adopted impact
units and 15 failure/compatibility integrations. Signature-origin, actual/formal,
result/BODY/field/current/source and same-bits foreign-child tampering reject
with unchanged output and restoration. The original loan escape and isolated
wrong-actual source remain semantic refusals. Old usize predicates and typed
returns/current callers are retained, with 20 byte-identical old C outputs.
Local CTest registration contains 962 tests, including the 11 new tests. The
unchanged CI-only T991 hook adds two tooling tests: the expected full CI total
is 964, or 963 without LLVM. Those two were not local compiler safety tests.

Fresh registration OOM censuses are A880/B922 (23 clones, two plans), selecting
11 distinct points each; actual CHECK censuses are A955/B985 (31 clones, one
finished-result seal), selecting ten each. Every selected failure fires OOM,
preserves the prior seeded context/body owners and output, and cleans up/retries.
These are not exhaustive sweeps. Named-output malloc/write/flush failures also
preserve bytes/inode/mtime, clean temporary files and retry successfully.

Twenty local guests pass: both sources, unedited raw/passive GCC and Clang C17
O0/O2, plus passive ASan+LSan/UBSan. A observes two backedges with current/input/
returned/old37, terminal current203 and sibling181. B observes three backedges
with91, terminal151 and sibling217. The same live main storage is checked at
every phase and write; pointers are cleared before it ends. Unmodified generated
C additionally traces each real call input to the field read, its returned Copy
through caller aliases to the same original store, and the callee tail through
aliases to its formal. Raw exit0 alone is not a numeric/identity observation.

All tested inputs are fixed at 1a700fc934686cbf37bc0a2a9e17abf61a0c4fe0.
Earlier f9bed18/a78df42 were not executed: static pre-build harness review
corrected an assumed terminal LOCAL hook (the emitter uses RESULT), removed an
unused adapter, and made a result mutation unconditionally different. Source,
oracle and Production code were unchanged. Their original freezes remain.

Apparatus RED is separate from compiler safety: ASan/UBSan configuration first
exhausted inodes; old generated CMake files were removed after archiving configs,
and the same host tuples succeeded with separate retry logs. Impact testing
initially omitted binaries invoked through Python wrappers. The original 28
successes and FileNotFoundError were retained; building the two dependencies and
running only the remaining 91 completed the 119. A supplementary read-only C
audit initially missed legal const-pointer syntax; correcting that audit changed
no C bytes or execution. No failure was relabeled or old oracle rewritten.

`evidence/LOCAL-FINAL.json` binds actual source/config/ELF/raw evidence to the
tested candidate. The archive preserves the baseline FIRST, unexecuted freezes,
apparatus RED/recovery, actual commands and generated C, main's original CI logs
and exact compiler/probe ELF bytes. Original P1366 NOT_RUN/NOT_OBSERVED remains.

Coverage is one H-free original, one changing field, named current-u8 actual,
one readonly helper with Copy bindings and a normal Copy tail. Wrapper/inner
loops, more formals/originals, cross-call refs, helper effects/nested calls,
conditional/RETURN helper extensions and general CFG are outside this increment.
These limits are not new source prohibitions. Private mutable-host contracts
and independent F/R material are unchanged. This adds maintenance to one shared
mechanism; capability success is not general soundness, C-target suitability,
library representation protection or Value1.1 author+ordinary-safe-caller audit
locality Product PASS. Product GO PAUSED / H1 HOLD; C decides adoption.
