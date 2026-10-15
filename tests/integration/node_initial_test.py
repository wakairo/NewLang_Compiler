"""Initial links from actual source/public owned evidence, with passive C reads."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
from node_join_native_test import native, execute


def observer(manifest):
    records = []
    for line in manifest.splitlines():
        symbol, kind, scalar, tag, target = [int(x) for x in line.split()[1:]]
        records.append((symbol, kind, scalar, tag, target))
    assert len(records) == 4 and sum(r[1] == 1 for r in records) == 2
    events = ",\n".join("{" + ",".join(map(str, r)) + "}" for r in records)
    return r'''
#include <stdio.h>
#include <stdlib.h>
typedef struct { size_t symbol,kind,scalar,tag,target; } expected;
static const expected events[] = {
''' + events + r'''
};
static size_t index_at;
static const nl_node *roots[256];
static const nl_node_option *copies[256];
static const nl_node *const *pointers[256];
static void require(int c) {
    if (!c) { fputs("native topology assertion failed\n",stderr); abort(); }
}
static const nl_node *target(size_t id) {
    require(id < 256 && (id == 0 || roots[id] != NULL));
    return roots[id];
}
static void observed_bind(size_t frame,size_t symbol,size_t kind,const void *address) {
    require(index_at < sizeof(events)/sizeof(events[0]));
    const expected *e=&events[index_at++];
    require(frame == 0 && symbol < 256 && e->symbol == symbol && e->kind == kind);
    if(kind == 1) {
        const nl_node *p=address;
        for(size_t i=0;i<256;++i) require(roots[i] != p);
        require(p->f1 == e->scalar && p->f0.tag == e->tag && p->f0.ptr == target(e->target));
        roots[symbol]=p;
        printf("root=%p tag=%u ptr=%p payload=%u\n",(const void *)p,p->f0.tag,(const void *)p->f0.ptr,(unsigned)p->f1);
    } else if(kind == 2) {
        const nl_node *const *p=address;
        require(*p == target(e->target)); pointers[symbol]=p;
    } else {
        require(kind == 3);
        const nl_node_option *p=address;
        require(p->tag == e->tag && p->ptr == target(e->target));
        for(size_t i=0;i<256;++i) if(roots[i]) require(p != &roots[i]->f0);
        copies[symbol]=p;
    }
}
static void observed_finish(void) {
    require(index_at == sizeof(events)/sizeof(events[0]));
    for(size_t i=0;i<sizeof(events)/sizeof(events[0]);++i) {
        const expected *e=&events[i];
        if(e->kind == 1) {
            const nl_node *p=roots[e->symbol];
            require(p && p->f1 == e->scalar && p->f0.tag == e->tag && p->f0.ptr == target(e->target));
        } else if(e->kind == 2) require(*pointers[e->symbol] == target(e->target));
        else require(copies[e->symbol]->tag == e->tag && copies[e->symbol]->ptr == target(e->target));
    }
    puts("native topology observations complete");
}
#define NL_NODE_BIND(f,s,k,p) observed_bind(f,s,k,p)
#define NL_NODE_FINISH() observed_finish()
'''


def main():
    compiler, probe, cc, fixtures, sanitizer = sys.argv[1:]
    fixtures = Path(fixtures)
    records = []
    with tempfile.TemporaryDirectory(prefix="newlang-initial-") as tmp:
        root = Path(tmp)
        for name in ("node_initial_some", "node_initial_none"):
            source = fixtures / (name + ".nl")
            checked = execute([compiler, "--check", str(source)])
            assert (checked.returncode, checked.stdout, checked.stderr) == (0, "", "")
            emitted = execute([compiler, str(source)])
            again = execute([compiler, str(source)])
            assert (emitted.returncode, emitted.stderr) == (0, "")
            assert (again.returncode, again.stdout, again.stderr) == (0, emitted.stdout, "")
            owned = execute([probe, "view", str(source)])
            assert owned.returncode == 0 and not owned.stderr
            header = root / (name + ".h")
            header.write_text(observer(owned.stdout))
            runs = [native(root, cc, name + opt + str(observed), emitted.stdout,
                           header if observed else None, opt, sanitizer)
                    for opt in ("-O0", "-O2") for observed in (False, True)]
            mutants = []
            if name == "node_initial_some":
                import re
                for label in ("tag", "pointer"):
                    def replacement(match):
                        pointer = match[1]
                        return ("{ 0, " + pointer + " }" if label == "tag"
                                else "{ 1, NULL };\n(void)" + pointer)
                    broken, count = re.subn(r"\{ 1, (nl_v_\d+) \}", replacement, emitted.stdout, count=1)
                    assert count == 1
                    mutants.append(native(root, cc, label, broken, header, "-O2", sanitizer, False))
                roots = [line.split()[1] for line in owned.stdout.splitlines() if line.split()[2] == "1"]
                assert len(roots) == 2
                for symbol in roots:
                    broken = emitted.stdout.replace("NL_NODE_FINISH();", f"nl_b_0_{symbol}.f1 ^= 1;\nNL_NODE_FINISH();")
                    mutants.append(native(root, cc, "sibling" + symbol, broken, header, "-O2", sanitizer, False))
            records.append({"source": name, "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                            "C_bytes": len(emitted.stdout.encode()), "C_sha256": hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                            "checked_public_manifest": owned.stdout, "native": runs, "observer_mutants": mutants})
        text = (fixtures / "node_initial_some.nl").read_text()
        decl = "struct Node { next: Option<ptr<Node>>, payload: u8, }\n"
        negatives = {
            "wrong_type": (text.replace("::Some(peer_ptr)", "::Some(u8(3))"), "P6-PAYLOAD-TYPE"),
            "escape": (text.replace("ptr_from_ref(stable)", "stable"), "P8-EXIT-DEPENDENCY"),
            "expired": (decl + "fn main()->unit{let p={let dead=Node{next:Option<ptr<Node>>::None,payload:u8(7)};loan_read(dead){|r|ptr_from_ref(r)}};loan_read_ptr(p){|r|unit};unit}\n", "P3-STALE-POINTER"),
        }
        for name, (source, diagnostic) in negatives.items():
            path = root / (name + ".nl")
            path.write_text(source)
            for mode in ([], ["--check"]):
                bad = execute([compiler, *mode, str(path)])
                assert bad.returncode == 3 and not bad.stdout and diagnostic in bad.stderr
        # A legal Copy alias would require a broader producer graph contract.
        alias = root / "alias.nl"
        alias.write_text(text.replace("    let item=", "    let alias=peer_ptr;\n    let item=").replace("::Some(peer_ptr)", "::Some(alias)"))
        accepted = execute([compiler, "--check", str(alias)])
        assert (accepted.returncode, accepted.stdout, accepted.stderr) == (0, "", "")
        refused = execute([compiler, str(alias)])
        assert refused.returncode == 4 and not refused.stdout and "V1-BACKEND-UNSUPPORTED" in refused.stderr
    print(json.dumps({"host": cc, "sanitizer": sanitizer, "cases": records,
                      "semantic_negatives": list(negatives), "alias": "CHECK0/backend4/C0 profile refusal"}, indent=2))


if __name__ == "__main__":
    main()
