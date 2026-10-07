"""#135 real source -> checked fixed field -> C17/native; no host seeds."""
from pathlib import Path
import re
import sys
import tempfile
from checked_c_v0_test import negative, positive, run

DECL = "struct Pair{left:u8,right:u8}\n"
BODY = """fn main()->unit {
let p=Pair{left:u8(7),right:u8(9)};
let before=p@left;
let old=loan_write(p@left){|w|replace(w,u8(11))};
let after=p@left;let sibling=p@right;
let Pair{left,right}=p;
before;old;after;sibling;left;right;unit
}"""


def main() -> None:
    compiler, cc = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="newlang-field-") as directory:
        root = Path(directory)
        positive(root, compiler, cc, "primary", DECL + BODY)
        c = (root / "primary.c").read_text()
        required = [r"(?m)^    nl_type_\d+ nl_local_\d+ = .*\.f0 = 7.*\.f1 = 9",
                    r"const uint8_t nl_local_\d+ = nl_local_\d+\.f0;",
                    r"uint8_t \*const nl_local_\d+ = &nl_local_\d+\.f0;",
                    r"const uint8_t nl_replace_old_\d+ = \*nl_local_\d+;",
                    r"\*nl_local_\d+ = 11;",
                    r"nl_loan_result_\d+ = nl_replace_old_\d+;",
                    r"const uint8_t nl_local_\d+ = nl_local_\d+\.f1;",
                    r"nl_destructure_\d+ = nl_local_\d+;"]
        if not all(re.search(pattern, c) for pattern in required):
            raise SystemExit("missing field/old-result/post-state checked lowering")
        if len(re.findall(r"= nl_local_\d+\.f0;", c)) != 2:
            raise SystemExit("before/after field read must remain actual member reads")
        if "Pair" in c or "left" in c or "right" in c or "offsetof" in c:
            raise SystemExit("source labels/layout leaked into semantic lowering")
        positive(root, compiler, cc, "right", DECL + BODY.replace("p@left", "p@right"))
        positive(root, compiler, cc, "reordered", DECL + BODY.replace(
            "left:u8(7),right:u8(9)", "right:u8(9),left:u8(7)"))
        positive(root, compiler, cc, "standalone", DECL +
                 "fn main()->unit{let p=Pair{left:u8(7),right:u8(9)};p@left;p@right;unit}")
        positive(root, compiler, cc, "sequential", DECL +
                 "fn main()->unit{let p=Pair{left:u8(7),right:u8(9)};"
                 "loan_write(p@left){|w|replace(w,u8(11));unit};"
                 "loan_write(p@left){|w|replace(w,u8(13));unit};p@left;unit}")
        cases = [
            ("legacy", "p.left;unit", "SOURCE-DOT-RESERVED"),
            ("legacyloan", "loan_write(p.left){|w|unit};unit", "LOCAL-LOAN-PROFILE"),
            ("colonloan", "loan_write(p::left){|w|unit};unit", "LOCAL-LOAN-PROFILE"),
            ("valuequalifier", "p::left;unit", "P6-SUM-QUALIFIER"),
            ("receiver", "p.left(unit);unit", "SOURCE-DOT-RESERVED"),
            ("module", "Foo::make(unit);unit", "P6-SUM-QUALIFIER"),
            ("nested", "p@left@right;unit", "P5-EXPRESSION-UNSUPPORTED"),
            ("wrongbase", "let x=u8(7);x@left;unit", "FIELD-PROFILE"),
            ("ptrbase", "let x=u8(7);let token=loan_read(x){|r|ptr_from_ref(r)};token@left;unit", "FIELD-PROFILE"),
            ("refbase", "let x=u8(7);loan_read(x){|r|r@left};unit", "FIELD-PROFILE"),
            ("arbitrarybase", "u8(7)@left;unit", "P5-EXPECTED-ITEM-END"),
            ("unknown", "let bad=p@nope;unit", "FIELD-UNKNOWN-FIELD"),
            ("wrongtype", "loan_write(p@left){|w|replace(w,unit)};unit", "P3-TYPE-MISMATCH"),
            ("authority", "replace(p@left,u8(11));unit", "P3-WRITE-REF-REQUIRED"),
            ("escape", "let bad=loan_write(p@left){|w|w};unit", "P8-EXIT-DEPENDENCY"),
            ("range", "loan_write(p@left){|w|replace(w,u8(256))};unit", "V1-U8-LITERAL-RANGE"),
        ]
        for name, body, code in cases:
            negative(root, compiler, name, DECL +
                     f"fn main()->unit{{let p=Pair{{left:u8(7),right:u8(9)}};{body}}}", code)
        for name, body in [
            ("store", "loan_write(p@left){|w|store(w,u8(11));unit};unit"),
            ("call", "let value=u8fn();unit"),
        ]:
            source = root / f"{name}.nl"
            source.write_text(DECL + "fn u8fn()->u8{u8(11)}fn main()->unit{"
                              "let p=Pair{left:u8(7),right:u8(9)};" + body + "}")
            result = run([compiler, str(source)])
            if result.returncode != 4 or result.stdout or "V1-BACKEND-UNSUPPORTED" not in result.stderr:
                raise SystemExit(f"accepted outside backend profile lost distinction: {result}")
        # Output setup failure is an I/O failure after semantic acceptance,
        # never NewLang invalidity. No host compile/executable is attempted.
        if Path("/dev/full").exists():
            with Path("/dev/full").open("wb") as sink:
                import subprocess
                failure = subprocess.run([compiler, str(root / "primary.nl")],
                                         stdout=sink, stderr=subprocess.PIPE, check=False)
            if failure.returncode == 0 or b"V0-C-OUTPUT" not in failure.stderr:
                raise SystemExit("Checked-C output failure not diagnosed")
    print("fixed-field E8: member reads/write/old result/destructure -> strict C17/native; negatives no C/exe")


if __name__ == "__main__":
    main()
