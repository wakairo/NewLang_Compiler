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
    if mode in ("mixed_alloc", "mixed_domain"):
        first = ("ra","ld") if mode == "mixed_alloc" else ("la","rd")
        second = ("la","rd") if mode == "mixed_alloc" else ("ra","ld")
        head += f"""fn exchange(v:Vessel)->Vessel {{
    let Vessel {{ first: left, second: right, third: third, fourth: fourth }} = v;
    let LiveRoot {{ p: lp, a: la, d: ld }} = left;
    let LiveRoot {{ p: rp, a: ra, d: rd }} = right;
    return Vessel {{
        first: LiveRoot {{ p: lp, a: {first[0]}, d: {first[1]} }},
        second: LiveRoot {{ p: rp, a: {second[0]}, d: {second[1]} }},
        third: third, fourth: fourth
    }};
}}
"""
    def cleanup(i):
        return f"""let void_{i} = loan_exclusive_read(d_{i}) {{ |ender_{i}| destroy(p_{i}, ender_{i}) }};
let raw_{i} = erase_slot<Node>(void_{i});
finalize_domain(d_{i});
deallocate(a_{i}, raw_{i});
"""
    def success_body():
        # The Draft17.30 three-link H profile requires this exact original
        # five-allocation graph before any custody-transfer experiment.
        wiring = ""
        if n == 5:
            edges = [(0,"child",1),(1,"prev",3),(1,"next",2),
                     (2,"prev",1),(2,"next",3),(3,"prev",2)]
            for j,(src,field,dst) in enumerate(edges):
                wiring += f"""let previous_{j} = loan_read(d_{src}) {{ |guard_{j}|
  let projected_{j} = ref_from_ptr(write, p_{src}, guard_{j});
  replace(projected_{j}@{field}, Option<ptr<Node>>::Some(p_{dst}))
}};
"""
        if mode == "direct_release":
            return wiring + "".join(
                f"retire(LiveRoot {{p:p_{i},a:a_{i},d:d_{i}}});\\n"
                for i in ([1,3,0,2,4] if n == 5 else range(n))) + "unit\\n"
        expr = "Vessel { " + ", ".join(
          f"{labels[i]}: LiveRoot {{ p: p_{i}, a: a_{i}, d: d_{i} }}"
          for i in range(m)) + " }"
        if mode == "omitted_member":
            expr = "Vessel { first: LiveRoot { p: p_0, a: a_0, d: d_0 } }"
        decl = "let vessel = " + expr + ";\n"
        if mode == "active_loan":
            # Move one active D through a returning aggregate while a stability
            # loan still protects it; source must NOT accept this.
            return wiring + f"loan_read(d_1) {{ |active| transit({expr}) }};\nunit\n"
        if mode == "duplicate_donor":
            decl += "let shipped = transit(vessel);\nlet twice = transit(vessel);\n"
            decl += "let Vessel {first: l, second: r, third: t, fourth: f}=shipped;\nretire(l); retire(r); retire(t); retire(f); unit\n"
            return wiring + decl
        if mode == "local_transport":
            decl += "let shipped = vessel;\\n"
        elif mode in ("mixed_alloc", "mixed_domain"):
            decl += "let shipped = exchange(vessel);\n"
        else:
            decl += "let shipped = transit(vessel);\n"
        decl += "let Vessel {" + ", ".join(f"{labels[i]}: keeper_{i}" for i in range(m)) + " } = shipped;\n"
        if mode == "lost_original":
            return wiring + decl + "retire(keeper_0);\nunit\n"
        if mode == "duplicate_receiver":
            decl += "retire(keeper_0);\nretire(keeper_0);\n"
            decl += "retire(keeper_1); retire(keeper_2); retire(keeper_3);\nunit\n"
            return wiring + decl
        order = [1,3,0,2] if n == 5 else list(range(m))
        for i in order:
            decl += f"retire(keeper_{i});\n"
        for i in range(m,n):
            decl += f"let keeper_{i} = LiveRoot {{p:p_{i}, a:a_{i}, d:d_{i}}};\n"
            decl += f"retire(keeper_{i});\n"
        return wiring + decl+"unit\n"
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
  ("duplicate_donor", "duplicate_donor", 5),
  ("duplicate_receiver", "duplicate_receiver", 5),
  ("mixed_allocation_release", "mixed_alloc", 5),
  ("mixed_domain_release", "mixed_domain", 5),
  ("lost_original", "lost_original", 5),
  ("omitted_owner", "omitted_member", 5),
  ("active_domain_loan", "active_loan", 5),
  ("original_3", "plain", 3),
  ("original_4", "plain", 4),
  ("original_5", "plain", 5),
  ("direct_release_5", "direct_release", 5),
  ("local_transport_5", "local_transport", 5),
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
