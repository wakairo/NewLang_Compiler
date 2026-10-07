"""Revised V1 acceptance: P1/E2/E3 + N1/N2. Negative unsigned is Deferred.

E1 is independently checked in u8_literal_test through public checked views.
No oracle inference, source prelude, I/O or NewLang integer exit convention.
"""
from pathlib import Path
import re
import sys
import tempfile
from checked_c_v0_test import negative, positive, run


def main() -> None:
    compiler, cc = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="newlang-v1-") as temp:
        root = Path(temp)
        source = "fn main()->unit{let x=u8(7);x;unit}\n"
        positive(root, compiler, cc, "p1", source)
        emitted = (root / "p1.c").read_text(encoding="utf-8")
        match = re.search(r"const uint8_t (nl_local_\d+) = 7;", emitted)
        if match is None or emitted.count(f"(void){match[1]};") != 2:
            raise SystemExit("P1/E2: missing checked value initializer or explicit local use")
        # Decimal payload spelling is not copied into C (octal would mean 63).
        positive(root, compiler, cc, "decimal", "fn main()->unit{let x=u8(007);x;unit}\n")
        decimal = (root / "decimal.c").read_text(encoding="utf-8")
        if " = 7;" not in decimal or "007" in decimal:
            raise SystemExit("checked decimal value was not used for emission")
        positive(root, compiler, cc, "unused", "fn main()->unit{let x=u8(255);unit}\n")
        negative(root, compiler, "n1", "fn main()->unit{let x=u8(256);unit}\n", "V1-U8-LITERAL-RANGE")
        negative(root, compiler, "n2", "fn main()->unit{let x=7;unit}\n", "P5-EXPECTED-EXPRESSION")
        # Checker-accepted u8 function result is beyond the V1 backend subset.
        unsupported = root / "unsupported.nl"
        unsupported.write_text("fn scalar()->u8{u8(7)}\nfn main()->unit{scalar();unit}\n", encoding="utf-8")
        result = run([compiler, str(unsupported)])
        if result.returncode != 4 or result.stdout or "V1-BACKEND-UNSUPPORTED" not in result.stderr:
            raise SystemExit(f"backend-unsupported boundary lost: {result}")
    print("V1 P1/E2/E3 execute; N1/N2 reject before emission; E1 in u8_body.unit")


if __name__ == "__main__":
    main()
