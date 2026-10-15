"""Independent finite O482 cases; no acceptance of one model gates another."""
import ast
from dataclasses import replace
from pathlib import Path
from unittest.mock import patch
import random
import unittest
from ir import fixture,alter,alter_action,world,Dep,Option,pointer,digest,Original
import snapshot,delta,concrete


def cases():
    p=fixture();o=p.originals
    alias=alter_action(p,mappings=p.some_action.mappings+(('unused_alias','tail.next'),))
    stale_dep=Dep('tail.next','fact.entry.tail.next')
    old=fixture(old_some=True)
    same_locator_old=alter(p,tail_old=Option(pointer(o,'tail')))
    matrix={
      'direct_inline':(fixture(mode='inline'),'ACCEPT'),
      'named_call':(p,'ACCEPT'),
      'same_root_locator_alias_without_extra_effect':(alias,'ACCEPT'),
      'old_option_some_return':(old,'ACCEPT'),
      'same_locator_old_some_fresh_occurrence':(same_locator_old,'ACCEPT'),
      'disjoint_tail_sibling_ref':(alter(p,surviving=(Dep('tail.prev','fact.entry.tail.prev'),)),'ACCEPT'),
      'post_join_whole_change':(alter(p,post_use='write'),'ACCEPT'),
      'post_join_copy_read':(alter(p,post_use='copy'),'ACCEPT'),
      'unaffected_head_ref':(alter(p,surviving=(Dep('head.next','fact.entry.head.next'),)),'ACCEPT'),
      'known_both_some_after_join':(alter(old,post_use='read_some'),'ACCEPT'),
      'conditional_none_after_join':(alter(p,post_use='read_some'),'PRECISION'),
      'unknown_effect':(alter_action(p,unknown_effect=True),'PRECISION'),
      'missing_reset_summary':(alter_action(p,summary_effects=('Change',)),'PRECISION'),
      'cap_one':(alter(p,cap=1),'PRECISION'),
      'unsupported_runtime':(alter(p,post_use='external_ffi'),'UNSUPPORTED'),
      'extra_heap_root':(alter(p,originals=o+(Original('third','R.third','O.third','I.third','A.third','D.third','owner.third'),)),'UNSUPPORTED'),
      'bad_actual_root':(alter_action(p,actual_write='head.next',mappings=(('field','head.next'),('next','q'),('stable','D.tail'))),'SEMANTIC_REJECT'),
      'wrong_formal_actual_relation':(alter_action(p,mappings=(('field','head.next'),('next','q'),('stable','D.tail'))),'SEMANTIC_REJECT'),
      'wrong_original_domain':(alter_action(p,read_domain='D.head',mappings=(('field','tail.next'),('next','q'),('stable','D.head'))),'SEMANTIC_REJECT'),
      'duplicate_formal_keys':(alter_action(p,mappings=p.some_action.mappings+(('field','head.next'),)),'SEMANTIC_REJECT'),
      'stale_write_ref':(alter_action(p,stale_loan=True),'SEMANTIC_REJECT'),
      'mismatched_ref_root':(alter_action(p,ref_root='head'),'SEMANTIC_REJECT'),
      'mismatched_ref_incarnation':(alter_action(p,ref_incarnation='I.stale'),'SEMANTIC_REJECT'),
      'scope_exits_before_call':(alter_action(p,scope_closed=False),'SEMANTIC_REJECT'),
      'missing_write_ref':(alter_action(p,write_ref=False),'SEMANTIC_REJECT'),
      'bad_q_source':(alter_action(p,actual_next='bad_copy',mappings=(('field','tail.next'),('next','bad_copy'),('stable','D.tail'))),'SEMANTIC_REJECT'),
      'stale_q_copy':(alter_action(p,stale_q=True),'SEMANTIC_REJECT'),
      'call_exit_missing':(alter_action(p,call_exit_ok=False),'SEMANTIC_REJECT'),
      'wrong_expected_return_old':(alter_action(p,expected_old=Option(pointer(o,'tail'))),'SEMANTIC_REJECT'),
      'old_copy_result_dropped':(alter_action(p,return_old=False),'SEMANTIC_REJECT'),
      'old_some_occurrence_survives':(alter(old,surviving=(Dep('tail.next','fact.entry.tail.next','occ.tail.old'),)),'SEMANTIC_REJECT'),
      'field_dependent_ref_survives':(alter(p,surviving=(stale_dep,)),'SEMANTIC_REJECT'),
      'enclosing_root_dependent_ref_survives':(alter(p,surviving=(Dep('tail.prev','fact.entry.tail.prev',parent_fact='rootfact.entry.tail'),)),'SEMANTIC_REJECT'),
      'sibling_mutation_not_in_summary':(alter_action(p,sibling_effect=True),'SEMANTIC_REJECT'),
      'dead_original':(alter(p,originals=(o[0],replace(o[1],live=False))),'SEMANTIC_REJECT'),
      'original_allocation_alias':(alter(p,originals=(o[0],replace(o[1],allocation=o[0].allocation))),'SEMANTIC_REJECT'),
      'original_domain_alias':(alter(p,originals=(o[0],replace(o[1],domain=o[0].domain))),'SEMANTIC_REJECT'),
      'original_root_alias':(alter(p,originals=(o[0],replace(o[1],root=o[0].root))),'SEMANTIC_REJECT'),
      'source_loan_not_stable':(alter_action(p,read_stable=False),'SEMANTIC_REJECT'),
      'unsupported_body_kind':(alter_action(p,mode='unknown_target'),'UNSUPPORTED'),
    }
    return matrix

class O482Tests(unittest.TestCase):
    def test_matrix(self):
        for name,(p,expected) in cases().items():
            with self.subTest(name=name):
                a=snapshot.check(p);b=delta.check(p);ref=concrete.run(p)
                self.assertEqual((a.status,b.status,ref.status),(expected,expected,'ACCEPT' if name in ('cap_one','missing_reset_summary') else expected),(name,a.reason,b.reason,ref.reason))
                if expected=='ACCEPT':
                    self.assertEqual(a.states,b.states,name)
                    self.assertEqual(a.states,ref.states,name)
                    self.assertEqual(len(a.states),2)

    def test_local_delta_only_two_semantic_updates(self):
        p=fixture(old_some=True)
        a=snapshot.check(p);b=delta.check(p)
        self.assertEqual((a.clones,b.clones),(10,0))
        self.assertEqual((a.records,b.records),(1,1))
        self.assertEqual(b.slots,2)
        self.assertEqual(a.states,b.states)
        self.assertEqual([x[1] for x in b.states[1].returned_old],[p.tail_old])
        before=dict(world(p,'Some').cells)
        after=dict(b.states[1].cells)
        self.assertEqual(before['tail.next'].place,after['tail.next'].place)
        self.assertNotEqual(before['tail.next'].fact,after['tail.next'].fact)
        self.assertNotEqual(before['tail.next'].occurrence,after['tail.next'].occurrence)
        self.assertEqual(before['tail.prev'],after['tail.prev'])
        self.assertEqual(before['tail.child'],after['tail.child'])
        self.assertEqual(before['tail.payload'],after['tail.payload'])
        self.assertNotEqual(dict(world(p,'Some').root_facts)['tail'],dict(b.states[1].root_facts)['tail'])

    def test_same_copy_locator_must_not_conflate_occurrence(self):
        p=alter(fixture(),tail_old=Option(pointer(fixture().originals,'tail')))
        for v in (snapshot.check(p),delta.check(p)):
            before=dict(world(p,'Some').cells)['tail.next']
            after=dict(v.states[1].cells)['tail.next']
            self.assertEqual(before.value,after.value) # equal locator bytes do not imply equal semantic identity
            self.assertNotEqual(before.occurrence,after.occurrence)
            self.assertNotEqual(before.fact,after.fact)
            self.assertNotEqual(dict(world(p,'Some').root_facts)['tail'],dict(v.states[1].root_facts)['tail'])

    def test_deterministic_generated_metaregression(self):
        rng=random.Random(48220261015)
        matrix=cases()
        names=sorted(matrix)
        counts={k:0 for k in names}
        for i in range(320):
            name=rng.choice(names)
            p,expected=matrix[name]
            self.assertEqual(digest(p),digest(p))
            a=snapshot.check(p);b=delta.check(p);c=concrete.run(p)
            self.assertEqual((a.status,b.status,c.status),(expected,expected,'ACCEPT' if name in ('cap_one','missing_reset_summary') else expected),(i,name))
            if expected=='ACCEPT':self.assertEqual(a.states,b.states)
            counts[name]+=1
        self.assertTrue(all(v>=1 for v in counts.values()),[k for k,v in counts.items() if not v])

    def test_algorithms_are_independent(self):
        root=Path(__file__).resolve().parent
        for file,unwanted in [('snapshot.py','delta'),('delta.py','snapshot'),('concrete.py','snapshot')]:
            tree=ast.parse((root/file).read_text())
            imports={a.name for node in ast.walk(tree) if isinstance(node,ast.Import) for a in node.names}
            imports.update(node.module for node in ast.walk(tree) if isinstance(node,ast.ImportFrom) and node.module)
            self.assertNotIn(unwanted,imports)
        p=fixture()
        with patch.object(snapshot,'check',side_effect=AssertionError('no delegation')):
            self.assertEqual(delta.check(p).status,'ACCEPT')
        with patch.object(delta,'check',side_effect=AssertionError('no delegation')):
            self.assertEqual(snapshot.check(p).status,'ACCEPT')

    def test_formal_mapping_permutations_do_not_change_validity(self):
        rng=random.Random(482)
        p=fixture()
        for i in range(96):
            perm=list(p.some_action.mappings)
            rng.shuffle(perm)
            q=alter_action(p,mappings=tuple(perm))
            self.assertEqual((snapshot.check(q).status,delta.check(q).status,concrete.run(q).status),('ACCEPT',)*3)
            self.assertEqual(snapshot.check(q).states,delta.check(q).states)

    def test_formal_alias_semantics_not_blanket_rejected(self):
        p,_=cases()['same_root_locator_alias_without_extra_effect']
        self.assertEqual((snapshot.check(p).status,delta.check(p).status),('ACCEPT','ACCEPT'))

if __name__=='__main__':unittest.main()
