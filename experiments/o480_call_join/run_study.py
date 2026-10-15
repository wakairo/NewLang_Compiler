"""Finite O482 deterministic report, bounded benchmark (stdlib only)."""
from __future__ import annotations
import json,platform,sys,time,tracemalloc,statistics
from dataclasses import asdict
import unittest
from ir import fixture,digest
import snapshot,delta,concrete
from test_o482 import cases


def measure(call,p,repetitions=51):
    t=[];m=[]
    for _ in range(repetitions):
        tracemalloc.start()
        start=time.perf_counter_ns()
        r=call(p)
        t.append(time.perf_counter_ns()-start)
        _,peak=tracemalloc.get_traced_memory()
        tracemalloc.stop()
        m.append(peak)
    return {'repeats':repetitions,'median_wall_ns_traced':int(statistics.median(t)),
            'median_peak_tracemalloc_bytes':int(statistics.median(m)),'first_status':r.status,
            'min_wall_ns_traced':min(t),'max_wall_ns_traced':max(t)}


def main():
    outcome=[]; disagreement=[];status_counts={}
    for name,(p,expected) in cases().items():
        a,b,c=snapshot.check(p),delta.check(p),concrete.run(p)
        expected_c='ACCEPT' if name in ('cap_one','missing_reset_summary') else expected
        assert (a.status,b.status,c.status)==(expected,expected,expected_c),(name,a.reason,b.reason,c.reason)
        if expected=='ACCEPT':assert a.states==b.states==c.states,name
        if c.status=='SEMANTIC_REJECT' and (a.status=='ACCEPT' or b.status=='ACCEPT'):
            disagreement.append(name)
        outcome.append({'name':name,'input_sha256':digest(p),'expected':expected,
                        'concrete':{'status':c.status,'reason':c.reason,'successor_worlds':len(c.states)},
                        'snapshot':{'status':a.status,'reason':a.reason,'new_cloned_slots':a.clones,'records':a.records,'estimated_retained_slots':a.slots,'visited_slots':a.visits},
                        'delta':{'status':b.status,'reason':b.reason,'new_updates':b.slots,'records':b.records,'visited_slots_with_diagnostic_materialization':b.visits}})
        status_counts[expected]=status_counts.get(expected,0)+1
    highlights=['named_call','direct_inline','old_option_some_return','post_join_whole_change','sibling_mutation_not_in_summary','conditional_none_after_join','field_dependent_ref_survives']
    timing={}
    for name in highlights:
        p=cases()[name][0]
        timing[name]={'snapshot':measure(snapshot.check,p), 'delta':measure(delta.check,p)}
    suite=unittest.defaultTestLoader.discover('.',pattern='test_o482.py')
    import io
    result=unittest.TextTestRunner(stream=io.StringIO(),verbosity=0).run(suite)
    assert result.wasSuccessful(),(result.failures,result.errors)
    doc={'authority':'Class O/H synthetic only; no source-issued C authority or real P acceptance',
         'compiler_main':'d6e603e12591c2ff39e3e1b77c784681fec66033',
         'canonical':'Draft17.30','canonical_blob':'db8fe08f811acc8286f2d37e9d3229b5d7a67067',
         'python':sys.version.split()[0],'platform':platform.platform(),
         'seed':48220261015,'unit_test_methods':result.testsRun,
         'matrix_size':len(outcome),'matrix_statuses':status_counts,
         'generated_sample_count':320,'formal_mapping_permutations':96,
         'unsafe_concrete_reject_abstract_accept':disagreement,
         'example_minimal_stale_ref':{'positive_source_pattern':'None=>unit, Some(q)=>set_link(tail@next,Some(q))',
            'counterexample':'tail.next current ValueFact F0 (or original parent RF0) supports surviving ref; in Some branch replace creates F1/RF1; old F0/RF0 remains only on None branch; using joined old ref is invalid',
            'example_digest':digest(cases()['field_dependent_ref_survives'][0])},
         'cases':outcome,'benchmark_method':'51 Python repeats with tracemalloc active; tiny synthetic models; not a C implementation comparison',
         'benchmarks':timing,
         'model_limits':['two lexical originals only','one known body Change/Reset in Some arm','No actual NewLang parser/checker','no dynamic heap issuance','no runtime/FFI','no generic CFG/callgraph effects','diagnostic B full world reconstruction contributes to memory','not proof of same-actual multi-formal general alias safety']}
    print(json.dumps(doc,sort_keys=True,indent=2))

if __name__=='__main__':main()
