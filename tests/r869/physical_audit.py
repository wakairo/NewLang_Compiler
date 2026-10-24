#!/usr/bin/env python3
"""R869: original full-Option Array4 physical witness; GitHub Actions only."""
import base64
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

SHA = {
    "r839_azurite_1root.nl": "1e9e7f92872b86b82d69f2154391b41c51b39cc09c43703816c95514ddf6999c",
    "r839_terraces_2root.nl": "129ded0ef85cd486b89b41053ad878a51b91528f195c67525bc00bdaebec1d74",
    "r839_reject_arity3.nl": "6ba92a43981b0d3ca13915efa01c8723a6888112dfd220e149e7949d278f6b03",
    "r839_reject_element_u8.nl": "540f2c04ca2ad85bcebdcd9a64cde12d9a4df6928149b607a95f294f5d7fefb8",
    "r839_reject_indirect_none.nl": "337fa43880c41b6f6a0ea78e7115addf4382988e5a9744d40d33fa0ad851df75",
    "r839_reject_some_without_payload.nl": "0d8a54f2432174d9e63874a94d8b2dc7fdb03ff263235bd39c6df5e36e36ca97",
}
EMIT = {
    "r839_azurite_1root.nl": (696, "9ab362d269a9cb1a3cb32cd842174ceb06ed55ae456e42ec9ddabda47eeda77f"),
    "r839_terraces_2root.nl": (895, "57a3c3711159f682cd1913120cfaa3a9a1893b98ddda7a54efd89238783b9574"),
}
CAP = 16384


class NotTested(Exception):
    pass


def check(yes, why):
    if not yes:
        raise NotTested(why)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def run(args):
    return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=90, check=False)


def b64(case, kind, b):
    encoded = base64.b64encode(b).decode("ascii")
    check(len(encoded) <= CAP, "log cap " + kind)
    count = (len(encoded) + 2047) // 2048
    print(f"R869_SEGMENTS case={case} kind={kind} raw={len(b)} sha256={sha(b)} parts={count}", flush=True)
    for i in range(count):
        print(f"R869_B64 case={case} kind={kind} part={i+1}/{count} data={encoded[i*2048:(i+1)*2048]}", flush=True)


def masked(s):
    pattern = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: "".join("\n" if c == "\n" else " "
                                             for c in m.group()), s)


def match_brace(s, start):
    check(s[start] == "{", "expected C brace")
    depth = 0
    for i in range(start, len(s)):
        if s[i] == "{":
            depth += 1
        elif s[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    raise NotTested("C scope unclosed")


def stmt_end(s, start, limit):
    depth = 0
    for i in range(start, limit):
        if s[i] == "{":
            depth += 1
        elif s[i] == "}":
            depth -= 1
        elif s[i] == ";" and depth == 0:
            return i + 1
    raise NotTested("C original initializer not complete")


def source_bindings(b):
    return re.findall(r"\blet\s+([A-Za-z_]\w*)\s*=\s*Array\s*<\s*Option\s*<\s*u8\s*>\s*,\s*4\s*>\s*\{",
                      b.decode("utf-8"))


def public_records(map_exe, source, names):
    p = run([map_exe, str(source), str(len(names))])
    check(p.returncode == 0 and not p.stderr and len(p.stdout) < 2048,
          "source-issued registered public map failed: " + repr((p.returncode, p.stderr[-400:])))
    out = []
    pattern = (r"R869_PUBLIC index=(\d+) name=([A-Za-z_]\w*) symbol=(\d+) "
               r"array=(\d+) element=(\d+) binding=(\d+) constructor=(\d+) "
               r"root=(\d+) incarnation=(\d+)")
    for line in p.stdout.decode("ascii").splitlines():
        m = re.fullmatch(pattern, line)
        check(m is not None, "public checked output unrecognized")
        obj = {"idx": int(m[1]), "name": m[2], "symbol": int(m[3]),
               "array": int(m[4]), "element": int(m[5]), "binding": int(m[6]),
               "constructor": int(m[7]), "root": int(m[8]), "incarnation": int(m[9])}
        out.append(obj)
    check(len(out) == len(names), "source original checked count mismatch")
    for i, (rec, name) in enumerate(zip(out, names)):
        check(rec["idx"] == i and rec["name"] == name and
              all(rec[k] > 0 for k in ("symbol","array","element","binding","constructor","root","incarnation")),
              "public source record/typed binding mismatch")
        print("R869_TYPED " + " ".join(f"{k}={v}" for k,v in rec.items()), flush=True)
    for key in ("symbol", "root", "incarnation", "binding", "constructor"):
        check(len({obj[key] for obj in out}) == len(out), "public roots not distinct: " + key)
    return out


def map_original(plain, records):
    text = plain.decode("ascii")
    s = masked(text)
    mm = list(re.finditer(r"\bint\s+main\s*\(\s*void\s*\)\s*\{", s))
    check(len(mm) == 1, "unique generated C main absent")
    mo = s.index("{", mm[0].start(), mm[0].end())
    me = match_brace(s, mo)
    call = re.fullmatch(r"\s*([A-Za-z_]\w*)\s*\(\s*\)\s*;\s*return\s+0\s*;\s*",s[mo+1:me])
    check(call is not None, "main does not call one original containing function")
    called = call[1]
    functions = list(re.finditer(r"\bstatic\s+void\s+" + re.escape(called) +
                                r"\s*\(\s*void\s*\)\s*\{",s))
    check(len(functions) == 1, "actual called source function not unique")
    fo = s.index("{", functions[0].start(), functions[0].end())
    fe = match_brace(s, fo)
    check(fe < mo, "called original must precede main")
    body = s[fo+1:fe]
    check(not re.search(r"\b(return|goto|longjmp)\b",body), "early original function exit")
    locals_ = []
    ends = []
    for rec in records:
        opt = "nl_option_" + str(rec["element"])
        arr = "nl_array_" + str(rec["array"])
        local = "nl_local_" + str(rec["symbol"])
        struct = (r"\btypedef\s+struct\s*\{\s*uint8_t\s+tag\s*;\s*uint8_t\s+payload\s*;\s*\}\s*" +
                  re.escape(opt) + r"\s*;")
        array = r"\btypedef\s+" + re.escape(opt) + r"\s+" + re.escape(arr) + r"\s*\[\s*4\s*\]\s*;"
        check(len(re.findall(struct,s)) == 1 and len(re.findall(array,s)) == 1,
              "real full Option tag+payload and 4-element array type chain absent")
        matches = list(re.finditer(r"\b"+re.escape(arr)+r"\s+"+re.escape(local)+r"\s*=\s*\{",body))
        check(len(matches) == 1, "checked symbol/type -> unique C original missing: "+rec["name"])
        init = fo + 1 + matches[0].end() - 1
        end = stmt_end(s,init,fe)
        initializer = (r"\{\s*" +
                       (r"\{\s*\.tag\s*=\s*NL_ARRAY4_NONE\s*\}\s*,\s*" * 3) +
                       r"\{\s*\.tag\s*=\s*NL_ARRAY4_NONE\s*\}\s*\}\s*;")
        check(re.fullmatch(initializer,s[init:end]) is not None,
              "real C original initializer does not contain four None Options")
        start = fo+1+matches[0].start()
        check(s[fo+1:start].count("{") == s[fo+1:start].count("}"),
              "original typed declarator not at common top lexical scope")
        locals_.append((local,opt))
        ends.append(end)
        print(f"R869_ANCHOR source={rec['name']} binding={rec['binding']} "
              f"constructor={rec['constructor']} symbol={rec['symbol']} "
              f"root={rec['root']} incarnation={rec['incarnation']} "
              f"original={local} array={arr} element={opt} span={start}:{end}",flush=True)
    check(len(re.findall(r"\bnl_array_\d+\s+nl_local_\d+\s*=\s*\{",body)) == len(records),
          "ambiguous additional original C array")
    check(ends == sorted(ends) and len(set(x[0] for x in locals_)) == len(records),
          "C source/original construction order mismatch")
    offset=ends[-1]
    check(offset < fe and s[fo+1:offset].count("{")==s[fo+1:offset].count("}"),
          "no shared live lexical point after last original initializer")
    enum = re.findall(r"\benum\s*\{\s*NL_ARRAY4_NONE\s*=\s*(\d+)\s*\}\s*;",s)
    check(len(enum) == 1, "none discriminator not source-emitted")
    print(f"R869_MAPPING called={called} function_open={fo} function_end={fe} "
          f"insertion_byte={offset} original_none_enum={enum[0]} source_roots={len(records)}",flush=True)
    return text,locals_,offset,int(enum[0])


def inject(locals_):
    lines=[]
    for idx,(local,opt) in enumerate(locals_):
        lines.append(f'    _Static_assert(sizeof({local})/sizeof({local}[0]) == 4, "real original Array4");')
        addresses=", ".join(f"(const void *)&{local}[{j}]" for j in range(4))
        deltas=", ".join(f"(size_t)(&{local}[{j+1}]-&{local}[{j}])*sizeof({local}[0])" for j in range(3))
        tags=", ".join(f"(unsigned){local}[{j}].tag" for j in range(4))
        lines.append('    printf("R869_ROOT idx=' + str(idx) +
                     ' addr0=%p addr1=%p addr2=%p addr3=%p elem=%zu array=%zu align=%zu '
                     'tag_offset=%zu payload_offset=%zu d01=%zu d12=%zu d23=%zu '
                     'tag0=%u tag1=%u tag2=%u tag3=%u\\n", ' +
                     addresses+f", sizeof({local}[0]), sizeof({local}), _Alignof({opt}), "+
                     f"offsetof({opt},tag), offsetof({opt},payload), "+
                     deltas+", "+tags+");")
    if len(locals_)==2:
        a,b=(x[0] for x in locals_)
        lines.append("    int _r869_disjoint = 1;")
        lines.append(f"    for (size_t _r869_i=0; _r869_i<sizeof({a}); ++_r869_i) {{")
        lines.append(f"        for (size_t _r869_j=0; _r869_j<sizeof({b}); ++_r869_j) {{")
        lines.append(f"            if (((const unsigned char *)(const void *)&{a})+_r869_i ==")
        lines.append(f"                ((const unsigned char *)(const void *)&{b})+_r869_j)")
        lines.append("                _r869_disjoint = 0;")
        lines.append("        }")
        lines.append("    }")
        lines.append(f'    printf("R869_PAIR addr_a=%p addr_b=%p disjoint=%d bytes_a=%zu bytes_b=%zu\\n", '
                     f'(const void *)&{a}[0], (const void *)&{b}[0], _r869_disjoint, sizeof({a}), sizeof({b}));')
        lines.append("    if (!_r869_disjoint) abort();")
    lines.append('    puts("R869_PHYSICAL_OK");')
    return "\n"+"\n".join(lines)+"\n"


def guest(b,folder,case,traced,sanit,roots,tag):
    suffix="derivative" if traced else "plain"
    src=folder/(case+"-"+suffix+".c")
    src.write_bytes(b)
    for compiler in ("gcc","clang"):
        cc=shutil.which(compiler)
        check(cc is not None,"native compiler not found: "+compiler)
        v=run([cc,"--version"])
        check(v.returncode==0,compiler+" --version unavailable")
        for opt in ("-O0","-O2"):
            exe=folder/(case+"-"+suffix+"-"+compiler+opt)
            cmd=[cc,"-std=c17","-Wall","-Wextra","-Werror","-pedantic",opt]
            if traced and sanit in ("address","undefined"):
                cmd += ["-fsanitize="+sanit,"-fno-sanitize-recover=all"]
            cp=run(cmd+[str(src),"-o",str(exe)])
            check(cp.returncode==0,"strict C17 "+suffix+" "+compiler+opt+" "
                  +repr(cp.stderr[-1000:]))
            p=run([str(exe)])
            check(p.returncode==0 and not p.stderr and len(p.stdout)<4096,
                  "native "+suffix+" "+compiler+opt+" "+
                  repr((p.returncode,p.stderr[-500:],p.stdout[-500:])))
            if not traced:
                check(not p.stdout,"original unmodified C printed output")
                print(f"R869_PLAIN case={case} compiler={compiler} opt={opt} C17=PASS",flush=True)
                continue
            rows=p.stdout.decode("ascii").splitlines()
            root_rows=[s for s in rows if s.startswith("R869_ROOT ")]
            check(len(root_rows)==roots and rows[-1:] == ["R869_PHYSICAL_OK"],
                  "original physical trace missing/incomplete")
            for idx,row in enumerate(root_rows):
                vals=dict(v.split("=",1) for v in row.split()[1:])
                size,total=int(vals["elem"]),int(vals["array"])
                check(int(vals["idx"])==idx and size>=2 and total==4*size and
                      int(vals["align"])>0,"actual full Option element/Array size invalid")
                check(0<=int(vals["tag_offset"])<size and
                      0<=int(vals["payload_offset"])<size and
                      vals["tag_offset"]!=vals["payload_offset"],
                      "full Option tag+payload offsets not distinct")
                check(all(int(vals["d"+k])==size for k in ("01","12","23")),
                      "numeric element-start byte stride not sizeof full Option")
                check(all(int(vals["tag"+str(j)])==tag for j in range(4)),
                      "actual original None discriminator mismatch")
                check(len({vals["addr"+str(j)] for j in range(4)})==4,
                      "4 actual original element-start addresses not distinct")
            if roots==2:
                pairs=[r for r in rows if r.startswith("R869_PAIR ")]
                check(len(pairs)==1,"two originals not sampled together")
                pair=dict(v.split("=",1) for v in pairs[0].split()[1:])
                check(pair["disjoint"]=="1" and pair["addr_a"]!=pair["addr_b"] and
                      pair["bytes_a"]==pair["bytes_b"] and
                      int(pair["bytes_a"])==int(dict(v.split("=",1) for v in root_rows[0].split()[1:])["array"]),
                      "two original live object identities overlap")
            print(f"R869_NATIVE case={case} compiler={compiler} opt={opt} sanit={sanit} "
                  f"version={v.stdout.decode(errors='replace').splitlines()[0]}",flush=True)
            print(p.stdout.decode("ascii"),end="",flush=True)


def positive(cli,mapper,root,folder,name,sanit):
    src=root/name
    names=source_bindings(src.read_bytes())
    n=1 if "1root" in name else 2
    check(len(names)==n,"immutable source typed direct constructor count invalid")
    ck=run([cli,"--check",str(src)])
    check(ck.returncode==0 and not ck.stdout and not ck.stderr,
          "ordinary original CHECK fail: "+repr(ck.stderr[-500:]))
    out=run([cli,str(src)])
    check(out.returncode==0 and not out.stderr and out.stdout,
          "ordinary original emission fail: "+repr(out.stderr[-500:]))
    plain=out.stdout
    case="one" if n==1 else "two"
    (folder/(case+"-original.c")).write_bytes(plain)
    print(f"R869_ORIGINAL case={case} bytes={len(plain)} sha256={sha(plain)}",flush=True)
    b64(case,"original",plain) # unconditional before correspondence / original emission SHA gate
    expected=EMIT[name]
    check(len(plain)==expected[0] and sha(plain)==expected[1],
          "frozen ordinary C17 output SHA/length mismatch; original retained")
    guest(plain,folder,case,False,sanit,n,0)
    records=public_records(mapper,src,names)
    text,locals_,offset,none=map_original(plain,records)
    prelude="#include <stddef.h>\n#include <stdio.h>\n#include <stdlib.h>\n"
    injected=inject(locals_)
    patch=(prelude+injected).encode("ascii")
    check(len(base64.b64encode(plain))+len(base64.b64encode(patch))<=CAP,
          "combined plain+patch CI log budget exceeded")
    derived=(prelude+text[:offset]+injected+text[offset:]).encode("ascii")
    print(f"R869_DERIVATIVE case={case} plain_sha256={sha(plain)} derived_sha256={sha(derived)} "
          f"insertion_byte={offset} prelude_bytes={len(prelude)} observer_bytes={len(injected)}",flush=True)
    b64(case,"patch",patch)
    guest(derived,folder,case,True,sanit,n,none)
    print(f"R869_ORIGINAL_PHYSICAL=PASS case={case} roots={n} live_numeric_full_Option=OBSERVED",flush=True)


def main():
    check(len(sys.argv)==5,"usage: physical_audit.py NEWLANGC PUBLIC_MAP ROOT SANITIZER")
    cli,mapper,dirname,sanit=sys.argv[1:]
    check(sanit in ("none","off","","address","undefined"),"unexpected sanitizer")
    root=Path(dirname)
    for name,expected in SHA.items():
        raw=(root/name).read_bytes()
        check(sha(raw)==expected,"R839 IMMUTABLE SOURCE SHA mismatch "+name)
        print(f"R869_FROZEN source={name} bytes={len(raw)} sha256={sha(raw)}",flush=True)
    for name in list(SHA)[2:]:
        for arg in (["--check",str(root/name)],[str(root/name)]):
            result=run([cli]+arg)
            check(result.returncode!=0 and not result.stdout,
                  "frozen original negative ACCEPT/EMIT: "+name)
        print(f"R869_NEGATIVE=PASS source={name}",flush=True)
    with tempfile.TemporaryDirectory(prefix="r869-") as temp:
        folder=Path(temp)
        for name in EMIT:
            try:
                positive(cli,mapper,root,folder,name,sanit)
            except NotTested as err:
                print(f"R869_ORIGINAL_PHYSICAL=NOT TESTED source={name} reason={err}",flush=True)
                raise
    print("R869_TERMINAL ORIGINAL_PHYSICAL=PASS witnessed=2/2",flush=True)


if __name__=="__main__":
    try:
        main()
    except (NotTested,OSError,UnicodeError,subprocess.TimeoutExpired,KeyError,ValueError,IndexError) as err:
        print("R869_STOP ORIGINAL_PHYSICAL=NOT TESTED reason="+repr(err),file=sys.stderr,flush=True)
        sys.exit(1)
