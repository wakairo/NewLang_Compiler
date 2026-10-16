import ast
from dataclasses import replace
from pathlib import Path
import random
import unittest
from unittest.mock import patch

import abstract
import exact
from generate import LENGTHS, alter_evidence, make, paired_mutations
from neutral import (Change, Evidence, HoldPayload, OpenLoan, Option, Pointer,
                     Unmodeled, digest)


class O1Tests(unittest.TestCase):
    def run_both(self, case, budget=12):
        a = exact.run(case)
        b = abstract.run(case, event_budget=budget)
        self.assertFalse(a.decision == "SEMANTIC_REJECT" and b.decision == "ACCEPT",
                         (case.motif, digest(case), a, b))
        return a, b

    def test_ten_safe_motif_length_pairs(self):
        for motif in ("device_directory", "intrusive_scheduler"):
            for n in LENGTHS:
                with self.subTest(motif=motif, length=n):
                    program = make(motif, n)
                    self.assertEqual(self.run_both(program), (exact.Outcome("ACCEPT"), exact.Outcome("ACCEPT")))
                    self.assertEqual(len([o for o in program.operations if isinstance(o, Change)]), n)
                    self.assertNotIn("cjson", digest(program))

    def test_wrong_and_corrupt_ninth_paired_cases(self):
        for motif in ("device_directory", "intrusive_scheduler"):
            positive = make(motif, 12)
            self.assertEqual(self.run_both(positive)[1].decision, "ACCEPT")
            for name, poisoned in paired_mutations(positive).items():
                with self.subTest(motif=motif, negative=name):
                    a, b = self.run_both(poisoned)
                    if name == "unknown_alias":
                        self.assertEqual((a.decision, b.decision), ("UNSUPPORTED", "PRECISION"))
                    else:
                        self.assertEqual((a.decision, b.decision), ("SEMANTIC_REJECT", "SEMANTIC_REJECT"))
                        self.assertNotEqual(a.rule, "OK")

    def test_budget8_to_12_is_precision_not_semantics(self):
        for motif in ("device_directory", "intrusive_scheduler"):
            c = make(motif, 12)
            exact_decision, abstract_limited = self.run_both(c, budget=8)
            self.assertEqual(exact_decision.decision, "ACCEPT")
            self.assertEqual((abstract_limited.decision, abstract_limited.rule),
                             ("PRECISION", "FIELD_EVENT_BUDGET"))
            self.assertEqual(abstract.run(c, event_budget=12).decision, "ACCEPT")

    def test_same_pointer_change_generates_new_occurrence(self):
        c = make("intrusive_scheduler", 8)
        changes = [p.after for p in c.evidence if p.kind == "Change"]
        self.assertEqual(changes[2].value, changes[4].value)
        self.assertNotEqual(changes[2].occurrence, changes[4].occurrence)
        # Corrupt the fifth direct Change to claim that old Some occurrence survives.
        ix = 4 * 3 + 1
        bad = alter_evidence(c, ix, after=replace(c.evidence[ix].after,
                                                  occurrence=changes[2].occurrence))
        self.assertEqual(self.run_both(bad)[0].decision, "SEMANTIC_REJECT")
        self.assertEqual(self.run_both(bad)[1].decision, "SEMANTIC_REJECT")

    def test_live_payload_ref_blocks_whole_option_reset(self):
        c = make("intrusive_scheduler", 3)
        held_occ = c.evidence[1].after.occurrence
        hold = HoldPayload("observe:old_some", "payload_ref_1", "schedulerX", 0, held_occ)
        ops = c.operations[:3] + (hold,) + c.operations[3:]
        proof = list(c.evidence[:3]) + [Evidence(3, hold.node, type(hold).__name__)] + list(c.evidence[3:])
        proof = [replace(x, ordinal=i) for i, x in enumerate(proof)]
        bad = replace(c, operations=ops, evidence=tuple(proof))
        results = self.run_both(bad)
        self.assertEqual([r.decision for r in results], ["SEMANTIC_REJECT", "SEMANTIC_REJECT"])
        self.assertEqual([r.rule for r in results], ["SURVIVING_OCCURRENCE_DEP", "SURVIVING_OCCURRENCE_DEP"])

    def test_stale_old_occurrence_hold_explicitly_rejected(self):
        c = make("intrusive_scheduler", 5)
        stale = c.evidence[1].after.occurrence
        hold = HoldPayload("observe:retired_some", "stale_ref", "schedulerX", 0, stale)
        insert_at = 3 * 4
        ops = c.operations[:insert_at] + (hold,) + c.operations[insert_at:]
        proof = list(c.evidence[:insert_at]) + [Evidence(insert_at, hold.node, "HoldPayload")] + list(c.evidence[insert_at:])
        bad = replace(c, operations=ops, evidence=tuple(replace(x, ordinal=i) for i, x in enumerate(proof)))
        actual = self.run_both(bad)
        self.assertEqual([r.rule for r in actual], ["STALE_PAYLOAD_HOLD", "STALE_PAYLOAD_HOLD"])

    def test_unmodeled_owner_endroot_stops_not_fake_success(self):
        c = make("device_directory", 2)
        u = Unmodeled("owner_move:unmodeled", "WholeMove+EndRoot with live loan")
        ops = c.operations[:2] + (u,) + c.operations[2:]
        cert = list(c.evidence[:2]) + [Evidence(2, u.node, "Unmodeled")] + list(c.evidence[2:])
        cert = [replace(x, ordinal=i) for i, x in enumerate(cert)]
        sample = replace(c, operations=ops, evidence=tuple(cert))
        self.assertEqual([x.decision for x in self.run_both(sample)], ["UNSUPPORTED", "UNSUPPORTED"])

    def test_abstract_never_imports_or_calls_exact(self):
        # AST-level import contract; adversarial runtime trap of exact.run.
        module_path = Path(abstract.__file__)
        tree = ast.parse(module_path.read_text())
        banned = {"exact", "semantics", "unified_ir", "newlang_checker"}
        imported = set()
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                imported.update(alias.name.split(".")[0] for alias in node.names)
            elif isinstance(node, ast.ImportFrom) and node.module:
                imported.add(node.module.split(".")[0])
        self.assertFalse(imported & banned)
        with patch.object(exact, "run", side_effect=AssertionError("oracle must not execute")):
            self.assertEqual(abstract.run(make("intrusive_scheduler", 12)).decision, "ACCEPT")

    def test_deterministic_randomized_safe_and_poison_pairs(self):
        rng = random.Random(20261012)
        for i in range(256):
            motif = rng.choice(("device_directory", "intrusive_scheduler"))
            count = rng.choice(LENGTHS)
            seed = rng.getrandbits(32)
            sample = make(motif, count, seed)
            self.assertEqual(self.run_both(sample)[1].decision, "ACCEPT")
            ix = rng.randrange(count) * 3
            op = sample.operations[ix]
            self.assertIsInstance(op, OpenLoan)
            bad = replace(sample, operations=sample.operations[:ix] +
                          (replace(op, domain="counterfeit:D"),) + sample.operations[ix + 1:])
            a, b = self.run_both(bad)
            self.assertEqual((a.decision, b.decision), ("SEMANTIC_REJECT", "SEMANTIC_REJECT"))


if __name__ == "__main__":
    unittest.main()
