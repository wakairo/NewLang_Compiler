#!/usr/bin/env python3
"""R860 independent source-original physical audit, executed only on GitHub Actions.
No P fixture, host carrier, emitter modification, or guest-field mutation.
"""
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
POSITIVE = {
    "r839_azurite_1root.nl": ("cobalt_inventory",),
    "r839_terraces_2root.nl": ("valley_register", "terrace_register"),
}
MAX_SOURCE_LOG = 64 * 1024


class Incomplete(Exception):
    """Insufficient defensible source-to-C identity evidence, NOT compiler failure."""


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def execute(argv):
    p = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       timeout=40, check=False)
    return p


def require(ok, why):
    if not ok:
        raise RuntimeError(why)


def mask_noncode(s):
    pattern = r"""//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'"""
    return re.sub(pattern, lambda m: "".join("\n" if ch == "\n" else " "
                                             for ch in m.group()), s)


def close_brace(s, start):
    require(s[start] == "{", "internal nonbrace")
    level = 0
    for i in range(start, len(s)):
        if s[i] == "{":
            level += 1
        elif s[i] == "}":
            level -= 1
            if level == 0:
                return i
    raise Incomplete("unclosed C brace")


def typedef_structs(code):
    found = {}
    for m in re.finditer(r"\btypedef\s+struct(?:\s+[A-Za-z_]\w*)?\s*\{", code):
        start = code.index("{", m.start(), m.end())
        end = close_brace(code, start)
        tail = re.match(r"\s*(?P<type>[A-Za-z_]\w*)\s*;", code[end + 1:])
        if tail:
            found[tail.group("type")] = code[start + 1:end]
    return found


def fields(body):
    out = []
    for portion in body.split(";"):
        part = portion.strip()
        if not part:
            continue
        m = re.fullmatch(
            r"(?P<type>(?:unsigned\s+char|struct\s+\w+|enum\s+\w+|[A-Za-z_]\w*))"
            r"\s+(?P<name>[A-Za-z_]\w*)\s*(?P<array>\[\s*4\s*\])?", part)
        if m is None:
            return None
        out.append((m.group("type"), m.group("name"), bool(m.group("array"))))
    return out


def real_option_type(typename, typedefs):
    if typename not in typedefs:
        return False
    fs = fields(typedefs[typename])
    if fs is None or len(fs) < 2 or any(f[2] for f in fs):
        return False
    # A full emitted Option record has a distinct discriminator and u8 payload.
    # Refuse an opaque, scalar/host array, or name-only "option" evidence.
    tags = [f for f in fs if re.search(r"(^|_)(is_some|has_value|present|tag|kind|discriminant|has)(_|$)", f[1], re.I)]
    payloads = [f for f in fs if re.search(r"(^|_)(value|payload|data)(_|$)", f[1], re.I)
                and f[0] in ("uint8_t", "unsigned char")]
    return len(tags) == 1 and len(payloads) == 1 and tags[0] != payloads[0]


def statement_end(s, begin, limit):
    brace = square = paren = 0
    for i in range(begin, limit):
        c = s[i]
        if c == "{":
            brace += 1
        elif c == "}":
            brace -= 1
        elif c == "[":
            square += 1
        elif c == "]":
            square -= 1
        elif c == "(":
            paren += 1
        elif c == ")":
            paren -= 1
        elif c == ";" and brace == square == paren == 0:
            return i + 1
    raise Incomplete("no completed initialized source declarator")


def map_originals(original, names):
    text = original.decode("utf-8")
    code = mask_noncode(text)
    main_matches = list(re.finditer(r"\bmain\s*\([^{};]*\)\s*\{", code))
    if len(main_matches) != 1:
        raise Incomplete("unique real main definition not structurally identified")
    open_at = code.index("{", main_matches[0].start(), main_matches[0].end())
    main_end = close_brace(code, open_at)
    tdefs = typedef_structs(code)
    anchors = []
    for label in names:
        pattern = (r"\b(?P<type>[A-Za-z_]\w*)\s+" + re.escape(label) +
                   r"\s*(?P<direct>\[\s*4\s*\])?\s*=\s*")
        hits = [m for m in re.finditer(pattern, code)
                if open_at < m.start() < main_end]
        if len(hits) != 1:
            raise Incomplete("no unique initialized original C declarator: " + label)
        m = hits[0]
        if code[open_at + 1:m.start()].count("{") != code[open_at + 1:m.start()].count("}"):
            raise Incomplete("original declaration not at main lexical top scope: " + label)
        end = statement_end(code, m.end(), main_end)
        if re.search(r"\breturn\b", code[open_at + 1:end]):
            raise Incomplete("original preceded by return or exit: " + label)
        init = code[m.end():end - 1].strip()
        if not init or init[0] not in "{(" or init.count("{") < 4:
            raise Incomplete("four original Option initializer records not structurally visible: " + label)
        typ = m.group("type")
        field = None
        if m.group("direct"):
            element_type = typ
        else:
            fs = fields(tdefs.get(typ, ""))
            if fs is None:
                raise Incomplete("original array typedef not an inspectable struct: " + label)
            arrays = [f for f in fs if f[2]]
            if len(fs) != 1 or len(arrays) != 1:
                raise Incomplete("original carrier must have single 4-element field: " + label)
            element_type, field, _ = arrays[0]
        if not real_option_type(element_type, tdefs):
            raise Incomplete("tag + u8 payload of real Option typedef not established: " + label)
        anchors.append((label, typ, element_type, field, m.start(), end))
    insertion = max(a[5] for a in anchors)
    if not insertion < main_end:
        raise Incomplete("after original declarations no shared live main scope")
    if code[open_at + 1:insertion].count("{") != code[open_at + 1:insertion].count("}"):
        raise Incomplete("insertion is not a shared top-level lexical main point")
    if re.search(r"\breturn\b", code[open_at + 1:insertion]):
        raise Incomplete("return before simultaneous live observation")
    for a in anchors:
        if not a[5] <= insertion:
            raise Incomplete("an original begins after instrumentation")
    return text, anchors, insertion, (open_at, main_end), tdefs


def instrument(anchors):
    lines = []
    firsts = []
    lasts = []
    for index, (label, typ, element, field, start, end) in enumerate(anchors):
        ref = label + ("." + field if field else "")
        v = ["_r860_%d_%d" % (index, j) for j in range(4)]
        firsts.append(v[0])
        lasts.append(v[3])
        lines.append('    _Static_assert(sizeof(' + ref + ')/sizeof(' + ref +
                     '[0]) == 4, "R860 original arity");')
        lines.append('    _Static_assert(sizeof(uintptr_t) <= sizeof(unsigned long long), "R860 ABI logging");')
        for j in range(4):
            lines.append("    uintptr_t %s = (uintptr_t)(const void *)&%s[%d];" % (v[j], ref, j))
        checks = ["%s < %s && %s - %s == sizeof(%s[0])" %
                  (v[j], v[j + 1], v[j + 1], v[j], ref) for j in range(3)]
        lines.append("    if (!(%s)) abort();" % (" && ".join(checks)))
        lines.append('    printf("R860_ROOT name=%s element=%s type=%s addr0=%%p addr3=%%p elem_bytes=%%zu carrier_bytes=%%zu delta01=%%llu delta12=%%llu delta23=%%llu\\n", (void *)&%s[0], (void *)&%s[3], sizeof(%s[0]), sizeof(%s), (unsigned long long)(%s - %s), (unsigned long long)(%s - %s), (unsigned long long)(%s - %s));' %
                     (label, element, typ, ref, ref, ref, label, v[1], v[0],
                      v[2], v[1], v[3], v[2]))
    if len(anchors) == 2:
        a, b = anchors
        ar = a[0] + ("." + a[3] if a[3] else "")
        br = b[0] + ("." + b[3] if b[3] else "")
        ae = "_r860_end_a"
        be = "_r860_end_b"
        lines.append("    uintptr_t %s = %s + sizeof(%s[3]);" % (ae, lasts[0], ar))
        lines.append("    uintptr_t %s = %s + sizeof(%s[3]);" % (be, lasts[1], br))
        lines.append("    if (%s <= %s || %s <= %s) abort();" % (ae, lasts[0], be, lasts[1]))
        lines.append("    if (!((%s <= %s) || (%s <= %s))) abort();" %
                     (ae, firsts[1], be, firsts[0]))
        lines.append('    printf("R860_PAIR original_a=%s original_b=%s first_a=%%p first_b=%%p end_a=%%llu end_b=%%llu nonoverlap=1\\n", (void *)&%s[0], (void *)&%s[0], (unsigned long long)%s, (unsigned long long)%s);' %
                     (a[0], b[0], ar, br, ae, be))
    lines.append('    puts("R860_PHYSICAL_OK");')
    return "\n" + "\n".join(lines) + "\n"


def native(generated, folder, name, trace, sanitizer):
    suffix = "derived" if trace else "original"
    c_file = folder / (name + "-" + suffix + ".c")
    c_file.write_bytes(generated)
    results = []
    for compiler in ("gcc", "clang"):
        cc = shutil.which(compiler)
        require(cc is not None, compiler + " unavailable")
        version = execute([cc, "--version"])
        require(version.returncode == 0, compiler + " --version unavailable")
        for level in ("-O0", "-O2"):
            out = folder / (name + "-" + suffix + "-" + compiler + level)
            cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror",
                   "-pedantic", level]
            if trace and sanitizer not in ("none", "off", ""):
                cmd.extend(["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"])
            cmd.extend([str(c_file), "-o", str(out)])
            cp = execute(cmd)
            require(cp.returncode == 0, "strict C17 " + suffix + " " +
                    compiler + level + ": " + repr(cp.stderr[-1200:]))
            run = execute([str(out)])
            require(run.returncode == 0 and not run.stderr,
                    "native " + suffix + " " + compiler + level + ": " +
                    repr((run.returncode, run.stdout[-800:], run.stderr[-800:])))
            if trace:
                require(run.stdout.count(b"R860_ROOT name=") == (1 if name == "one" else 2)
                        and run.stdout.count(b"R860_PHYSICAL_OK\n") == 1
                        and (name != "two" or b"nonoverlap=1" in run.stdout),
                        "missing exact physical trace " + compiler + level)
                require(len(run.stdout) < 2048, "unbounded native trace")
                print("R860_NATIVE name=%s compiler=%s level=%s sanitizer=%s version=%s" %
                      (name, compiler, level, sanitizer,
                       version.stdout.decode(errors="replace").splitlines()[0]), flush=True)
                print(run.stdout.decode("ascii"), end="", flush=True)
            else:
                require(not run.stdout, "original guest stdout drift")
                print("R860_PLAIN name=%s compiler=%s level=%s strict_C17_native=PASS" %
                      (name, compiler, level), flush=True)
            results.append((compiler, level))
    return results


def audit_one(cli, root, folder, source, names, sanitizer):
    raw = (root / source).read_bytes()
    check = execute([cli, "--check", str(root / source)])
    require(check.returncode == 0 and not check.stdout and not check.stderr,
            "ordinary source --check failure: " + source + repr(check.stderr[:400]))
    emit = execute([cli, str(root / source)])
    require(emit.returncode == 0 and emit.stdout and not emit.stderr,
            "ordinary C17 emission failure: " + source + repr(emit.stderr[:400]))
    c17 = emit.stdout
    short = "one" if len(names) == 1 else "two"
    print("R860_ORIGINAL source=%s source_sha256=%s c17_sha256=%s c17_size=%d" %
          (source, digest(raw), digest(c17), len(c17)), flush=True)
    native(c17, folder, short, False, sanitizer)
    try:
        text, anchors, insertion, scope, tdefs = map_originals(c17, names)
    except (Incomplete, UnicodeDecodeError) as exc:
        print("R860_ORIGINAL_PHYSICAL=NOT TESTED source=%s reason=%s" %
              (source, str(exc)), flush=True)
        return False
    injection = instrument(anchors)
    prelude = "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n"
    derivative = (prelude + text[:insertion] + injection + text[insertion:]).encode("utf-8")
    fragment = prelude.encode("utf-8") + injection.encode("utf-8")
    b64c = base64.b64encode(c17)
    b64p = base64.b64encode(fragment)
    if len(b64c) + len(b64p) + 4096 > MAX_SOURCE_LOG:
        print("R860_ORIGINAL_PHYSICAL=NOT TESTED source=%s reason=trace-size cap; reconstructibility incomplete" %
              source, flush=True)
        return False
    print("R860_MAPPING source=%s main_open=%d main_close=%d insertion_utf8_byte=%d" %
          (source, scope[0], scope[1], len(text[:insertion].encode("utf-8"))), flush=True)
    for label, typ, element, field, begin, end in anchors:
        print("R860_ANCHOR name=%s carrier_type=%s element_type=%s field=%s declarator_span=%d:%d option_typedef=%s" %
              (label, typ, element, field if field is not None else "direct[4]",
               begin, end, tdefs[element].replace("\n", " ").strip()), flush=True)
    for label, payload in (("original", b64c), ("patch", b64p)):
        pieces = [payload[i:i + 2048] for i in range(0, len(payload), 2048)]
        print("R860_SEGMENTS source=%s kind=%s count=%d encoded_bytes=%d" %
              (source, label, len(pieces), len(payload)), flush=True)
        for i, part in enumerate(pieces):
            print("R860_B64 source=%s kind=%s chunk=%d/%d data=%s" %
                  (source, label, i + 1, len(pieces), part.decode("ascii")), flush=True)
    print("R860_DERIVATIVE source=%s original_sha256=%s derived_sha256=%s insertion_utf8_byte=%d prelude_utf8_bytes=%d inject_utf8_bytes=%d" %
          (source, digest(c17), digest(derivative), len(text[:insertion].encode("utf-8")),
           len(prelude.encode("utf-8")), len(injection.encode("utf-8"))), flush=True)
    native(derivative, folder, short, True, sanitizer)
    print("R860_ORIGINAL_PHYSICAL=PASS source=%s roots=%d full_option_type_liveness_numeric_stride=OBSERVED" %
          (source, len(anchors)), flush=True)
    return True


def main():
    require(len(sys.argv) == 4,
            "usage: physical_audit.py NEWLANGC SRCDIR none|address|undefined")
    cli, directory, sanitizer = sys.argv[1:]
    require(sanitizer in ("none", "off", "", "address", "undefined"),
            "sanitizer unexpected")
    root = Path(directory)
    for name, expected in SHA.items():
        raw = (root / name).read_bytes()
        require(digest(raw) == expected,
                "R839 IMMUTABLE SHA DRIFT " + name)
        print("R860_FROZEN source=%s bytes=%d sha256=%s" %
              (name, len(raw), expected), flush=True)
    for neg in tuple(SHA)[2:]:
        for args in (["--check", str(root / neg)], [str(root / neg)]):
            result = execute([cli] + args)
            require(result.returncode != 0 and not result.stdout,
                    "R839 negative unexpectedly accepted " + neg)
        print("R860_NEGATIVE=PASS source=%s original_oracle=unchanged" % neg, flush=True)
    with tempfile.TemporaryDirectory(prefix="r860-") as temp:
        folder = Path(temp)
        ok = [audit_one(cli, root, folder, source, names, sanitizer)
              for source, names in POSITIVE.items()]
    print("R860_TERMINAL ORIGINAL_PHYSICAL=%s witnessed=%d/2 sanitizer=%s" %
          ("PASS" if all(ok) else "NOT TESTED", sum(ok), sanitizer), flush=True)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, subprocess.TimeoutExpired, UnicodeError) as err:
        print("R860_STOP=FAIL " + repr(err), file=sys.stderr, flush=True)
        sys.exit(1)
