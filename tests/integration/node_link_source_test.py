"""Actual-source semantic gate; recursive Node emission is still unsupported."""
from pathlib import Path
import sys
import tempfile
from checked_c_v0_test import negative, run


def unsupported(root: Path, compiler: str, name: str, text: str) -> None:
    source = root / f"{name}.nl"
    source.write_text(text, encoding="utf-8")
    first = run([compiler, str(source)])
    second = run([compiler, str(source)])
    if (first.returncode != 4 or second.returncode != 4 or
            first.stdout or second.stdout or
            "V1-BACKEND-UNSUPPORTED" not in first.stderr or
            first.stderr != second.stderr):
        raise SystemExit(f"{name}: lost semantic/backend boundary: {first}")


def main() -> None:
    compiler, witness = sys.argv[1:]
    declaration = "struct Node{next:Option<ptr<Node>>,payload:u8}"
    prefix = ("let tail=Node{next:Option<ptr<Node>>::None,payload:u8(2)};"
              "let p=loan_read(tail){|r|ptr_from_ref(r)};"
              "let head=Node{next:Option<ptr<Node>>::None,payload:u8(1)};")
    with tempfile.TemporaryDirectory(prefix="newlang-node-link-") as temp:
        root = Path(temp)
        actual = Path(witness).read_text(encoding="utf-8")
        unsupported(root, compiler, "two_roots", actual)
        unsupported(root, compiler, "same_variant", declaration +
                    "fn main()->unit{" + prefix +
                    "let a=loan_write(head@next){|w|replace(w,Option<ptr<Node>>::Some(p))};"
                    "let b=loan_write(head@next){|w|replace(w,Option<ptr<Node>>::Some(p))};"
                    "let c=loan_write(head@next){|w|replace(w,Option<ptr<Node>>::None)};unit}")
        unsupported(root, compiler, "metadata_names",
                    "struct Cell{link:Option<ptr<Cell>>,data:u8}fn main()->unit{"
                    "let c=Cell{link:Option<ptr<Cell>>::None,data:u8(1)};"
                    "let p=loan_read(c){|r|ptr_from_ref(r)};"
                    "loan_write(c@link){|w|replace(w,Option<ptr<Cell>>::Some(p))};"
                    "c@link;loan_read_ptr(p){|r|unit};unit}")
        controls = [
            ("payload", "head@payload;", "NODE-LINK-FIELD-PROFILE"),
            ("unknown", "head@absent;", "FIELD-UNKNOWN-FIELD"),
            ("ptr_base", "p@next;", "FIELD-PROFILE"),
            ("ref_base", "loan_read(head){|r|r@next};", "FIELD-PROFILE"),
            ("root_write", "loan_write(head){|w|unit};", "LOCAL-LOAN-PROFILE"),
            ("scope_escape", "let escaping=loan_write(head@next){|w|w};", "P8-EXIT-DEPENDENCY"),
            ("wrong_value", "loan_write(head@next){|w|replace(w,u8(7))};", "P3-TYPE-MISMATCH"),
            ("old_dot", "head.next;", "SOURCE-DOT-RESERVED"),
            ("nested", "head@next@payload;", "P5-EXPRESSION-UNSUPPORTED"),
            ("expression_base", "{head}@next;", "P5-EXPECTED-ITEM-END"),
            ("sum_separator", "head::next;", "P6-SUM-QUALIFIER"),
            ("same_name", "let Node=u8(7);Node@next;", "FIELD-PROFILE"),
        ]
        for name, operation, code in controls:
            negative(root, compiler, name,
                     declaration + "fn main()->unit{" + prefix + operation + "unit}", code)
        negative(root, compiler, "stale", declaration +
                 "fn main()->unit{let p={let n=Node{next:Option<ptr<Node>>::None,payload:u8(1)};"
                 "loan_read(n){|r|ptr_from_ref(r)}};loan_read_ptr(p){|r|unit};unit}",
                 "P3-STALE-POINTER")
        if any(path.suffix != ".nl" for path in root.iterdir()):
            raise SystemExit("semantic-only gate left a C/object/executable artifact")
    print("Node @link: actual source/evidence accepted; deterministic backend unsupported; no C/native output")


if __name__ == "__main__":
    main()
