# P694 Stage0 GO — before production edits

Fixed main4c39ddd25039ce605bd51e2b39996e4a5820b7ed/tree484895d129fc14467bb82856df6c6b7c80b4db9a,
CURRENT_SPEC Draft17.33. Exact main-push FULL5 39551773219 all5 and PARTIAL
39551773199 success, push/attempt1. Sources and expectations frozen before runs.
Baseline.json preserves exact source SHA, stdout C hashes and CLI diagnostics.
Actual normal source bytes1564 (settings) /1522 (Node), source/check/emit0.
New option currently CLI2. Real shell redirection with semantic failure3
truncates a prior artifact to zero bytes; current stdout /dev/full fails1.
The first missing layer is host output-file containment, not source/proof.

Small-reuse decision GO: parameterize one existing compile_v0 implementation's
FILE* sink for V0/V1, Node and Array output branches; preserve compile_v0 wrapper
with stdout for existing users/private tests. Add one named-output orchestrator:
canonicalize destination parent once using POSIX realpath; reject invalid leaf,
nonregular/symlink destination and source inode aliases by stat/lstat before
staging and again before commit; exclusive unpredictable mkstemp sibling0600;
fdopen; same compiler emission+fflush; fclose success; same-directory rename.
Caught failure closes/unlinks the private temporary; report unlink refusal if
filesystem itself prevents cleanup. No output bytes go to stdout in new mode.
All existing admission/issuer/C17 byte generation untouched.

Estimated ~120-160 production lines including shared sink refactor and file/path
handling, ~200-350 test lines plus fault controls/docs. POSIX2008 available,
actual Linux x86_64 user1000, GCC13 and retained Clang; normal Actions lock unchanged.
No new host abstraction/framework required within the authorized lane.
Paths are resolved against the process working directory (never changed).
Destination parent symlinks resolve once; final destination symlinks are refused.
Temporary names append .tmp.XXXXXX, so never equal the target name; too-long
filenames fail safely rather than relaxing exclusivity. Final successful mode
is private0600; permissions/ownership/hardlink topology of old target not copied.

Guarantee is atomic pathname replacement on stable writable local POSIX storage,
not fsync/power-fail durability, arbitrary stdout/pipe transaction, hostile
concurrent path/directory mutation, network-FS races, signals/crash cleanup,
Windows-native or metadata-preservation. Caught normal failures preserve old
artifact. File system refusal to unlink cannot be magically repaired; expose it.
Any newly unresolved supported-lane safety issue or scope expansion => STOP.

Alternative shell unique-temp && mv is valid; integrating the common compiler
sink removes repeated caller shell truncation mistakes for every current emitter
without changing source meanings. Keeping stdout remains supported. New named
mode is opt-in; malformed flag combinations remain CLI2. Historical gate N/A:
no source/semantic design change; all canonical/checker/issuer rules KEEP.
Goal is compiler output reliability, not state-derived export, safe-library audit
closure, C-vs-Rust advantage or Product GO. P692/M693 remain NO-GO-DEFER.
One prepared HEAD / one natural FULL5+PARTIAL pair only; no CI retries/repair.
