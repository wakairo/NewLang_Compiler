#!/usr/bin/env python3
"""Track R #250: independent source perturbations derived only from pinned canonical Draft.
No production fixtures, implementation diagnostics or prior tests are inputs.
"""
from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
CANON = ROOT / "docs/reference/NewLang_v0_spec_Draft17_30.md"
OUT = Path(__file__).resolve().parent / "sources"
text = CANON.read_text(encoding="utf-8")
segment = text.split("### 3.2b.3 Exact five-site full source-shaped positive witness",1)[1]
match = re.search(r"~~~newlang\n(.*?)\n~~~", segment, re.S)
assert match, "canonical five-root source missing"
base = match.group(1).rstrip() + "\n"
assert base.count("match try_allocate_one<Node>()") == 5
assert base.count("Some(bundle_") == 5
assert base.count("fn main() -> unit") == 1

def once(code, old, new):
    count = code.count(old)
    if count != 1:
        raise ValueError(f"expected one replacement, got {count}: {old!r}")
    return code.replace(old, new, 1)

def insert_before(code, anchor, payload):
    return once(code, anchor, payload + anchor)

def swap_blocks(code, start_a, start_b, start_c):
    a,b,c = (code.index(x) for x in (start_a,start_b,start_c))
    assert a < b < c
    return code[:a] + code[b:c] + code[a:b] + code[c:]

def swap_last_match_arms(code):
    marker = "match try_allocate_one<Node>() {  // independent allocation site 5: dst"
    at = code.index(marker)
    opening = code.index("{", at)
    def brace_end(opening):
        level = 0
        for i in range(opening,len(code)):
            if code[i] == "{": level += 1
            elif code[i] == "}":
                level -= 1
                if level == 0: return i
        raise ValueError("unbalanced")
    end = brace_end(opening)
    body = code[opening+1:end]
    nm = re.search(r"\n([ ]*)None => \{",body)
    sm = re.search(r"\n([ ]*)Some\(bundle_dst\) => \{",body)
    assert nm and sm and nm.start() < sm.start()
    def arm(stopmatch):
        brace = body.index("{",stopmatch.start())
        depth = 0
        for i in range(brace,len(body)):
            if body[i] == "{": depth += 1
            elif body[i] == "}":
                depth -= 1
                if depth == 0:
                    # Include the comma if present
                    return i+2 if body[i+1:i+2] == "," else i+1
        raise ValueError("bad arm")
    n0, n1 = nm.start()+1, arm(nm)
    s0, s1 = sm.start()+1, arm(sm)
    assert n1 <= s0
    swapped = body[:n0]+body[s0:s1]+body[n1:s0]+body[n0:n1]+body[s1:]
    return code[:opening+1]+swapped+code[end:]

cases = []
def add(name, expected, question, code):
    assert code != "" and code.count("struct ") >= 1
    cases.append(dict(name=name,expected=expected,question=question,source=code))

add("P00_canonical", "ACCEPT", "Full five-site and all six success worlds witness",base)
add("M01_alpha_nominal","SAME_AS_P00","Alpha-renaming stable nominal type identity must not affect semantics",re.sub(r"\bNode\b","GraphH",base))
add("M02_swap_A_siblings","SAME_AS_P00","A.prev and A.next write order independent if scope-finished, per-field ProjectionIds",swap_blocks(base,
    "                                            let old_A_prev =",
    "                                            let old_A_next =",
    "                                            let old_B_prev ="))
alias = insert_before(base,
    "                                            let old_src_child =",
    "                                            let copied_A_token = ptr_A;\n")
alias = once(alias,
    "Option<ptr<Node>>::Some(ptr_A))\n                                            };\n                                            let old_A_prev",
    "Option<ptr<Node>>::Some(copied_A_token))\n                                            };\n                                            let old_A_prev")
add("M03_copy_alias","SAME_AS_P00","Copy ptr token alias never mints an original owner",alias)
add("M04_permute_last_arms","SAME_AS_P00","Some-first/None-first ordering preserves each independently-checked allocation world",swap_last_match_arms(base))
add("M05_swap_B_siblings","SAME_AS_P00","B.prev and B.next changes use separate projection IDs",swap_blocks(base,
    "                                            let old_B_prev =",
    "                                            let old_B_next =",
    "                                            let old_C_prev ="))

add("N01_wrong_domain_B_read","REJECT_SEMANTIC","An A ptr must not be treated as D_B current even though the H type matches",once(base,
    "ref_from_ptr(write, ptr_B, stable_B_prev)",
    "ref_from_ptr(write, ptr_A, stable_B_prev)"))
add("N02_wrong_domain_src_write","REJECT_SEMANTIC","D_src stability cannot authorize write reference for C",once(base,
    "ref_from_ptr(write, ptr_src, stable_src_child)",
    "ref_from_ptr(write, ptr_C, stable_src_child)"))
# Wrong ending authority; same nominal H, but different original root.
add("N03_destroy_wrong_original_D","REJECT_SEMANTIC","An ending_B exclusive ref cannot end A's typed root",once(base,
    "destroy(ptr_B, ending_B)\n                                            };\n                                            let full_B",
    "destroy(ptr_A, ending_B)\n                                            };\n                                            let full_B"))
add("N04_cross_deallocate","REJECT_SEMANTIC","Allocation_A and B's full raw must not be accepted as same BackingRegion",once(base,
    "deallocate(allocation_B, full_B);\n                                            let empty_A",
    "deallocate(allocation_A, full_B);\n                                            let empty_A"))
add("N05_double_release","REJECT_SEMANTIC","Second use of same original nonCopy allocation/full raw must be rejected",once(base,
    "deallocate(allocation_dst, full_dst);\n                                            let empty_C",
    "deallocate(allocation_dst, full_dst);\n                                            deallocate(allocation_dst, full_dst);\n                                            let empty_C"))
add("N06_moved_allocation_reused","REJECT_SEMANTIC","Moving original allocation does not leave previous binding owning it",once(base,
    "deallocate(allocation_dst, full_dst);\n                                            let empty_C",
    "let moved_allocation_dst = allocation_dst;\n                                            deallocate(allocation_dst, full_dst);\n                                            let empty_C"))
add("N07_stale_after_endroot","REJECT_SEMANTIC","Even with D_B alive, ptr_B does not permit safe read after EndRoot",once(base,
    "destroy(ptr_B, ending_B)\n                                            };\n                                            let full_B",
    "destroy(ptr_B, ending_B)\n                                            };\n                                            loan_read(life_B) { |stable_stale|\n                                                let post_end = ref_from_ptr(read, ptr_B, stable_stale);\n                                                unit\n                                            };\n                                            let full_B"))
add("N08_escape_stability_loan","REJECT_SEMANTIC","A D-scoped ref cannot be returned outside its loan closure",insert_before(base,
    "                                            let empty_dst =",
    "                                            let escaping_ref = loan_read(life_A) { |s_ephemeral|\n                                                ref_from_ptr(read, ptr_A, s_ephemeral)\n                                            };\n"))
add("N09_read_to_write","REJECT_SEMANTIC","read authority cannot write A.next",once(base,
    "ref_from_ptr(write, ptr_A, stable_A_next)",
    "ref_from_ptr(read, ptr_A, stable_A_next)"))
add("N10_ptr_projection","REJECT_SOURCE","ptr_A is not a ref-base and cannot use @prev",once(base,
    "replace(w_A_prev@prev,",
    "replace(ptr_A@prev,"))
add("N11_payload_projection","REJECT_SOURCE","Heap ref @payload is not in closed pointer-field profile",once(base,
    "replace(w_B_next@next,",
    "replace(w_B_next@payload,"))
add("N12_unknown_projection","REJECT_SOURCE","Unknown field cannot alias/borrow a valid sibling",once(base,
    "replace(w_C_prev@prev,",
    "replace(w_C_prev@ghost,"))
# First four site grants exist in fifth None world; a lost B allocation is never implicitly finalized.
site5 = base.index("match try_allocate_one<Node>() {  // independent allocation site 5: dst")
suc5 = base.index("Some(bundle_dst) =>",site5)
prior5 = base[:site5]
none5 = base[site5:suc5]
after5 = base[suc5:]
none5 = once(none5,"deallocate(allocation_B, full_B);","unit;")
add("N13_failure_world_B_leak","REJECT_SEMANTIC","At fifth failure, earlier B Allocation remains nonDiscardable without deallocate",prior5+none5+after5)
add("N14_not_yet_existing_dst","REJECT_SEMANTIC","In fifth None branch, success-only ptr_dst and D_dst must be unavailable",
    prior5+once(base[site5:suc5],
    "                                            let empty_C =",
    "                                            let phantom_dst = ptr_dst;\n                                            let empty_C =")+after5)
# Positive safety-control: changes product link oracle, not memory safety.
add("P01_wrong_cjson_policy","ACCEPT_OR_PRECISION","Swapping A.prev projection to A.next corrupts app topology, not memory safety",
    once(base,"replace(w_A_prev@prev,","replace(w_A_prev@next,"))
# Valid repeat store of the same Copy ptr; identical address does not retain old Option occurrence.
write_again = ("                                            let old_A_next_again = loan_read(life_A) { |stable_A_next_again|\n"
    "                                                let w_A_next_again = ref_from_ptr(write, ptr_A, stable_A_next_again);\n"
    "                                                replace(w_A_next_again@next,\n"
    "                                                    Option<ptr<Node>>::Some(ptr_B))\n"
    "                                            };\n")
add("P02_reset_same_ptr","ACCEPT_OR_PRECISION","Same pointer payload value is a new occurrence on fresh replace",
    insert_before(base,"                                            let old_B_prev =",write_again))

# Sixth allocation is syntactically beyond finite profile but cleans its original claims in both worlds.
sixth = """                                            match try_allocate_one<Node>() {
                                                None => { unit },
                                                Some(extra_bundle) => {
                                                    let OneBacking { allocation, raw } = extra_bundle;
                                                    let extra_vacant = into_slot<Node>(raw);
                                                    let extra_life = lifetime_domain();
                                                    let extra_ptr = loan_read(extra_life) { |extra_stable|
                                                        initialize(extra_vacant, Node {
                                                            next: Option<ptr<Node>>::None,
                                                            prev: Option<ptr<Node>>::None,
                                                            child: Option<ptr<Node>>::None,
                                                            payload: u8(6)
                                                        }, extra_stable)
                                                    };
                                                    let extra_empty = loan_exclusive_read(extra_life) { |extra_ending|
                                                        destroy(extra_ptr, extra_ending)
                                                    };
                                                    let extra_raw = erase_slot<Node>(extra_empty);
                                                    finalize_domain(extra_life);
                                                    deallocate(allocation, extra_raw);
                                                    unit
                                                },
                                            };
"""
add("N15_sixth_fallible_site","REJECT_SOURCE","A sixth independent fallible site is out of adopted closed profile",
    insert_before(base,"                                            let empty_dst =",sixth))

OUT.mkdir(parents=True, exist_ok=True)
manifest = []
for case in cases:
    payload = case.pop("source").encode("utf-8")
    path = OUT / (case["name"] + ".nl")
    path.write_bytes(payload)
    manifest.append({**case,"file":str(path.relative_to(ROOT)),
                     "sha256":hashlib.sha256(payload).hexdigest(),
                     "bytes":len(payload),
                     "allocation_site_count":payload.count(b"match try_allocate_one<")})
(OUT.parent/"manifest.json").write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
for row in manifest:
    print(row["name"],row["sha256"],row["allocation_site_count"],row["expected"])
