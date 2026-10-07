"""Actual-source AVS witness, E4/E5 and canonical N1/N2/N3/N5.

E1-E3 are checked separately through public registry/checked APIs in avs_test.
No host-known aggregate seed or runtime helper participates in these programs.
"""
from pathlib import Path
import re
import sys
import tempfile
from checked_c_v0_test import negative, positive, run

DECL = "struct Pair { left: u8, right: u8, }\n"


def main() -> None:
    compiler, cc = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="newlang-avs-") as temp:
        root = Path(temp)
        positive(root, compiler, cc, "p1", DECL + "fn main()->unit{let p=Pair{left:u8(7),right:u8(9),};let Pair{left,right}=p;left;right;unit}\n")
        c = (root / "p1.c").read_text(encoding="utf-8")
        required = [r"typedef struct .*uint8_t f0;.*uint8_t f1;.*nl_type_\d+;",
                    r"\.f0 = 7", r"\.f1 = 9",
                    r"nl_destructure_\d+ = nl_local_\d+;",
                    r"const uint8_t nl_local_\d+ = nl_destructure_\d+\.f0;",
                    r"const uint8_t nl_local_\d+ = nl_destructure_\d+\.f1;"]
        if not all(re.search(pattern, c) for pattern in required):
            raise SystemExit("E4: missing checked nominal/field/value/receiving lowering")
        receivers = re.findall(r"const uint8_t (nl_local_\d+) = nl_destructure_\d+\.f[01];", c)
        if len(receivers) != 2 or any(c.count(f"(void){name};") != 2 for name in receivers):
            raise SystemExit("E4: missing explicit scalar receiver use")
        if "Pair" in c or "left" in c or "right" in c:
            raise SystemExit("source nominal/field names leaked into C representation")
        positive(root, compiler, cc, "reordered", DECL + "fn main()->unit{let p=Pair{right:u8(9),left:u8(7)};let Pair{right,left}=p;right;left;unit}\n")
        reordered = (root / "reordered.c").read_text(encoding="utf-8")
        if reordered.index(".f1 = 9") > reordered.index(".f0 = 7"):
            raise SystemExit("source initializer order lost in checked lowering")
        positive(root, compiler, cc, "copy_reuse", DECL + "fn main()->unit{let p=Pair{left:u8(7),right:u8(9)};{let Pair{left,right}=p;left;right;unit};let Pair{left,right}=p;left;right;unit}\n")
        # Labels/local names which are C keywords require no special backend
        # semantic resolution: checked indices/symbols already identify them.
        positive(root, compiler, cc, "names", "struct Record{typedef:u8,enum:u8}fn main()->unit{let p=Record{enum:u8(9),typedef:u8(7)};let Record{typedef,enum}=p;typedef;enum;unit}\n")
        cases = [
            ("n1", "let p=Pair{left:u8(7),nope:u8(9)};unit", "P5-AGGREGATE-FIELD"),
            ("n2", "let p=Pair{left:u8(7)};unit", "P5-AGGREGATE-FIELD-COUNT"),
            ("n3", "let p=Pair{left:u8(7),left:u8(9)};unit", "P5-AGGREGATE-FIELD"),
            ("n5", "let p=Pair{left:u8(7),right:u8(9)};let Pair{left,nope}=p;unit", "P5-AGGREGATE-FIELD"),
        ]
        for name, body, code in cases:
            negative(root, compiler, name, DECL + f"fn main()->unit{{{body}}}\n", code)
        path = root / "unsupported.nl"
        path.write_text(DECL + "fn value()->u8{u8(7)}fn main()->unit{let p=Pair{left:value(),right:u8(9)};unit}", encoding="utf-8")
        result = run([compiler, str(path)])
        if result.returncode != 4 or result.stdout or "V1-BACKEND-UNSUPPORTED" not in result.stderr:
            raise SystemExit(f"unsupported accepted initializer not fenced: {result}")
    print("AVS E4/E5 execute; N1/N2/N3/N5 reject before emission; E1-E3 in avs_evidence.unit")


if __name__ == "__main__":
    main()
