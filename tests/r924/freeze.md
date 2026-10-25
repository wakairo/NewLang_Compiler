# R924 V3 immutable freeze, NEW NON-BLIND calibrated

Authority: P921 02d3d32e96a834f8918569a2c4e1f282d6476965.
Inherited R922 V2 source bytes copied VERBATIM, not a new blind first pass.
meridian source SHA256 36879c1c4f80635a1248eff30505046aebed888276740257d6c5deb1d17e91c6
glacier source SHA256 a879ddffb819221f347ba77a5a47ed90f33f630ce2fc56bb93526bea1b332b72
NEW harness SHA256 337e52106f222ea5260fefb7b7ac45e75e93adabe4519d5dec5797f4c47deb93
Previous V1/V2 source/oracle/SHA and PR923, original RFAST/FULL5/PARTIAL never edited.

Gate A: check_definition() failure in semantic_check.c invokes an EXTERNAL
nl_definition_owned_take from DIFFERENT copy_body.c object in static archive.
R-only --wrap=nl_definition_owned_take synchronous interception + R-only
wrap of owned_call/attach counts EXACTLY two live pending stateful calls.
One and only one pre-take hit, ORIGINAL checker-owned root/context nonnull.
Original source IF arm getter maps arm0/1, original proof arms and distinct
live context_origin are checked before changing whole NLCheckedArm entries.

V3 mutations per family, independent cases in this exact order:
1) swap WHOLE parent->arms[0] and [1]: getter reverses arm order
2) corrupt parent->arms[0].match = 0: getter index0 selects previous arm1,
   index1 becomes NULL for the authentic conditional
3) change parent->arms[0].context_origin to original LIVE arm1 context;
   getter still selects arm0 but authoritative context origin differs

For EACH: call actual __real_nl_definition_owned_take with corrupt original
checker-owned parent, require identical nonnull *root and *context after
return, immediately restore EXACT two original structs before checker cleanup.
After normal checker cleanup and session close: INVALID status, count0,
no private link. NEVER free or retain a borrowed pointer after cleanup.
Use the exact SAME original registration semantic context pointer for
healthy retry, no fresh semantic context. Source and parser and registration
destroyed after successful owned session close and before replay validation.
Both original roots remain different, same named root/field updated in
two independent arm worlds, sibling and second original root unchanged.
Meridian index0 old26, new63/118, sibling137, other79/204.
Glacier index1 old75, new33/142, sibling229, other14/186.
Both validate PRE, RETURN-old, Change, loan closed and one replacement.
CLI remains CHECK3/P6-JOIN-PRECISION, not public CHECK0/C17/native.

STOP if no interception, >1 hit, wrong source edge, impossible ownership
restoration, moved root/context, no same-context retry, false old return,
sanitizer or CI error. Never retune the oracle. POST_BUILD physical positive
logs archive six PRE-TAKE-HIT / REAL-TAKE / RESTORE / RETRY cases on success.
Budget RFAST max6 including initial ref, FULL5+PARTIAL max1 pair.
No producer, header, workflow, R922 branch/PR changes or manual retries.
