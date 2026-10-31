# R970 strict BLIND source first-pass

Inputs exactly reproduce Issue #963 comments 6437394806 + 6437510768; original source/oracle/SHA never modified. Each CTest verifies frozen source SHA256/bytes, records first parser/CHECK stage, requires negative stage-qualified fail-closed with named output dev/ino/bytes sentinel, and when positive is accepted attempts strict C17/native GCC/Clang O0/O2 and sanitizers in Actions.

Limits: test does not establish original physical root / BackingRegion / Allocation / Domain, actual heap worlds 0/1/2, Copy-old values, or distinct genuine CHECK instance/foreign arm. These require genuinely owned passive/separate host proof and safe teardown, not a mock/shallow copy. R968 identified un-restored A foreign child destruction as unsafe; STOP that subcase pending a safe R-only ownership boundary. R-FAST tests only formatting, compilation and CTest registration, NOT runtime. No P implementation, fixture or oracle referenced.
