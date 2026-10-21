# P694 local candidate verification

Stage0 source/oracle and old stdout C hashes precede production edits.
Production src/main.c +116/-7; one common FILE* parameter and preserved stdout
wrapper, one POSIX target-safety predicate, one private-sibling staging function,
CLI dispatch. No source/checker/issuer/public-proof/header/lock/workflow changes.
CMake9 lines, README10; tests268 lines before formatting/doc/evidence.
Prediction120-160 production lines incl sink refactor held (116 additions).

GCC13 Debug frontend-only:378/378 CTest PASS. New integration initially passed;
no local source/oracle calibration or test failures. Retained Clang23.1.2 ASan/
LSan and UBSan: new integration PASS. Local direct GCC13/Clang O0/O2 native
8/8 runs exit0, compiler/guest stdout empty. Same byte hashes both prior stdout
and fresh/replaced named output: settings cd4f22d82e5107286cb2735fec224fc9ddb7f716c468fe4112f3ab3f06b61a4c,
Node6040f24fd9aedffe07ca6178822bb6c8b94b2aaf33f1a39b8df9207b545af4b6.
Test-only Array4 branch byte equivalence PASS (not ordinary source admission).
Fourteen deliberate file/source/profile failures per host compiler preserve
old artifact, plus real stdout /dev/full baseline1. Source aliases intact;
no caught-error temporary leaks. Actual kernel RLIMIT_FSIZE causes output1;
link-wrap fflush/fclose/rename failures preserve old valid C and remove temporary.
Wrapped direct allocator fail-at-N:364 settings /307 Node failing positions,
old target preserved on each, cleanup and eventual recovery PASS. Libc-internal
allocator/filesystem errors are not exhaustively injected; no universal claim.

Local lane Debian13 Linux x86_64, unprivileged user1000, GCC13.3.0 and retained
Clang23.1.2, frontend-only PARTIAL. Normal Actions stays Ubuntu24.04/pinned
LLVM23.1.3 FULL5. One prepared HEAD / natural FULL5+PARTIAL only, no CI repair.
Atomic name replacement under stable local POSIX paths; no fsync/power-fail,
crash/signal cleanup, hostile concurrent directory/source/target or network FS
safety promise. If filesystem rejects cleanup, explicitly diagnose leftover
pathname; tested normal caught failures remove temp. No new semantic issue found.
C integration recommendation GO subject to exact-head CI/review of this bounded
file contract; not Product GO. P692/M693 deferred work stays deferred.
