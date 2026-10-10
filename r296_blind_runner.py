#!/usr/bin/env python3
"""R296 fresh independently authored adverse source corpus. No P fixture/read."""
import os,sys,json,hashlib,pathlib,subprocess
CASES=("positive_permute_locator","positive_first_locator","callee_twice",
 "callee_drop","caller_donor_reuse","caller_duplicate_actual",
 "wrong_releasing_grant","wrong_releasing_domain",
 "crossed_grants_pure","third_incorrect_type","wrong_stable_loan")
HEADER="""struct Node {
 next: Option<ptr<Node>>,
 prev: Option<ptr<Node>>,
 child: Option<ptr<Node>>,
 payload: u8,
}
struct OwnerCar {
 locator: ptr<Node>,
 permit: Allocation,
 realm: LifetimeDomain,
}
struct PairOwners {
 first: OwnerCar,
 second: OwnerCar,
}
"""
def cleanup(i,loc=None,grant=None,domain=None,tag=""):
 p=loc or f"p_{i}"; a=grant or f"a_{i}"; d=domain or f"d_{i}"
 return f"""let e_{tag}{i} = loan_exclusive_read({d}) {{ |end_{tag}{i}| destroy({p}, end_{tag}{i}) }};
let raw_{tag}{i} = erase_slot<Node>(e_{tag}{i});
finalize_domain({d});
deallocate({a}, raw_{tag}{i});"""
def node(i):
 return f"""let vacant_{i} = into_slot<Node>(raw);
let d_{i} = lifetime_domain();
let p_{i} = loan_read(d_{i}) {{ |st_{i}|
  initialize(vacant_{i}, Node {{
   next: Option<ptr<Node>>::None,
   prev: Option<ptr<Node>>::None,
   child: Option<ptr<Node>>::None,
   payload: u8({20+i})
  }}, st_{i})
}};"""
def full(case):
 agr=("a_3","a_2") if case in ("crossed_grants_pure","wrong_releasing_grant") else ("a_2","a_3")
 dom=("d_3","d_2") if case=="wrong_releasing_domain" else ("d_2","d_3")
 third={"positive_first_locator":"p_2","third_incorrect_type":"d_0"}.get(case,"p_0")
 parts=[f"let owner_two = OwnerCar {{ locator: p_2, permit: {agr[0]}, realm: {dom[0]} }};",
        f"let owner_three = OwnerCar {{ locator: p_3, permit: {agr[1]}, realm: {dom[1]} }};",
        f"let packed = route(owner_two, {'owner_two' if case=='caller_duplicate_actual' else 'owner_three'}, {third});"]
 if case=="caller_donor_reuse": parts.append("let after_move = owner_two;")
 parts.append("let PairOwners { first, second } = packed;")
 if case=="wrong_stable_loan":
  parts.append("let illegitimate = loan_read(d_0) { |stale| let bad_ref = ref_from_ptr(write, p_2, stale); unit };")
 parts.append("""let finished_first = {
let OwnerCar { locator, permit, realm } = first;
"""+cleanup(2,loc="locator",grant="permit",domain="realm",tag="f")+"""
unit
};""")
 parts.append("""let finished_second = {
let OwnerCar { locator, permit, realm } = second;
"""+cleanup(3,loc="locator",grant="permit",domain="realm",tag="s")+"""
unit
};""")
 for i in (4,1,0): parts.append(cleanup(i))
 parts.append("unit")
 return "\n".join(parts)
def branch(i,case):
 if i==5:return full(case)
 fail="\n".join([cleanup(j) for j in reversed(range(i))]+["unit"])
 return f"""match try_allocate_one<Node>() {{
None => {{ {fail} }},
Some(bundle_{i}) => {{
let OneBacking {{ allocation, raw }} = bundle_{i};
let a_{i} = allocation;
{node(i)}
{branch(i+1,case)}
}}
}}"""
def generate(case):
 if case=="callee_twice":
  body="PairOwners { first: first, second: first }"
 elif case=="callee_drop":
  body="let abandoned = second; PairOwners { first: first, second: abandoned }"
 else:
  body="PairOwners { first: first, second: second }"
 return HEADER+f"""fn route(first: OwnerCar, second: OwnerCar, marker: ptr<Node>) -> PairOwners {{
 {body}
}}
fn main() -> unit {{
 {branch(0,case)}
}}
"""
def main(binary,directory):
 directory.mkdir(exist_ok=True,parents=True)
 results=[]
 for case in CASES:
  source=generate(case); path=directory/(case+".nl")
  path.write_text(source)
  out=directory/(case+".stdout"); err=directory/(case+".stderr")
  code=124
  try:
   with out.open("wb") as stdout,err.open("wb") as stderr:
    done=subprocess.run([str(binary),str(path)],stdout=stdout,stderr=stderr,timeout=45)
    code=done.returncode
  except subprocess.TimeoutExpired:pass
  data=out.read_bytes() if out.exists() else b""
  c_output=bool(data.lstrip().startswith((b"#include",b"/*")))
  if c_output:(directory/(case+".c")).write_bytes(data)
  record=dict(case=case,source_sha256=hashlib.sha256(source.encode()).hexdigest(),exit_status=code,
              stdout_bytes=len(data),c_output=c_output,
              diagnostics=err.read_text(errors="replace")[:2500])
  results.append(record)
  print("R296_RESULT "+json.dumps(record,ensure_ascii=False),flush=True)
 (directory/"summary.json").write_text(json.dumps(results,indent=2,ensure_ascii=False))

# Independent alternate ordinary-record design.  Keep direct call and terminal
# cleanup in a flat lexical block, without source-level nested cleanup blocks.
CASES = CASES + ("hetero_good_p0","hetero_good_p2","hetero_cross_pure_repaired","hetero_cross_bad_release")
_old_full = full
_old_generate = generate
HETERONOMINAL = """struct Node {
 next: Option<ptr<Node>>,
 prev: Option<ptr<Node>>,
 child: Option<ptr<Node>>,
 payload: u8,
}
struct OwnerLeft {
 leftptr: ptr<Node>,
 leftgrant: Allocation,
 leftdomain: LifetimeDomain,
}
struct OwnerRight {
 rightptr: ptr<Node>,
 rightgrant: Allocation,
 rightdomain: LifetimeDomain,
}
struct OwnerPair {
 first: OwnerLeft,
 second: OwnerRight,
}
"""
def full(case):
 if not case.startswith("hetero_"):
  return _old_full(case)
 crossing=case in ("hetero_cross_pure_repaired","hetero_cross_bad_release")
 a_left,a_right=("a_3","a_2") if crossing else ("a_2","a_3")
 terminal_left="rightgrant" if case=="hetero_cross_pure_repaired" else "leftgrant"
 terminal_right="leftgrant" if case=="hetero_cross_pure_repaired" else "rightgrant"
 link="p_2" if case=="hetero_good_p2" else "p_0"
 lines=[
 f"let left = OwnerLeft {{ leftptr: p_2, leftgrant: {a_left}, leftdomain: d_2 }};",
 f"let right = OwnerRight {{ rightptr: p_3, rightgrant: {a_right}, rightdomain: d_3 }};",
 f"let both = ferry(left, right, {link});",
 "let OwnerPair { first, second } = both;",
 "let OwnerLeft { leftptr, leftgrant, leftdomain } = first;",
 "let OwnerRight { rightptr, rightgrant, rightdomain } = second;",
 cleanup(2,loc="leftptr",grant=terminal_left,domain="leftdomain",tag="left"),
 cleanup(3,loc="rightptr",grant=terminal_right,domain="rightdomain",tag="right")]
 for i in (4,1,0):lines.append(cleanup(i))
 lines.append("unit")
 return "\n".join(lines)
def generate(case):
 if not case.startswith("hetero_"):
  return _old_generate(case)
 return HETERONOMINAL+"""fn ferry(left: OwnerLeft, right: OwnerRight, marker: ptr<Node>) -> OwnerPair {
 OwnerPair { first: left, second: right }
}
fn main() -> unit {
"""+branch(0,case)+"\n}\n"

if __name__=="__main__":
 main(pathlib.Path(sys.argv[1]).resolve(),pathlib.Path(sys.argv[2]).resolve())
