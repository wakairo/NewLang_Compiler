#!/usr/bin/env python3
"""Independent Issue 291 R second-pass source cases; never uses P fixtures."""
import json
import pathlib
import subprocess
import sys

OUT = pathlib.Path("/tmp/r291_cases")
OUT.mkdir(exist_ok=True)
HEADER = """struct Node {
    next: Option<ptr<Node>>,
    prev: Option<ptr<Node>>,
    child: Option<ptr<Node>>,
    payload: u8,
}
struct LiveRoot { p: ptr<Node>, a: Allocation, d: LifetimeDomain, }
"""
FINISH = """fn retire(root:LiveRoot)->unit {
    let LiveRoot { p, a, d } = root;
    let vacant = loan_exclusive_read(d) { |end_token| destroy(p, end_token) };
    let whole = erase_slot<Node>(vacant);
    finalize_domain(d);
    deallocate(a, whole);
    unit
}
"""
def make(mode, n=2):
    labels = ["first", "second", "third", "fourth"]
    m = min(n,4)
    fields = ", ".join(f"{labels[i]}: LiveRoot" for i in range(m))
    header = HEADER + "struct Vessel { " + fields + ", }\n"
    head = header + FINISH + "fn transit(v:Vessel)->Vessel { return v; }\n"
    if mode == "mixed_alloc":
        head += """fn exchange(v:Vessel)->Vessel {
    let Vessel { first: left, second: right } = v;
    let LiveRoot { p: lp, a: la, d: ld } = left;
    let LiveRoot { p: rp, a: ra, d: rd } = right;
    return Vessel {
        first: LiveRoot { p: lp, a: ra, d: ld },
        second: LiveRoot { p: rp, a: la, d: rd }
    };
}
"""
    def cleanup(i):
        return f"""let void_{i} = loan_exclusive_read(d_{i}) {{ |ender_{i}| destroy(p_{i}, ender_{i}) }};
let raw_{i} = erase_slot<Node>(void_{i});
finalize_domain(d_{i});
deallocate(a_{i}, raw_{i});
"""
    def success_body():
        expr = "Vessel { " + ", ".join(
          f"{labels[i]}: LiveRoot {{ p: p_{i}, a: a_{i}, d: d_{i} }}"
          for i in range(m)) + " }"
        if mode == "omitted_member":
            expr = "Vessel { first: LiveRoot { p: p_0, a: a_0, d: d_0 } }"
        decl = "let vessel = " + expr + ";\n"
        if mode == "active_loan":
            # Move one active D through a returning aggregate while a stability
            # loan still protects it; source must NOT accept this.
            return f"loan_read(d_1) {{ |active| transit({expr}) }};\nunit\n"
        if mode == "duplicate_donor":
            decl += "let shipped = transit(vessel);\nlet twice = transit(vessel);\n"
            decl += "let Vessel {first: l, second: r}=shipped;\nretire(l); retire(r); unit\n"
            return decl
        if mode == "mixed_alloc":
            decl += "let shipped = exchange(vessel);\n"
        else:
            decl += "let shipped = transit(vessel);\n"
        decl += "let Vessel {" + ", ".join(f"{labels[i]}: keeper_{i}" for i in range(m)) + " } = shipped;\n"
        if mode == "lost_original":
            return decl + "retire(keeper_0);\nunit\n"
        if mode == "duplicate_receiver":
            decl += "retire(keeper_0);\nretire(keeper_0);\n"
            decl += "retire(keeper_1);\nunit\n"
            return decl
        for i in range(m):
            decl += f"retire(keeper_{i});\n"
        for i in range(m,n):
            decl += f"retire(LiveRoot {{p:p_{i}, a:a_{i}, d:d_{i}}});\n"
        return decl+"unit\n"
    def branch(i):
        prefix = f"""match try_allocate_one<Node>() {{
None => {{
{''.join(cleanup(j) for j in reversed(range(i)))}
unit
}},
Some(bundle_{i}) => {{
let OneBacking {{ allocation, raw }} = bundle_{i};
let a_{i} = allocation;
let vacant_{i} = into_slot<Node>(raw);
let d_{i} = lifetime_domain();
let p_{i} = loan_read(d_{i}) {{ |stable_{i}|
  initialize(vacant_{i}, Node {{
    next: Option<ptr<Node>>::None,
    prev: Option<ptr<Node>>::None,
    child: Option<ptr<Node>>::None,
    payload: u8({i+10})
  }}, stable_{i})
}};
"""
        body=success_body() if i==n-1 else branch(i+1)
        return prefix+body+"},\n}\n"
    return head+"fn main()->unit {\n"+branch(0)+"}\n"

CASES = [
  ("original_2", "plain", 2),
  ("duplicate_donor", "duplicate_donor", 2),
  ("duplicate_receiver", "duplicate_receiver", 2),
  ("mixed_allocation_release", "mixed_alloc", 2),
  ("lost_original", "lost_original", 2),
  ("omitted_owner", "omitted_member", 2),
  ("active_domain_loan", "active_loan", 2),
  ("original_3", "plain", 3),
  ("original_5", "plain", 5),
]
rows=[]
for name, mode, n in CASES:
    p=OUT/(name+".nl")
    p.write_text(make(mode,n))
    proc=subprocess.run([sys.argv[1],str(p)],text=True,capture_output=True,timeout=90)
    diagnostic=proc.stderr[-1300:].replace("\n"," / ")
    print("R291_CASE",json.dumps({"case":name,"count":n,"exit":proc.returncode,
          "c_generated":bool(proc.stdout.strip()),"diag":diagnostic},ensure_ascii=False),flush=True)
    rows.append({"case":name,"exit":proc.returncode,"diag":diagnostic})
(OUT/"results.json").write_text(json.dumps(rows,indent=2))
