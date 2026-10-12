# P362 preflight / conditional integration decision

Fixed semantic main a5343bf70004b30a8b026e52aa2fde93b5920be0; canonical Draft17.30
blob db8fe08f811acc8286f2d37e9d3229b5d7a67067; fork PR353
82578ff6eb23970e3ae934c98a38383ca01468f7. Newer main governance commits do not repin.

Input hashes issued before historical calibration/M361 reading: first adoption
bef64179aca653b04a3fe20bc3d03c81906cc7a2fd7639ffb8bb1757b3c82ffc;
returned owner bd7692f8118c8e4de2b7662b0f90fcf503a548d9948bfded27bd430ad127716d.

Read Process §4.1.1/4.2, Design Decision Procedure A–C, Ledger DI001/002/006/007/
009–014, proposed unadopted DI015 status, CURRENT_SPEC and canonical relevant
source/ref/Change/transfer/known-call/occurrence rules, M361 design/adjudication.
KEEP original R/O/inc/A/D, explicit correct-D scoped ref, four-field currentness,
whole nonCopy transfer and body-sensitive calls, matched 0–5 explicit cleanup.
DEFER general effect/call/borrow/alias/CFG/owner/ABI/FFI, privacy and final syntax,
DI015. Intentionally replace canonical rules: NONE. Early Surface/M0/full M9
history not exhaustively available: absence of access is not absence of proposals.
No Ledger or normative selection update for these empirical findings.

Decision after unchanged-compiler measurement: HOLD; no compiler edit. New ON
rejects first registered helper in chronological_changes; nine direct captures
exist only in an UNSEALED registration attempt, not an owned success. Legacy
owned8 calibration has all20 heap fields continuous at helper boundaries, while
B A/D current places migrate 30/33→95/96 through M, and B/dst responsibility
becomes AGGREGATE carrier at J.entry. A correct temporal custody verifier must
account for those checked nonCopy operations; neither skipping the call nor
ignoring A/D carrier differences supplies that proof. owner_node_shape verifies
structural member identity, not the missing temporal splice. A sound finite
source-custody rederivation was not established in this pilot; no permissive
fallback or invented authority was implemented. This is not a claim that a
future bounded private custody witness is impossible or needs new language law.
