# P792 Gate0 STOP — direct H-only caller inbox

Base77d83dfc00850b14fd7c3bc95e7e3c12e34161fb, Draft17.34.
Independent source/oracle freeze8ee16d3 before execution; no compiler edit.
Material difference from the historical fixture: producer constructs local owner
then explicit RETURN; caller directly deposits into an initially None local
inbox with no policy MATCH and no artificial link pre-read. Current historical
fixture CHECK0/C170 is a control, not a new native success.

Fresh source parser succeeds, registration/definition-time main checking fails
CLI3/precision CUSTODY-PACKET-ORIGIN-PRECISION at line95:53–60, original message
argument. The readonly recipient preflight already checks correct sink mode,
current None/local identity, scoped write and alias conditions; its final
nl_packet_available_inherited(artifact,packet) requires packet_parent/match,
distinct child entry world and original producer fork relation (packet_fork.c
line142 onward). A packet produced in the same straight caller fragment lacks
that inherited fork certificate. Static LiveTail type or equal ptr cannot
replace it. Do not remove the predicate or mint the missing certificate.

This is implementation precision at source semantic registration, not parser
rejection, backend limitation or proof of an invalid ownership program. §18.1c
lists the narrow local-inbox categories; exact local RETURN + straight deposit
coverage and any issuer extension need M/C adjudication, not this task's
conditional backend-only work. No source-generated entry-owned custody artifact
was published, hence no public validator/native/0–2 malloc/OOM claim for this new
source. Scope stops before those gates.

Dropping the displaced nonDiscardable None independently fails semantic
CUSTODY-DEFINITION-OLD-NONE (line22). Reuse-after-deposit is blocked earlier by
the same positive-origin precision predicate; its intended use-after-consume
negative is NOT reached. Named-output sentinel remains byte-identical on refusal.

KEEP Draft17.34 §§18.1b/c and DI009–013; no new language/owner/source rule.
Goal Alignment: direct inbox ergonomics versus forced policy graph is a concrete
usability boundary, not measured audit-locality benefit. Product GO PAUSED/H1
HOLD. No implementation/CI/PR or other Track launch. Original evidence immutable.
