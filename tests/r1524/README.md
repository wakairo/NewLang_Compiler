# R1533 C1220 same-FIRST replay of R1524 terminal-receiver experiment

This R-owned directory reuses the independent R1524 experiment.
Operational authority: main b2f22d39b61c0cf3ea18b7a1529fa8f0bf801be1,
CURRENT_SPEC Draft17.38. Exact candidate parent:
b1a6f1725abe25981ad29088175de581f956f919, tree
1fb87662e11c8e1ccd4d54a45be0e3d38a8e67b9.
Execution release: Issue1533 C1220; branch r-fast/r1533-prefix-replay;
DRAFT / NEVERMERGE PR base p1531-terminal-integration-c1209.

This new execution actor inherits original R1524 FIRST and R1533 PREP;
it is neither the original author nor the preparer, and is not fresh BLIND.
Original R2c86, FIRST, RED and the 7de execution remain historical:
P1 reached CHECK and public-owned but C17 refused; native was NOT_RUN.
P2 source/backend/native was NOT_RUN after that P1 stop. This replay does
not regrade those observations or predict the new target's result.

The six .nl inputs and r1524_first_oracle.txt are the original FIRST bytes.
first_manifest.json identifies all seven immutable payloads. The earlier
authority and PREP statements inside that oracle remain historical FIRST,
not an assertion about this later execution phase.

CTest r1524.first_packet verifies the frozen hashes, reads actual checkout
metadata, checks each negative through the ordinary CLI and a public API
probe, and checks existing/absent named C output no-clobber. Positive paths
then require ordinary CLI CHECK, actual registered-main checked-owned graphs,
public named-Copy and actual/formal validators, receiver original p/A/D
entry/post observations, authenticated C17 emission, and native observations.

public_probe.c creates no guest roots, values, references or owner facts.
Its only source is the actual complete .nl unit plus an ordinary main()
entry expression. It observes and validates only source-issued artifacts;
no semantic seed APIs or fabricated certificates are used.

native_observer.py recognizes only our newly emitted C. Its guest observer
reads actual objects/arguments and records events. It cannot create payload,
end a lifetime, mint authority, or compensate a missing guest cleanup.
Unrecognized C spellings stop before native execution with HARNESS_MAPPING
and show our own generated C for a possible bounded mechanical repair.
The real allocator is used for successful guest allocations; fail-at1/2
covers H0/H1, and fail-at0 covers H2. Payload0 remains H2 success.

run.py applies strict C17 GCC/Clang O0/O2 and the selected NEWLANG_SANITIZER
flags to native compilations; it preserves diagnostics. The public probe
inherits the same CMake strict flags and selected sanitizer via production
library interface dependencies. No semantic/native test runs at configure
or build time; FAST checks registration/build only.

Passing CTest stdout may be suppressed by the existing workflow. A passing
r1524.first_packet means its assertions reached normal completion; it does
not mean every per-world stdout line is visible in the GitHub job log.
On failure, the diagnostic stream records the first reached layer, actual
command exit, source hash, and relevant raw observation.

Allowed changes are this directory and one root CMake include only.
No P/F material or other R originals were used. This remains finite
capability evidence; Product GO PAUSED and H1 HOLD are unchanged.
