"""Stdlib measured synthetic corpus, no original source/authority import."""
import json
import platform
import statistics
import sys
import time
import tracemalloc

import full_snapshot
import persistent_delta
from generate import fixtures,poison,POISON_KINDS
from model import stable_digest


def benchmark(fun,p,reps=31):
    timings=[]
    peaks=[]
    for i in range(reps):
        tracemalloc.start()
        start=time.perf_counter_ns()
        result=fun(p)
        elapsed=time.perf_counter_ns()-start
        _,peak=tracemalloc.get_traced_memory()
        tracemalloc.stop()
        assert result.status=="ACCEPT"
        timings.append(elapsed)
        peaks.append(peak)
    return {"median_ns":int(statistics.median(timings)),
            "median_tracemalloc_peak_bytes":int(statistics.median(peaks)),
            "min_ns":min(timings),"max_ns":max(timings)}


def main():
    cases=[]
    diffs=[]
    for motif in ("directory","scheduler"):
        for count in (2,4,9,12):
            p=fixtures(motif,count)
            a=full_snapshot.check(p)
            b=persistent_delta.check(p)
            assert a.status==b.status=="ACCEPT"
            cases.append({"motif":motif,"changes":count,"seed":p.seed,
              "trace_sha256":stable_digest(p),
              "snapshot":{"status":a.status,"cloned_slots":a.cloned_slots,
                          "visited_slots":a.visited_slots,"peak_semantic_slots":a.peak_stored_slots,
                          **benchmark(full_snapshot.check,p)},
              "delta":{"status":b.status,"new_delta_slots":b.cloned_slots,
                          "shared_versions":b.shared_nodes,"visited_slots":b.visited_slots,
                          "peak_semantic_slots":b.peak_stored_slots,
                          **benchmark(persistent_delta.check,p)}})
        for kind in POISON_KINDS:
            bad=poison(fixtures(motif,12,same_target=(motif=="scheduler")),kind)
            a,b=full_snapshot.check(bad),persistent_delta.check(bad)
            assert a.status==b.status=="SEMANTIC_REJECT",(motif,kind,a,b)
            diffs.append({"motif":motif,"case":kind,"sha256":stable_digest(bad),
                          "snapshot":a.status,"snapshot_first":a.first,"snapshot_rule":a.rule,
                          "delta":b.status,"delta_first":b.first,"delta_rule":b.rule})
        for mode in ("hidden","recorded","copy","unknown"):
            p=fixtures(motif,2,scalar_gap=mode)
            a,b=full_snapshot.check(p),persistent_delta.check(p)
            expected={"hidden":"SEMANTIC_REJECT","recorded":"ACCEPT",
                      "copy":"ACCEPT","unknown":"PRECISION"}[mode]
            assert a.status==b.status==expected,(mode,a,b)
            diffs.append({"motif":motif,"case":"scalar_gap_"+mode,
                          "sha256":stable_digest(p),"snapshot":a.status,
                          "snapshot_first":a.first,"snapshot_rule":a.rule,
                          "delta":b.status,"delta_first":b.first,"delta_rule":b.rule})
    import unittest
    suite=unittest.defaultTestLoader.discover('.',pattern='test_o5.py')
    ret=unittest.TextTestRunner(stream=sys.stderr,verbosity=0).run(suite)
    if not ret.wasSuccessful():raise SystemExit(1)
    payload={"status":"SYNTHETIC_ONLY_NOT_C_AUTHORITY","python":sys.version.split()[0],
             "platform":platform.platform(),"seed":20261012,"bench_reps":31,
             "test_methods":ret.testsRun,"passed":ret.wasSuccessful(),
             "positive_pairs":len(cases),"adversarial_cases":len(diffs),
             "critical_snapshot_reject_delta_accept":0,
             "scope":{"five_originals":True,"fixed_fields_per_root":4,
                      "unrecorded_same_value_scalar_refused":True,
                      "live_C_compiler_executed":False,
                      "source_issued_authority":False},
             "positive_cases":cases,"negative_cases":diffs}
    print(json.dumps(payload,sort_keys=True,indent=2))

if __name__=="__main__":main()
