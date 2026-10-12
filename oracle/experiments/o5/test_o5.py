"""O5 independent synthetic tests; neither verifier uses the other's verdict."""
import ast
from dataclasses import replace
from pathlib import Path
import random
import unittest
from unittest.mock import patch

import full_snapshot
import persistent_delta
from generate import POISON_KINDS, fixtures, poison
from model import Cell, Step, Value, field_key, stable_digest


class O5Tests(unittest.TestCase):
    def both(self, p, expect):
        a=full_snapshot.check(p)
        b=persistent_delta.check(p)
        self.assertEqual((a.status,b.status),(expect,expect),
                         (p.motif,stable_digest(p),a,b))
        self.assertFalse(a.status=="SEMANTIC_REJECT" and b.status=="ACCEPT")
        return a,b

    def test_8_structurally_distinct_length_pairs(self):
        for motif in ("directory","scheduler"):
            for length in (2,4,9,12):
                with self.subTest(motif=motif,n=length):
                    p=fixtures(motif,length)
                    a,b=self.both(p,"ACCEPT")
                    self.assertEqual(len(p.originals),5)
                    self.assertEqual(len(p.initial),25)
                    self.assertEqual(len(p.checked),length)
                    self.assertEqual(a.cloned_slots,25*length)
                    self.assertEqual(b.cloned_slots,2*length)
                    self.assertEqual(b.shared_nodes,length)
                    self.assertEqual(a.visited_slots,b.visited_slots)

    def test_minimal_p350_unrecorded_identical_scalar_value(self):
        p=fixtures("directory",2,scalar_gap="hidden")
        self.assertEqual([x.kind for x in p.steps],["change","change","change"])
        self.assertEqual(len(p.checked),2)
        hidden=p.steps[1]
        self.assertEqual(hidden.field,3)
        previous=p.checked[0].after
        original=dict(previous)[field_key(hidden.root,hidden.field)]
        self.assertIsInstance(original,Cell)
        self.assertEqual(hidden.value,original.value)
        # A stand-alone scalar primitive has a known legal typed target,
        # correct original Domain, derived write ref, scope and same numerical u8.
        root=next(o for o in p.originals if o.name==hidden.root)
        self.assertEqual(hidden.read_domain,root.domain)
        self.assertEqual(hidden.ref_incarnation,root.incarnation)
        self.assertEqual(hidden.projection,3)
        self.assertTrue(hidden.scope_closed and hidden.write_ref and hidden.read_access)
        self.assertEqual(hidden.value.scalar,original.value.scalar)
        # The SECOND ordinary checked link primitive remains locally coherent:
        second=p.checked[1]
        before=dict(second.before)[field_key(p.steps[2].root,p.steps[2].field)]
        after=dict(second.after)[field_key(p.steps[2].root,p.steps[2].field)]
        self.assertEqual(before.fact,second.old_fact)
        self.assertEqual(after.fact,second.new_fact)
        self.assertEqual(before.occurrence,second.old_occurrence)
        self.assertEqual(after.occurrence,second.new_occurrence)
        self.assertEqual(before.place,after.place)
        # Yet the first published checkpoint's scalar fact is not current
        # before event2, and no checked scalar event accounts for that effect.
        first_scalar=dict(p.checked[0].after)[field_key(hidden.root,3)]
        second_scalar=dict(p.checked[1].before)[field_key(hidden.root,3)]
        self.assertEqual(first_scalar.value,second_scalar.value)
        self.assertNotEqual(first_scalar.fact,second_scalar.fact)
        a,b=self.both(p,"SEMANTIC_REJECT")
        self.assertEqual(a.first,hidden.sid)
        self.assertEqual(b.first,hidden.sid)
        self.assertEqual(a.rule,"UNRECORDED_CHANGE_OR_ORDER")
        self.assertEqual(b.rule,"SOURCE_EVENT_BIJECTION")

    def test_recorded_same_value_scalar_change_is_legal(self):
        p=fixtures("scheduler",2,scalar_gap="recorded")
        self.assertEqual(len(p.checked),3)
        self.assertEqual(p.checked[1].sid,p.steps[1].sid)
        self.both(p,"ACCEPT")

    def test_inert_copy_noop_allowed(self):
        self.both(fixtures("directory",2,scalar_gap="copy"),"ACCEPT")

    def test_unknown_does_not_mean_safe(self):
        p=fixtures("directory",2,scalar_gap="unknown")
        self.both(p,"PRECISION")
        bad=replace(p,steps=tuple(replace(s,kind="owner_move",effect="known") if s.kind=="unknown" else s for s in p.steps))
        self.both(bad,"UNSUPPORTED")

    def test_same_pointer_fresh_occurrence(self):
        p=fixtures("scheduler",12,same_target=True)
        pairs=[]
        for s,w in zip((s for s in p.steps if s.kind=="change"),p.checked):
            before=dict(w.before)[field_key(s.root,s.field)]
            after=dict(w.after)[field_key(s.root,s.field)]
            if before.value.pointer_root and before.value.pointer_root==after.value.pointer_root:
                pairs.append((w,before,after))
        self.assertTrue(pairs,"generator must exercise Some(same ptr)->Some(same ptr)")
        for w,before,after in pairs:
            self.assertNotEqual(before.occurrence,after.occurrence)
            self.assertNotEqual(before.fact,after.fact)
        self.both(p,"ACCEPT")
        self.both(poison(p,"new_occurrence_reuse"),"SEMANTIC_REJECT")

    def test_16_adversarial_categories_in_both_motifs(self):
        for motif in ("directory","scheduler"):
            positive=fixtures(motif,12,same_target=(motif=="scheduler"))
            for kind in POISON_KINDS:
                with self.subTest(motif=motif,corruption=kind):
                    self.both(poison(positive,kind),"SEMANTIC_REJECT")

    def test_unknown_and_budget_distinct_from_semantic_rejection(self):
        p=fixtures("directory",12)
        a=full_snapshot.check(p,event_budget=8)
        b=persistent_delta.check(p,event_budget=8)
        self.assertEqual((a.status,b.status),("PRECISION","PRECISION"))
        self.both(p,"ACCEPT")

    def test_deterministic_replay_and_randomized_metaregression(self):
        rng=random.Random(112211)
        for i in range(128):
            motif=rng.choice(("directory","scheduler"))
            count=rng.choice((2,4,9,12))
            seed=rng.getrandbits(32)
            p=fixtures(motif,count,seed=seed,same_target=(motif=="scheduler"))
            self.assertEqual(stable_digest(p),stable_digest(fixtures(motif,count,seed=seed,same_target=(motif=="scheduler"))))
            self.both(p,"ACCEPT")
            self.both(poison(p,rng.choice(POISON_KINDS)),"SEMANTIC_REJECT")

    def test_implementations_never_use_each_other(self):
        basedir=Path(__file__).resolve().parent
        for name in ("full_snapshot","persistent_delta"):
            tree=ast.parse((basedir/(name+".py")).read_text())
            modules=set()
            for node in ast.walk(tree):
                if isinstance(node,ast.Import):modules.update(n.name.split(".")[0] for n in node.names)
                if isinstance(node,ast.ImportFrom) and node.module:modules.add(node.module.split(".")[0])
            self.assertNotIn("persistent_delta" if name=="full_snapshot" else "full_snapshot",modules)
            self.assertNotIn("generate",modules)
            self.assertNotIn("exact",modules)
            self.assertNotIn("abstract",modules)
        p=fixtures("directory",9)
        with patch.object(full_snapshot,"check",side_effect=AssertionError("NO oracle calls")):
            self.assertEqual(persistent_delta.check(p).status,"ACCEPT")
        with patch.object(persistent_delta,"check",side_effect=AssertionError("NO delta calls")):
            self.assertEqual(full_snapshot.check(p).status,"ACCEPT")

if __name__=="__main__":unittest.main()
