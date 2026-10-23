# P761-HDR-ONLY repair follow-up

Coordination RELEASE6357022431 authorizes this mechanical follow-up from saved
P HEAD17d0d10d80925ecf59dd85b02471718a4927ae92, existing branch only; fixed main
0eb528e6f3bc1dc2ce3f606690dc67be78fecbd4, canonical Draft17.34.

The parent README and all original source/oracle/evidence describe the preserved
first attempt and its correct 431/433 STOP. They remain byte-identical. This
supplement records a separately authorized repair and must not overwrite that
history or relabel the original first red.

The only production delta from saved candidate removes the new private layout
preparation prototype from semantic_internal.h and places its identical
signature in semantic_check.c. No function body, issuer, validator or target
layout logic changes. Current strict -Wall/-Wextra/-Wpedantic/-Werror policy has
no missing-prototype flag requiring an additional declaration in the defining
translation unit. The whole semantic_internal.h is exactly fixed-main bytes,
not a changed digest/exception. The repair must preserve every entry of the
original preserved-sha256.json, including first-red logs and frozen sources.

Before tests, freeze this repair as a separate commit. Run one complete local
frontend suite and focused ASan/LSan and UBSan including all nine P761 tests.
Do not retry a new unexpected failure or broaden the repair. Only if local
checks pass, create one existing-branch DRAFT/DO NOT MERGE PR to trigger its
first natural exact-head FULL5 and separate NO LLVM PARTIAL. No Actions reruns,
workflow changes, archive_window repair, Stage2B, main merge, issue close or
independent R launch. C decides the later fresh BLIND R and adoption.

Historical Intent §4.2 N/A: mechanical private declaration placement, explicitly
adjudicated by C; no semantic/source rule, authority or normative choice.
Goal Alignment: preserve existing proof-integrity checks while enabling a narrow
source-issued T Copy proof in mixed semantic programs. Mixed H ownership dual
carrier and mixed C17/native remain unimplemented. Ordinary-safe author+caller
UB-prevention audit-locality benefit is unmeasured. Product GO624 PAUSED and H1
268 HOLD remain. There is no new product-goal departure from this placement fix.
