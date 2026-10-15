# R437 independent pre-registered first pass (frozen before observing candidate outputs)

Authority: candidate d26d30568018f9d4866a9841d9eba6966a703557, Compiler/main 6e3cb6051b286e020d852c04bd37161cc08a367f, Draft17.30. R branch only; no P sources/tests/evidence read.

Independently authored positive sources:
- r_none.nl: two original lexical Node roots: anchor.payload=31, peer.payload=79, both links start None. Independent runtime selector None enters None normal arm, and only anchor.next changes from None to Some(ptr to peer); old replace Copy is None. The unselected Some normal arm does no write.
- r_some.nl: peer.payload=83, anchor.payload=47; anchor.next starts Some(ptr to peer). Runtime selector Some(ptr to peer) enters Some normal arm; only anchor.next changes to None; old replace Copy is Some(original peer pointer). The unselected None arm does no write.
- Both must preserve distinct original Node physical addresses, both payloads, peer.next=None, and surviving identity/provenance; selected arm tags and old return must be observed, not inferred from matched checked worlds.
- These exact two inputs are required; errors are classified SOURCE/PROFILE BLOCKED, not adjusted from Production fixture knowledge.

Independent refusals: invalid @payload field (source rejection), ref escaping write-loan scope (source rejection); public-owned certificate acceptance and altered candidate tests for wrong original root, wrong changed field, crossed arm worlds, variant mismatch, missing arm ownership, missing operation on a source-present change, stale before/after-change stage. Each is an independently proposed falsifier; unsupported or legitimately legal cases are marked unproven rather than invented as violations.

Evidence path: SHA256(NewLang source), real candidate CLI --check, parsed+checked owned proof via independent public API harness + positive/negative validation, CLI-generated C17 bytes/hash, -std=c17 -Wall -Wextra -Wpedantic -Werror GCC/Clang O0/O2 plus sanitizer, actual exit/logs and physical carrier observer if independently buildable. Compiler's own tests, logs and generated C are not inputs to this gate. Generated program process exit 0 alone is NOT enough for PASS. If native physical address/tag/ptr/sibling/old-Copy observations are unavailable, report a bounded gap; never upgrade a native smoke test to an independent physical proof.

No heap, nonCopy owner, CFG, LLVM codegen, product, or general soundness claim.
