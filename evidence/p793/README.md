# P793 — Gate0 STOP, separate two-normal versus sole-normal boundaries

Independent main77d83dfc00850b14fd7c3bc95e7e3c12e34161fb, Draft17.34.
Freeze ca8df9c predates fresh frontend build/execution. No Production/testsuite
changes, experimental flags, issuer patches, CI or implementation PR.

Router has two normal IF arms, each invoking the same ref-write/u8→old-u8
helper on root1/index1, then an actual postcall field read feeding a second
runtime helper on independent root2/index0. Only one arm runs, so runtime has2
calls (3 syntactic call sites). TRUE/FALSE sources expect94/133 to reach root2,
first old67, second old29, siblings12/185. Both stop CHECK3 at IF span10:17–14:6,
P6-JOIN-PRECISION. Source grammar/definition parsed, richer join not implemented.

Thermal has one return arm and one normal arm. First actual helper targets
root1/index0; normal continuation loads that changed field and passes its real
Copy to independent root2/index1. FALSE normal path expects2 calls,61→105,
232→105, siblings170/16; TRUE has first61→88 then returns, no second call/read.
Both CHECK0 but C174/zero C. Actual source-owned observer reports public Copy
body/parameters3 in EACH real IF arm, despite the sole-normal projected call
and later top-level call independently returning0. An absent two-normal field
view on sole-normal is not itself a validator failure or proof requirement;
the measured Copy body/parameters3 is the blocker.

Readonly private seal diagnostic separately confirms existing proof+exact caller
but caller_nodes=NULL for actual IF-arm children; top-level projected/later
calls have caller_nodes. copy_body.c parameter_entry first requires snapshot
origin/count/root/source/full nodes. Existing nl_copy_body_seal snapshots direct
children only at function completion; these completed IF-arm callers are not
sealed by this fixed base. Never substitute the good projected parent-call
receipt for the original arm-world call. This does not demonstrate unsafe
acceptance; native publication remains fail-closed.

public-observer.c.txt is a diagnostic tool stored as data, not a new compiler,
issuer, registered CTest or host-generated positive world. It loads/registers
actual source, destroys initial AST/source, walks actual owned call/IF children
and calls the public validators. Compile with -Iinclude -x c this file -x none,
link the fresh frontend build/libnewlang_*.a in --start-group/--end-group. Optional
-DP793_PRIVATE_SEAL_DIAGNOSTIC -include src/copy_body_internal.h adds only the
separate readonly snapshot observation, never mutating an artifact. All success
or rejection statuses come from existing public APIs. The first local observer
compile omitted stdlib.h and failed before execution; that log is retained,
then only the tool include was corrected. A temporary text insertion assertion
also failed before writing; no input/compiler edit was involved.

Negative sources independently reject read-formal write(P3-TYPE-MISMATCH),
unknown field(FIELD-UNKNOWN-FIELD), escaped loan(P8-EXIT-DEPENDENCY), eachCLI3,
zero C; all positive/negative named-output sentinels remain exact. Foreign-world,
stale fact/old Copy mutants, native, sanitizer and OOM/rollback gates NOT TESTED
because positive source/public gate failed. No claims of runtime numerical
values beyond the frozen oracle. Source/compiler stays unchanged.

KEEP DI006/007/017 and M2/M3/M4 selected field/call/IF/return semantics; no new
source law. Future source-issued named-call field-join extraction and IF-arm
seal work needs C authorization before emitter work. No import of unadopted
P786/#790, no new public proof/general CFG/effect mechanism. Product GO PAUSED,
H1 HOLD, library audit-locality benefit unmeasured. Preserved map, not PASS.
