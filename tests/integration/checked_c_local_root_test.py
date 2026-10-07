"""Issue #110 actual-source P1/N1/N2; public checked-view E1-E6 in unit tests.

C behavior is downstream execution evidence, never a provenance/stability check.
"""
from pathlib import Path
import re
import sys
import tempfile
from checked_c_v0_test import negative, positive, run


def main() -> None:
    compiler, cc = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="newlang-local-root-") as temp:
        root = Path(temp)
        p1 = """fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r|
        ptr_from_ref(r)
    };
    loan_read_ptr(p) { |r2|
        ptr_from_ref(r2);
        unit
    };
    unit
}
"""
        positive(root, compiler, cc, "p1", p1)
        emitted = (root / "p1.c").read_text(encoding="utf-8")
        local = re.search(r"const uint8_t (nl_local_\d+) = 7;", emitted)
        first = re.search(r"const uint8_t \*const (nl_local_\d+) = &(nl_local_\d+);", emitted)
        receive = re.search(r"const uint8_t \*const (nl_local_\d+) = (nl_loan_result_\d+);", emitted)
        if not local or not first or not receive or first[2] != local[1]:
            raise SystemExit("E7: missing checked local/address/result receiving")
        second = re.search(rf"const uint8_t \*const (nl_local_\d+) = {receive[1]};", emitted)
        if not second or second[1] == first[1] or emitted.count(f"(void){second[1]};") != 2:
            raise SystemExit("E7: missing fresh scoped ref and actual ptr_from_ref use")
        if "unchecked" in p1 or "loan_read" in emitted or "ptr_from_ref" in emitted:
            raise SystemExit("E8/authority boundary lost")

        mutation = """fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r|
        ptr_from_ref(r)
    };
    let old = loan_write(x) { |w|
        replace(w, u8(9))
    };
    old;
    loan_read_ptr(p) { |r2|
        ptr_from_ref(r2);
        unit
    };
    unit
}
"""
        positive(root, compiler, cc, "mutation", mutation)
        mutated = (root / "mutation.c").read_text(encoding="utf-8")
        mutable_root = re.search(r"(?m)^\s*uint8_t (nl_local_\d+) = 7;$", mutated)
        first_ptr = (
            re.search(
                rf"const uint8_t \*const (nl_local_\d+) = &"
                rf"({mutable_root[1] if mutable_root else 'never'});",
                mutated,
            )
            if mutable_root
            else None
        )
        persisted = re.search(
            r"const uint8_t \*const (nl_local_\d+) = (nl_loan_result_\d+);",
            mutated,
        )
        write_ref = (
            re.search(
                rf"uint8_t \*const (nl_local_\d+) = &{mutable_root[1]};",
                mutated,
            )
            if mutable_root
            else None
        )
        replace_old = re.search(
            r"const uint8_t (nl_replace_old_\d+) = \*(nl_local_\d+);",
            mutated,
        )
        if (
            not mutable_root
            or not first_ptr
            or not persisted
            or not write_ref
            or not replace_old
            or replace_old[2] != write_ref[1]
            or f"*{write_ref[1]} = 9;" not in mutated
        ):
            raise SystemExit(f"stable-root lowering lost root/write/replace identity:\n{mutated}")
        scalar_result = re.search(r"uint8_t (nl_loan_result_\d+);", mutated)
        old_binding = (
            re.search(
                rf"const uint8_t (nl_local_\d+) = {scalar_result[1]};",
                mutated,
            )
            if scalar_result
            else None
        )
        if not scalar_result or not old_binding:
            raise SystemExit("replace old-value loan result was not received")
        reacquired = re.search(
            rf"const uint8_t \*const (nl_local_\d+) = {persisted[1]};",
            mutated,
        )
        if not reacquired or reacquired[1] in (first_ptr[1], write_ref[1]):
            raise SystemExit("preexisting ptr was not used for fresh reacquisition")
        if (
            "loan_write" in mutated
            or "replace(" in mutated
            or "loan_read" in mutated
            or "ptr_from_ref" in mutated
            or "unchecked" in mutated
        ):
            raise SystemExit("stable-root Checked-C authority boundary lost")

        # Ordinary ptr Copy use and discarded ptr-producing loan remain bounded.
        positive(root, compiler, cc, "copy", "fn main()->unit{let x=u8(7);let p=loan_read(x){|r|ptr_from_ref(r)};let q=p;loan_read_ptr(q){|s|ptr_from_ref(s);unit};unit}\n")
        positive(root, compiler, cc, "discard", "fn main()->unit{let x=u8(7);loan_read(x){|r|ptr_from_ref(r)};unit}\n")
        positive(root, compiler, cc, "unit", "fn main()->unit{let x=u8(7);loan_read(x){|r|ptr_from_ref(r);unit};unit}\n")
        positive(root, compiler, cc, "nested", "fn main()->unit{let x=u8(7);loan_read(x){|r|loan_read(x){|s|ptr_from_ref(s);unit};ptr_from_ref(r);unit};unit}\n")
        negative(root, compiler, "n1", "fn main()->unit{let x=u8(7);let bad=loan_read(x){|r|r};unit}\n", "P8-EXIT-DEPENDENCY")
        negative(root, compiler, "write_escape", "fn main()->unit{let x=u8(7);let bad=loan_write(x){|w|w};unit}\n", "P8-EXIT-DEPENDENCY")
        negative(root, compiler, "n2", "fn main()->unit{let p={let x=u8(7);loan_read(x){|r|ptr_from_ref(r)}};loan_read_ptr(p){|r2|unit};unit}\n", "P3-STALE-POINTER")
        # Accepted local-root loan yielding scalar remains backend-unsupported.
        unsupported = root / "unsupported.nl"
        unsupported.write_text("fn main()->unit{let x=u8(7);let y=loan_read(x){|r|u8(9)};y;unit}\n", encoding="utf-8")
        for source in (
            "fn main()->unit{let x=u8(7);let y=loan_read(x){|r|u8(9)};y;unit}\n",
            # A valid persistent token for a body-local ended root is beyond
            # this C-address profile; don't emit an indeterminate C pointer.
            "fn main()->unit{let x=u8(7);let p=loan_read(x){|r|let y=u8(9);let q=loan_read(y){|s|ptr_from_ref(s)};q};unit}\n",
        ):
            unsupported.write_text(source, encoding="utf-8")
            result = run([compiler, str(unsupported)])
            if result.returncode != 4 or result.stdout or "V1-BACKEND-UNSUPPORTED" not in result.stderr:
                raise SystemExit(f"backend-unsupported boundary lost: {result}")
    print("local-root + stable-root mutation actual-source C17/native; write escape/stale reject before emission; semantic evidence in unit tests")


if __name__ == "__main__":
    main()
