#!/usr/bin/env python3
"""R1511 V1 observer. Never invent guest authority or execute unsafe N1."""
import hashlib
import pathlib
import subprocess
import sys
import tempfile

HOME = pathlib.Path(__file__).resolve().parent
FROZEN = {
    "P0_zero_field0_direct.nl": (3740, "9599e624d4d71cc8483e2af71fded103403d60aecd70a86126691743c4b73a8d"),
    "P1_nonzero_field1_link_return.nl": (5569, "79854b065f84bbe20593ea856d781f61fa005de08bee41faaaff31c99e937703"),
    "N1_wrong_governing_domain.nl": (3829, "30d3c0a52461c655eb2fc464f10044e72844c35aefdc079d333f4410ead144a1"),
    "N2_read_after_EndRoot.nl": (3740, "0e52b4b5d378fac47685ded804e2e4398b24e11db648fd06cdf621e70a7a42c7"),
    "N3_cross_allocation_deallocate.nl": (3740, "5de644ad366e5b8baaed71d9209bf07d40fe0becf4245fba99d852ccdbb01490"),
    "N4_wrong_named_formal.nl": (3739, "3d3b1096eba0de17c840cbf09aca0cd5a157e21ee1314c62d5aa82a757f9bfab"),
    "first_oracle.txt": (6245, "d94838e05f50f19c723cb84e4abc2f27353421dd0aaf764fce8feaaed89883d2"),
}
POS = ("P0_zero_field0_direct.nl", "P1_nonzero_field1_link_return.nl")
NEG = tuple(x for x in FROZEN if x.startswith("N"))


def check_frozen():
    for name, (count, sha) in FROZEN.items():
        p = HOME / ("sources" if name.endswith(".nl") else "") / name
        data = p.read_bytes()
        actual = hashlib.sha256(data).hexdigest()
        print("FROZEN", name, len(data), actual, flush=True)
        if len(data) != count or actual != sha:
            raise RuntimeError("FIRST TAMPER / WRONG BYTE COUNT: " + name)
    print("FIRST INTEGRITY PASS (7/7; not source eligibility)", flush=True)


def run(label, args):
    cmd = list(map(str, args))
    print("R1511 COMMAND", label, repr(cmd), flush=True)
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, errors="replace",
                           check=False, timeout=90)
    except subprocess.TimeoutExpired:
        raise RuntimeError("TIMEOUT: " + label)
    print("R1511 EXIT", label, p.returncode, flush=True)
    print("R1511 STDOUT", label, "\n" + p.stdout, flush=True)
    print("R1511 STDERR", label, "\n" + p.stderr, flush=True)
    return p.returncode


def native_smoke(name, compiler, cc, sanitizer):
    with tempfile.TemporaryDirectory(prefix="r1511-") as root:
        c = pathlib.Path(root) / "guest.c"
        if run(name + "/EMIT", [compiler, "--emit-c17-to", c, HOME / "sources" / name]):
            print("EMIT NOT_REACHED; no C17/native authority", flush=True)
            return False
        for opt in (0, 2):
            exe = pathlib.Path(root) / ("guest" + str(opt))
            args = [cc, "-std=c17", "-pedantic", "-Wall", "-Wextra",
                    "-Werror", "-O" + str(opt), c, "-o", exe]
            if sanitizer in ("address", "undefined"):
                args.extend(["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"])
            if run(name + "/C17_O" + str(opt), args):
                return False
            if run(name + "/NATIVE_O" + str(opt), [exe]):
                return False
    print("NATIVE SMOKE ONLY, NOT owned/identity, H0/H1 forced OOM, "
          "actual T byte, free order or no-clobber proof", flush=True)
    return True


def cohort(compiler, cc, sanitizer):
    print("ORIGINAL FIRST: SOURCE_ELIGIBILITY_UNCERTAIN", flush=True)
    print("P0 conditional H2 actual B.color u8(0) to same original T field0 "
          "after B->A frees; P1 H2 Some(q) actual back.pitch u8(173) to "
          "field1 after front->back frees. H0/H1: no second T write.", flush=True)
    checked = {}
    stop = False
    for name in POS:
        rc = run(name + "/CHECK", [compiler, "--check", HOME / "sources" / name])
        checked[name] = rc
        if rc:
            print("FIRST REFUSAL at ordinary CLI CHECK; SOURCE_ELIGIBILITY_UNCERTAIN/"
                  "NOT_REACHED for checked-owned/C17/native. Exact parser vs "
                  "semantic refusal requires diagnostic inspection; not "
                  "a compiler safety finding.", flush=True)
            stop = True
        else:
            print("CHECK exit0 != canonical source law or authentic owned proof",
                  flush=True)
            if not native_smoke(name, compiler, cc, sanitizer):
                print("STOP: C17/native emitted or executed mismatch/unsupported",
                      flush=True)
                stop = True
    for name in NEG:
        rc = run(name + "/CHECK", [compiler, "--check", HOME / "sources" / name])
        if name.startswith("N1_"):
            print("N1: false original Domain under unchecked is human-assumed "
                  "precondition/UB if false; CHECK exit", rc,
                  "not automatically machine-proof unsoundness. "
                  "NEGATIVE NATIVE EXECUTION PROHIBITED.", flush=True)
        elif not rc and not checked[POS[0]]:
            print("STOP C: unexpectedly accepted negative", name,
                  "against accepted P0; do not infer forged issuer from CLI alone",
                  flush=True)
            stop = True
        elif rc:
            print("NEGATIVE REFUSED; exact SOURCE vs SEMANTIC layer unverified "
                  "without diagnostic classification", name, flush=True)
        else:
            print("BASE P0 NOT ADMITTED; cannot claim semantic negative grade",
                  name, flush=True)
    print("FIRST V1 result:", "STOP/NOT_REACHED" if stop else
          "CLI+native smoke only; bounded original physical/owned/OOM oracle "
          "STILL NOT VERIFIED", flush=True)
    return 1 if stop else 0


def main():
    if len(sys.argv) != 5:
        raise RuntimeError("usage: probe.py integrity|cohort newlangc cc sanitizer")
    mode, compiler, cc, sanitizer = sys.argv[1:]
    check_frozen()
    if mode == "integrity":
        return 0
    if mode != "cohort":
        raise RuntimeError("unknown mode")
    return cohort(compiler, cc, sanitizer)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, OSError) as e:
        print("R1511 STOP apparatus:", e, file=sys.stderr, flush=True)
        sys.exit(1)
