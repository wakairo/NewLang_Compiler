P-owned preparation history, not a compiler semantic/proof failure:

- Read-only Gate A probe initial host compile omitted newlang/parser.h; gcc rejected unknown NLParser and implicit parser declarations before program execution. Corrected only the external /tmp probe include. The exact diagnostic remains in the tool transcript; no successful run was attributed to that attempt.
- First local ctest invocation lacked the activation PATH, exited127 before any test. Raw stderr is local-ctest-path-first-error.log. The activated first actually executed positive test is separately preserved.
- Production build, executed positive/negative/OOM/CLI/full/ASan/LSan/UBSan/Release tests had no unexpected semantic/proof failure or code repair. No Actions have run at this recording point.

- Staged git diff --check reported trailing spaces in raw ctest -V output (the generated Environment variables: line), after the pinned formatter passed. Raw logs are retained byte-for-byte; code/document diff checking excludes only these .log evidence files. This is a host evidence-format warning, not a source/proof/CI failure. No whitespace rule or test was disabled.
