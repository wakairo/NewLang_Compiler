# Machinery V3: explicit actual R registration receipt

FAST2 successfully built the source-issued public probe and verified the fixed-P
Git context. Its shared CTest log gave the 705-distinct total but did not print
the exact R test name. V3 adds an R-owned configure-time query of actual CMake
TESTS, checks one exact r1173.changing_current.integration registration, and
prints that receipt. The shared FAST inventory still separately verifies the
complete CTest list. No semantic/native test is added to FAST. Source/oracle,
probe, observer, fixed compiler and inherited tests are unchanged. V1 and V2
manifests remain retained. Driver only selects this new execution manifest.
