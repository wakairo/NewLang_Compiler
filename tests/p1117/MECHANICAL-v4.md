The first v3 current-legacy test used the narrow P2 expression parser for an
ordinary source call containing core usize literals. The source parser is the
correct existing API. Its genuine parser RED is retained in evidence with the
candidate HEAD/tree. A diagnostic-only inverted test_unchanged query also
printed its expected commit difference as a spurious failure; registration and
the actual typed Bool invocation remain the current commit oracle.

legacy_current_v2.py corrects those two harness details. The original legacy
C files, original v3 generator, oracle and all input versions are unchanged.
Every old type/loan/write/rollback/OOM assertion still compiles verbatim.
No production file changes for this repair. Current evidence forwarding selects
prospective history23; history22 from the first v3 candidate is externally
pinned by SHA/Git blob and retained exactly. P1107 appends harness8 only for
the legitimate changed CMake; its original source/oracle/FIRST remain exact.

This is a versioned harness correction, not a claim that the first test passed.
The original v3 current/effect/foreign-owner/dep/OOM oracles remain mandatory.
