"""Conservative R845 passive C17 derivative of *R-emitted* original source only.

No producer fixture, compiler hook, host-seeded substitute, or guest writes.
A mismatch in lexical/type anchoring is explicitly NOT TESTED.
"""
import hashlib
from pathlib import Path
import re
import subprocess


def _statement_end(text, begin):
    braces = 0
    square = 0
    paren = 0
    for i in range(begin, len(text)):
        ch = text[i]
        if ch == "{":
            braces += 1
        elif ch == "}":
            braces -= 1
        elif ch == "[":
            square += 1
        elif ch == "]":
            square -= 1
        elif ch == "(":
            paren += 1
        elif ch == ")":
            paren -= 1
        elif ch == ";" and braces == square == paren == 0:
            return i + 1
    return None


def _record_carrier(text, label):
    # Identifier must be an actual declarator, not an incidental string/comment.
    decl = re.compile(r"\b(?P<type>[A-Za-z_]\w*)\s+" + re.escape(label) +
                      r"\s*(?P<direct>\[\s*4\s*\])?\s*(?==|;)")
    matches = list(decl.finditer(text))
    if len(matches) != 1:
        return None
    match = matches[0]
    typ = match.group("type")
    if match.group("direct"):
        if "option" not in typ.lower():
            return None
        return (label, typ, None, _statement_end(text, match.end()))
    # A lexical original struct whose SINGLE full Option array field has 4 items.
    typedefs = re.finditer(
        r"typedef\s+struct\s*(?:[A-Za-z_]\w*\s*)?\{(?P<body>.*?)\}\s*"
        r"(?P<name>[A-Za-z_]\w*)\s*;", text, re.DOTALL)
    fields = []
    for td in typedefs:
        if td.group("name") != typ:
            continue
        field_pattern = re.compile(
            r"\b(?P<element>[A-Za-z_]\w*)\s+(?P<field>[A-Za-z_]\w*)"
            r"\s*\[\s*4\s*\]\s*;")
        for f in field_pattern.finditer(td.group("body")):
            if "option" in f.group("element").lower():
                fields.append((f.group("field"), f.group("element")))
    if len(fields) != 1:
        return None
    return (label, fields[0][1], fields[0][0],
            _statement_end(text, match.end()))


def derivative(generated, labels):
    try:
        text = generated.decode("utf-8")
    except UnicodeDecodeError:
        return None
    main = re.search(r"\bmain\s*\([^)]*\)\s*\{", text)
    if not main:
        return None
    carriers = [_record_carrier(text, name) for name in labels]
    if any(c is None or c[3] is None for c in carriers):
        return None
    # Require lexical declarations in ordinary source main, before its exit.
    if any(not (main.end() < c[3] < len(text)) for c in carriers):
        return None
    insertion = max(c[3] for c in carriers)
    refs = []
    lines = []
    for i, (name, element_type, field, _) in enumerate(carriers):
        ref = name + ("." + field if field is not None else "")
        refs.append(ref)
        lines.append(
            '    _Static_assert(sizeof(' + ref + ')/sizeof(' + ref +
            '[0]) == 4, "R845 full Option arity");')
        for j in range(3):
            lines.append(
                "    if ((uintptr_t)(const void *)&" + ref + "[" + str(j+1) +
                "] - (uintptr_t)(const void *)&" + ref + "[" + str(j) +
                "] != sizeof(" + ref + "[0])) abort();")
        lines.append("    (void)sizeof(" + element_type + ");")
    if len(refs) == 2:
        a, b = refs
        lines.extend([
            "    if (!((uintptr_t)(const void *)&" + a +
            "[3] + sizeof(" + a + "[3]) <= (uintptr_t)(const void *)&" +
            b + "[0] ||",
            "          (uintptr_t)(const void *)&" + b +
            "[3] + sizeof(" + b + "[3]) <= (uintptr_t)(const void *)&" +
            a + "[0])) abort();"
        ])
    lines.append('    puts("R845_PHYSICAL_OK");')
    addition = "\n" + "\n".join(lines) + "\n"
    modified = ('#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n' +
                text[:insertion] + addition + text[insertion:])
    return modified.encode("utf-8")


def test(generated, labels, folder, compilers):
    derived = derivative(generated, labels)
    if derived is None:
        print("R845 PASSIVE PHYSICAL=NOT TESTED (source-original carrier/type/"
              "lexical-live insertion not demonstrably identified)", flush=True)
        return False
    original_sha = hashlib.sha256(generated).hexdigest()
    derived_sha = hashlib.sha256(derived).hexdigest()
    path = Path(folder) / ("derivative" + str(len(labels)) + ".c")
    path.write_bytes(derived)
    for i, cc in enumerate(compilers):
        for level in ("-O0", "-O2"):
            exe = Path(folder) / ("physical-%d-%s-%s" % (len(labels), i, level))
            cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror",
                   "-pedantic", level, str(path), "-o", str(exe)]
            p = subprocess.run(cmd, capture_output=True, timeout=25)
            if p.returncode != 0:
                raise RuntimeError("R845 derived physical C17 compile failure " +
                                   repr(p.stderr[-900:]))
            p = subprocess.run([str(exe)], capture_output=True, timeout=25)
            if p.returncode != 0 or p.stdout != b"R845_PHYSICAL_OK\n" or p.stderr:
                raise RuntimeError("R845 derived physical identity/stride failure: " +
                                   repr((p.returncode, p.stdout, p.stderr)))
    print("R845 PASSIVE DERIVATIVE physical=PASS original_sha256=" +
          original_sha + " derived_sha256=" + derived_sha +
          " target ABI=CI runner GCC/Clang C17 O0/O2", flush=True)
    return True
