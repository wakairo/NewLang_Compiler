"""Emit machine-readable external comparison, never interpret UNSUPPORTED as match."""
import json
import random
from dataclasses import asdict
import abstract
import exact
from generate import LENGTHS, make, paired_mutations
from neutral import digest


def compare(label, case, seed, budget=12):
    o = exact.run(case)
    a = abstract.run(case, event_budget=budget)
    if o.decision == "SEMANTIC_REJECT" and a.decision == "ACCEPT":
        raise AssertionError(f"CRITICAL FALSE ACCEPT {label} {digest(case)}")
    if o.decision == "ACCEPT" and a.decision == "SEMANTIC_REJECT":
        raise AssertionError(f"UNJUSTIFIED SEMANTIC REJECTION {label}")
    return {"label": label, "motif": case.motif, "seed": seed,
            "field_change_count": sum(type(x).__name__ == "Change" for x in case.operations),
            "sequence_sha256": digest(case),
            "oracle_supported": o.supported, "oracle_decision": o.decision,
            "abstract_supported": a.supported, "abstract_decision": a.decision,
            "first_event": {"exact": o.first_event, "abstract": a.first_event},
            "error_rule": {"exact": o.rule, "abstract": a.rule},
            "agreement": o.supported and a.supported and o.decision == a.decision}


def main():
    cases = []
    for motif in ("device_directory", "intrusive_scheduler"):
        for n in LENGTHS:
            seed = 20261012
            c = make(motif, n, seed)
            cases.append(compare(f"positive_{motif}_{n}", c, seed))
            if n == 12:
                cases.append(compare(f"budget8_{motif}", c, seed, budget=8))
                for name, poisoned in paired_mutations(c).items():
                    cases.append(compare(f"negative_{motif}_{name}", poisoned, seed))
    rng = random.Random(20261012)
    for i in range(256):
        motif = rng.choice(("device_directory", "intrusive_scheduler"))
        n = rng.choice(LENGTHS)
        seed = rng.getrandbits(32)
        c = make(motif, n, seed)
        cases.append(compare(f"generated_safe_{i}", c, seed))
        ix = rng.randrange(n) * 3
        from dataclasses import replace
        ops = list(c.operations)
        ops[ix] = replace(ops[ix], domain="counterfeit:D")
        cases.append(compare(f"generated_wrong_D_{i}", replace(c, operations=tuple(ops)), seed))
    total = {"case_count": len(cases),
             "critical_false_accept": sum(x["oracle_decision"] == "SEMANTIC_REJECT" and x["abstract_decision"] == "ACCEPT" for x in cases),
             "unsupported_pairs_not_counted_as_agreement": sum(not x["oracle_supported"] or not x["abstract_supported"] for x in cases),
             "precision_cases": sum(x["abstract_decision"] == "PRECISION" for x in cases),
             "semantic_reject_cases": sum(x["oracle_decision"] == "SEMANTIC_REJECT" for x in cases),
             "both_accept": sum(x["oracle_decision"] == x["abstract_decision"] == "ACCEPT" for x in cases)}
    print(json.dumps({"authority": "CLASS O / H; synthetic neutral IR; no real source admission",
                      "generator_seed": 20261012, "summary": total, "cases": cases},
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
