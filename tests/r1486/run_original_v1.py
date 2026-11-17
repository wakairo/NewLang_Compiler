#!/usr/bin/env python3
"""Independent R1486 V1 source-stage probe; no synthetic proof or unsafe RX native."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

SOURCE_DIGESTS = {
    "RZ": "805824a8fe2096a4cf4d75aed9e9efd68e00617287f85a22e27ddbe08859506e",
    "RN": "f545f5a2a800a4c8a7ff17ab5786e0327bcc89466520764c302e1f2923e1b118",
    "RX": "b7881457090bdd30ad619775ef3359becf0d01ef5d966c670618fbe958c3d08b",
}
ORACLE_DIGEST = "31c887fe301df3a1c8415c86e020b0fa95724d190c0a3fbc75de4a4a85013e20"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def invoke(args, label):
    try:
        proc = subprocess.run(args, capture_output=True, timeout=40, check=False)
    except (OSError, subprocess.TimeoutExpired) as exc:
        print(f"R1486_STAGE={label}_ENVIRONMENT_FAILURE detail={exc}", flush=True)
        raise
    stderr = proc.stderr.decode("utf-8", errors="replace")
    print(f"R1486_STAGE={label} exit={proc.returncode} stderr={stderr[:3000]!r}", flush=True)
    return proc


def refuse(label, why):
    print(f"R1486_FIRST_REFUSAL={label} detail={why}", flush=True)
    print("R1486_LAYERS_NOT_REACHED=owned_public,C17,native,physical,OOM", flush=True)
    return 1


def main():
    if len(sys.argv) != 7:
        raise SystemExit("R1486 harness: expected CLI, C compiler, sanitizer, case, source, oracle")
    cli, cc, sanitizer, case, src_text, oracle_text = sys.argv[1:]
    if case not in SOURCE_DIGESTS or sanitizer not in ("none", "address", "undefined"):
        raise SystemExit("R1486: unsupported case or sanitizer")
    src, oracle_path = Path(src_text), Path(oracle_text)
    if digest(src) != SOURCE_DIGESTS[case] or digest(oracle_path) != ORACLE_DIGEST:
        raise SystemExit("R1486_INPUT_INTEGRITY_FAIL: original V1 bytes or oracle changed")
    oracle = json.loads(oracle_path.read_text(encoding="ascii"))
    if (oracle.get("oracle_id") != "R1483-C1075-ORACLE-V1"
            or oracle["programs"][case]["sha256"] != SOURCE_DIGESTS[case]):
        raise SystemExit("R1486_ORACLE_ID_FAIL")
    print(f"R1486_V1={case} source_sha={SOURCE_DIGESTS[case]} oracle_sha={ORACLE_DIGEST}", flush=True)
    print("R1486_SOURCE_ELIGIBILITY=UNCERTAIN_PRE_EXECUTION; FIRST intact", flush=True)
    check = invoke([cli, "--check", str(src)], "SOURCE_CHECK_CLI")
    diag = check.stderr.decode("utf-8", errors="replace")
    if case == "RX":
        if check.returncode == 0:
            print("R1486_SAFETY_STOP: wrong live Domain accepted by --check; RX C17/native PROHIBITED", flush=True)
            return 2
        if re.search(r"error\(parser\)\[", diag):
            return refuse("SOURCE_REJECTED", "RX parser rejected; bad-D CHECK not reached")
        if not re.search(r"error\(semantic\)\[", diag):
            return refuse("UNCLASSIFIED_REJECTION", "RX no structured semantic error")
        if not re.search(r"domain|loan|stabil|govern|ptr|provenance", diag, re.IGNORECASE):
            return refuse("SEMANTIC_REJECTED_OTHER", "RX diagnostic lacks matching-D evidence")
        print("R1486_RX=SEMANTIC_REJECTED; inspect diagnostic for wrong-live-D specificity", flush=True)
        print("R1486_RX_C17_NATIVE=NOT_REACHED; no unsafe guest executed", flush=True)
        return 0
    if check.returncode != 0:
        layer = ("SOURCE_REJECTED" if re.search(r"error\(parser\)\[", diag)
                 else "SEMANTIC_CHECK_REJECTED" if re.search(r"error\(semantic\)\[", diag)
                 else "SOURCE_OR_CHECK_REFUSED")
        return refuse(layer, f"positive {case}: rc={check.returncode}")
    if check.stdout or check.stderr:
        return refuse("CHECK_OUTPUT_CONTRACT", "positive source CHECK had unexpected output")
    print("R1486_CHECK_CLI=ACCEPTED; public owned receipt not independently inspected", flush=True)
    emitted = invoke([cli, str(src)], "C17_EMIT_CLI")
    if emitted.returncode != 0 or not emitted.stdout or emitted.stderr:
        return refuse("C17_EMITTER", "compiler did not successfully emit clean C17")
    with tempfile.TemporaryDirectory(prefix="r1486-v1-") as td:
        root = Path(td)
        c_file = root / "guest.c"
        c_file.write_bytes(emitted.stdout)
        for opt in ("-O0", "-O2"):
            exe = root / ("guest" + opt[1:])
            flags = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", opt]
            if sanitizer != "none":
                flags += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all"]
            build = invoke(flags + [str(c_file), "-o", str(exe)], "C17_COMPILE_" + opt[1:])
            if build.returncode != 0:
                return refuse("STRICT_C17", f"failed {opt}")
            guest = invoke([str(exe)], "NATIVE_" + opt[1:])
            if guest.returncode != 0 or guest.stderr:
                return refuse("NATIVE_RUNTIME", f"failed {opt}")
    print("R1486_NATIVE_SMOKE=STRICT_C17_O0_O2_EXIT0; NOT physical/original-value evidence", flush=True)
    print("R1486_SOURCE_ISSUED_OWNED=NOT_DIRECTLY_INSPECTED", flush=True)
    print("R1486_H2_GENUINE_RESULT_AND_ORIGINAL_H_COUNTERS=NOT_OBSERVABLE", flush=True)
    print("R1486_H0_H1_FORCED_ALLOCATION_FAILURE=NOT_TESTED", flush=True)
    print("R1486_OOM_NO_CLOBBER=NOT_TESTED", flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, KeyError, OSError, subprocess.TimeoutExpired) as exc:
        print(f"R1486_HARNESS_ERROR={exc}; no semantic/native PASS", flush=True)
        sys.exit(1)
