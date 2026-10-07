"""Draft 17.21 declaration/type-graph catch-up; no recursive C backend."""
from pathlib import Path
import sys
import tempfile
from checked_c_v0_test import negative, run


def main() -> None:
    compiler = sys.argv[1]
    decl = "struct Node{next:Option<ptr<Node>>,payload:u8}"
    body = "fn main()->unit{let n=Node{next:Option<ptr<Node>>.None,payload:u8(7)};unit}"
    with tempfile.TemporaryDirectory(prefix="newlang-recursive-") as temp:
        root = Path(temp)
        for i, text in enumerate((decl + body, body + decl)):
            source = root / f"accepted-{i}.nl"
            source.write_text(text, encoding="utf-8")
            first = run([compiler, str(source)])
            second = run([compiler, str(source)])
            if (first.returncode != 4 or second.returncode != 4 or
                    first.stdout or second.stdout or
                    "V1-BACKEND-UNSUPPORTED" not in first.stderr or
                    first.stderr != second.stderr):
                raise SystemExit(f"semantic acceptance/backend boundary lost: {first}")
        negative(root, compiler, "unknown",
                 "struct Node{next:Option<ptr<Missing>>,payload:u8}" + body,
                 "P3-UNKNOWN-TYPE")
        negative(root, compiler, "payload", decl +
                 "fn main()->unit{let n=Node{next:Option<ptr<Node>>.Some(u8(7)),payload:u8(7)};unit}",
                 "P6-PAYLOAD-TYPE")
        negative(root, compiler, "field", decl +
                 "fn main()->unit{let n=Node{next:Option<ptr<Node>>.None,payload:u8(7)};n.next;unit}",
                 "FIELD-PROFILE")
        if list(root.glob("*.c")) or list(root.glob("*.o")):
            raise SystemExit("recursive catch-up emitted a backend artifact")
    print("recursive declaration: actual source accepted; backend unsupported; invalid source emits no C")


if __name__ == "__main__":
    main()
