# R1581 frozen-source independent evidence apparatus

C1302 execution release:
https://github.com/wakairo/NewLang_Compiler/issues/1581#issuecomment-7055154924

Original source-FIRST:
https://github.com/wakairo/NewLang_Compiler/issues/1581#issuecomment-7038043349

Public-interface plan:
https://github.com/wakairo/NewLang_Compiler/issues/1581#issuecomment-7038279763

This directory contains the exact six frozen units and oracle from FIRST.
The manifest is a transport copy of the seven accepted byte counts/SHA256s.
The Python expectations are comparison data derived from that original oracle,
not measurements. Measurements come only from the public probe and actual CLI.

The public probe loads a complete unit, parses and registers it, checks an
ordinary main() selector, and observes actual owned call bodies through the
declared public accessor. It never requires a checked body's source bytes to
equal the entire original physical input, never seeds guest values/formals,
and never maps an event by finding a matching expected number. Body source
lengths/spans and context associations are diagnostic observations.

A known quantity is compared only when an actual getter returned it on the
observed graph path. Genuine division/remainder views are revalidated.
Unknown placeholders are never Known zero. Missing exact observations remain
NOT_OBSERVABLE; complete scalar actual/formal/result/world custody is not
certified by these local observations. Source-body equality and generated
C identifiers are not new gates.

Four logical CTest groups cover public positives, true-invalid rejection,
canonical-legal excluded coverage, and normal CLI/pristine C17/native.
No numeric main ABI is added. Unit exit0 is not internal-quantity proof.
No generated-C observer is implemented without authentic source/C mapping.
The pristine C bytes are saved and hashed before compilation and rechecked
after execution. Native internal quantities and source/C event mapping remain
NOT_OBSERVABLE.

C1302 explicitly permits one bounded build-time source-interface preflight
per build: the already built normal probe processes the six original sources
once and the driver compares actual known observations. It also prints actual
ctest -N own registration, which executes zero CTests. This preflight is
separate from FAST format/build/inventory and never replaces FULL5/PARTIAL.
The standard workflows are unchanged. CTests and preflight retain their actual
outputs under the build's r1581-results directory and may append measured
summaries to GITHUB_STEP_SUMMARY. A passing test verdict is distinct from a
trace actually retrieved by the reviewer.

No private implementation, other-track fixtures/oracles/reports, aggregate C,
or local/P-equivalent execution environment was used to construct this code.
Historical public metadata exposure is recorded on Issue #1581. A transient
restoration check initially found the previous turn's in-memory cache absent;
the original FIRST and all seven accepted hashes were restored from its
individual GET before any Git mutation or execution.

The only repository writes are this directory and minimal root registration.
The evidence PR must remain DRAFT and NEVERMERGE. Budget and STOP discipline
are those of C1302, not inferred from successful tests. Product GO PAUSED,
H1 HOLD, audit-locality versus well-factored C UNMEASURED.
