#!/usr/bin/env python3
"""R333 independent targeted source variants, based only on the R328-derived R333 composition.

Two-edit controls intentionally move C donor cleanup after the requiring
terminal solely to expose original-B A/D identity error at that terminal.
They do NOT purport to satisfy the donor-first positive predicate.
"""
from pathlib import Path
import difflib,hashlib,json

origin=Path("r333/independent_returned.nl")
source=origin.read_text()
out=Path("r333/variants")
out.mkdir(exist_ok=True)
def unique(old,new):
    assert source.count(old)==1,(old,source.count(old))
    return source.replace(old,new)
def late(old,new):
    before,sep,after=source.rpartition(old)
    assert sep
    return before+new+after
def paired_wrong(old,new):
    s=unique(old,new)
    # Move C's own original matched cleanup past B/dst requiring terminal:
    # isolates true B mismatch at call, not a harmless PURE transport.
    cleanup="""                                            let empty_C = loan_exclusive_read(life_C) { |e_C| destroy(ptr_C, e_C) };
                                            let reclaimed_C = erase_slot<Node>(empty_C);
                                            finalize_domain(life_C);
                                            deallocate(allocation_C, reclaimed_C);
"""
    assert cleanup in s
    prefix,marker,suffix=s.rpartition(cleanup)
    s=prefix+suffix
    assert s.count("                                            release_pair(returned_pair);")==1
    return s.replace("                                            release_pair(returned_pair);",
                     "                                            release_pair(returned_pair);\n"+cleanup)
cases=[
("reconsume_B_allocation",unique(
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_B_m };",
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_B_m }; let repeated_B_allocation = allocation_B_m;")),
("reconsume_dst_allocation",unique(
 "let dst_receipt = RootReceipt { node: ptr_dst, allocation: allocation_dst, domain: life_dst };",
 "let dst_receipt = RootReceipt { node: ptr_dst, allocation: allocation_dst, domain: life_dst }; let repeated_dst_allocation = allocation_dst;")),
("reconsume_detached_holder",unique(
 "let DetachedHolder { node, grant, dom } = carried_B;",
 "let DetachedHolder { node, grant, dom } = carried_B; let second_holder = carried_B;")),
("reuse_original_B_domain_after_whole_move",unique(
 "let life_B_m = dom;", "let life_B_m = life_B;")),
("live_B_scoped_ref_during_whole_owner_receipt",unique(
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_B_m };",
 "let b_receipt = loan_read(life_B_m) { |borrowed_B| RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_B_m } };")),
("overlapping_B_loan_during_pair_consume",unique(
 "let returned_pair = convey_pair(dst_receipt, b_receipt, ptr_B);",
 "let returned_pair = loan_read(life_B_m) { |active_B| convey_pair(dst_receipt, b_receipt, ptr_B) };")),
("duplicate_pair_return_member",unique(
 "let ownership = PairedCustody { receiver: destination, detached: isolated };",
 "let ownership = PairedCustody { receiver: destination, detached: destination };")),
("returned_pair_consumed_twice",unique(
 "release_pair(returned_pair);",
 "release_pair(returned_pair); release_pair(returned_pair);")),
("drop_returned_pair_without_terminal",unique(
 "release_pair(returned_pair);",
 "unit;")),
("omit_donor_A_cleanup",late(
 """                                            let empty_A = loan_exclusive_read(life_A) { |e_A| destroy(ptr_A, e_A) };
                                            let reclaimed_A = erase_slot<Node>(empty_A);
                                            finalize_domain(life_A);
                                            deallocate(allocation_A, reclaimed_A);
""","")),
("wrong_donor_A_governing_domain",late(
 "let empty_A = loan_exclusive_read(life_A)",
 "let empty_A = loan_exclusive_read(life_C)")),
("wrong_donor_C_governing_domain",late(
 "let empty_C = loan_exclusive_read(life_C)",
 "let empty_C = loan_exclusive_read(life_A)")),
("wrong_B_original_allocation_at_requiring_terminal",paired_wrong(
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_B_m };",
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_C, domain: life_B_m };")),
("wrong_B_original_domain_at_requiring_terminal",paired_wrong(
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_B_m };",
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_C };")),
("copy_ptr_as_allocation",unique(
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_B_m };",
 "let b_receipt = RootReceipt { node: ptr_B_m, allocation: ptr_B, domain: life_B_m };")),
("early_B_release_product_counterexample",unique(
 "                                            // Original donor A,C,src are freed before B or dst.",
 "                                            release_pair(returned_pair);\n                                            // Original donor A,C,src are freed before B or dst."
).replace("                                            release_pair(returned_pair);\n                                            unit\n", "                                            unit\n")),
("C_detach_wrong_fresh_target",unique(
 "replace(direct_C@prev, Option<ptr<Node>>::Some(ptr_A))",
 "replace(direct_C@prev, Option<ptr<Node>>::Some(ptr_B_m))")),
]
# Positive R328 old H0 separate B custody is known source-valid but product-negative:
# this is deliberately NOT counted as a semantic negative.
# Explicit non-novel lawful H0 comparator is the original independent R328 source.
cases.append(("H0_original_R328_caller_custody",
              Path("r333/h0_r328_caller_custody.nl").read_text()))
assert len({n for n,_ in cases})==18
manifest=[]
for name,src in cases:
    assert src!=source
    target=out/(name+".nl")
    target.write_text(src)
    diff="".join(difflib.unified_diff(source.splitlines(True),src.splitlines(True),
                                      fromfile=str(origin),tofile=str(target)))
    (out/(name+".diff")).write_text(diff)
    manifest.append(dict(name=name,sha256=hashlib.sha256(src.encode()).hexdigest(),
                         diff=str(out/(name+".diff")),diff_lines=len(diff.splitlines())))
    print("R333_VARIANT",name,manifest[-1]["sha256"],len(diff.splitlines()))
(out/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
