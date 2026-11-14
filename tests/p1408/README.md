# P1408 C833: same-original nonowning ptr Copy at IF

Pinned parent: main `6e351e76ee8ef03cb77717375431462166e3b21a`,
Draft17.37. Source/oracle freeze and the original baseline P13 rejection are
recorded in [Issue #1408](https://github.com/wakairo/NewLang_Compiler/issues/1408).
P1406 G0 and its original RED remain unchanged.

S returns the same live original through initialize's ptr and a real
Domain-scoped write-ref-to-ptr roundtrip. Both arms must preserve the complete
source-issued ptr facts and unchanged common-prefix state. The condition is
not used to prove original identity. N returns different originals and must
retain conservative P13 rejection; the pointer-producing expression itself is
not classified as semantically invalid.

The focused test checks definition and actual CHECK, canonical owned arm
custody, both referents against the real initialize event, absence of owner/
Allocation/Domain in the result, and selected allocator failures at the two
operand copies plus parent result copy. Definition/actual failure must leave
the caller world and artifact output unchanged and allow a successful retry.

The existing two-H C17 consumer remains unsupported for this source. Its
rejection/no-clobber is tested explicitly. Native is NOT_RUN, not a capability
PASS. No unchecked syntax or unproved operation precondition is implemented.
