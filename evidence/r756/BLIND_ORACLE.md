# R756 BLIND pre-observation oracle (author: Track R)
Compiler SHA: 3a9f4ad2a70d9aa1ad5c18bb35c50819d090c1d2; canonical Draft17.34, §16.3.8, §17.1, §18.1.
Executed only by a new R-only branch + GitHub Actions, never P-equivalent runtime.

Positive A h_first.nl: Reach (H) before Ticket (T). Ticket field index 0=incoming, index 1=outgoing.
Source value obligations: first incoming 17->63, old_first=17; second outgoing 91->17, old_second=91; first sibling outgoing=26; second sibling incoming=44.
Two distinct actual Reach original allocation sites. Failure-world 0: zero frees. Failure-world 1: front free once. Failure-world 2: back owned once by returned LiveTail then freed once, front freed once.
Root/incarnation/backing/Allocation/Domain must be distinct between two heap originals and never derived from Ticket.
Both actual helper calls must be definition-time valid, nonescaping, preserve sibling facts, and use pre-write old Copy in caller.

Positive B t_first_interposed.nl: Metric (T) before Stem (H), with recalibrate ordinary fn between declarations.
Metric index 0=top and index 1=base. first top 38->82 and old_first=38, second base 70->38 and old_second=70; unchanged first base=64, second top=11.
Stem first payload23; second payload29; two separate original allocations; 0/1/2 failure cleanup as A.

For each eligible input, expect semantic source admission and true successful source CHECK for the selected bounded Stage1 only.
If checker precision or parser grammar cannot represent a case, distinguish UNSUPPORTED/INCONCLUSIVE from safety rejection.
Public owned validator may not issue joined mixed certificate; do not conflate CHECK0 with its proof.
Mixed C17 is intentionally unsupported: every such request must refuse before writing any bytes, retaining pre-existing sentinel untouched.
Single H-only and single T-only independent baselines expected eligible within their previous bounded profiles.

Frozen negative register:
- n_bad_second_t_type.nl: second T declaration contains usize not u8; reject.
- n_bad_second_h_target.nl: second H points to Metric rather than itself; reject.
- n_h_t_name_collision.nl: H/T same namespace; reject.
- n_two_h_nominals.nl / n_two_t_nominals.nl: beyond closed mixed declaration count; reject for source eligibility, not general unsoundness.
- n_unknown_t_field.nl: undeclared field projection; reject.
- n_bad_uncalled_body.nl: definition-time wrong result even if uncalled; reject/rollback all registration.
- n_duplicate_function.nl: duplicate top-level fn; reject/rollback.
- n_read_write_escalation.nl: read-loan passed to write helper; semantic safety rejection if parsed, else parser unsupported.
- n_double_consume_owner.nl: original nonCopy Allocation/Domain used twice; reject.
- n_missing_failure_cleanup.nl: front original on second allocation failure is not discharged; reject.
Negative reject stage must be distinguished parser vs semantic vs CHECK/public validator/backend.
All fail-closed runs must preserve sentinel and context; no partial type/root/function publication.
No P source, oracle, PR diff or report consulted to author these cases. Draft17.34 sketches NOT used as test sources.
