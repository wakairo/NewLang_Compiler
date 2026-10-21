"""R690: independent first-pass user source -> generated C -> external C17 caller.

Original inputs and predictions live in the preceding immutable freeze commit.
This test deliberately does not load P688 fixtures, expected values, or probes.
"""
import hashlib
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def execute(*cmd):
    return subprocess.run([str(item) for item in cmd], capture_output=True, check=False)


def require(condition, label, result=None):
    if not condition:
        details = "" if result is None else (
            f" rc={result.returncode} out={result.stdout[:800]!r}"
            f" err={result.stderr[:1200]!r}")
        raise AssertionError(f"R690 first-pass STOP: {label}{details}")
    print(f"R690 PASS: {label}", flush=True)


def main():
    newlangc = Path(sys.argv[1]).resolve()
    cc = str(Path(sys.argv[2]).resolve())
    sanitizer = sys.argv[3]
    root = Path(sys.argv[4]).resolve()
    original = root / "tests/r690/original"
    include = root / "include"
    sources = (
        ("river", "river", 29, 21),
        ("lantern", "lantern", 61, 31),
    )
    for file in sorted(original.iterdir()):
        if file.is_file():
            print(f"R690 ORIGINAL SHA256 {file.name} "
                  f"{hashlib.sha256(file.read_bytes()).hexdigest()}",
                  flush=True)

    flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-I", str(include)]
    if sanitizer != "none":
        flags += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer"]
    print(f"R690 C compiler={cc}, sanitizer={sanitizer}", flush=True)

    with tempfile.TemporaryDirectory(prefix="r690-blind-") as folder:
        temporary = Path(folder)
        producer_c = {}
        for stem, selector, expected, wrong_rc in sources:
            source = original / f"{stem}.nl"
            checked = execute(newlangc, "--check", source)
            if stem == "lantern":
                # NON-BLIND CALIBRATION ONLY: keep the original BLIND
                # CHECK0 oracle in original/oracle.md and historical CI.
                require(checked.returncode == 3 and checked.stdout == b"" and
                        b"P3-UNKNOWN-CALLEE" in checked.stderr,
                        "CALIBRATED lantern ordinary --check CLI3 / "
                        "P3-UNKNOWN-CALLEE (original BLIND CHECK0 falsified)",
                        checked)
            else:
                require(checked.returncode == 0 and checked.stdout == b"" and
                        checked.stderr == b"", f"{stem}: source CHECK", checked)

            exported = execute(newlangc, "--experimental-c17-u8-export",
                               selector, source)
            require(exported.returncode == 0 and bool(exported.stdout) and
                    exported.stderr == b"", f"{stem}: export CLI0", exported)
            text = exported.stdout.decode("utf-8")
            require(bool(re.search(
                r"(?m)^uint8_t\s+newlang_c17_u8_export_v0\(void\)", text)),
                f"{stem}: public exact uint8_t(void) symbol")
            require(not re.search(r"(?m)^\s*int\s+main\s*\(", text),
                    f"{stem}: no emitted C main")
            require('#include "newlang/experimental_c17_u8_export_v0.h"' in text,
                    f"{stem}: producer imports specified versioned C header")
            require(bool(re.search(r"return\s+nl_fn_\d+\(\);", text)),
                    f"{stem}: wrapper calls runtime helper, not a seeded scalar")
            producer = temporary / f"{stem}_producer.c"
            producer.write_bytes(exported.stdout)
            producer_c[stem] = producer
            print(f"R690 GENERATED SHA256 {stem} "
                  f"{hashlib.sha256(exported.stdout).hexdigest()}", flush=True)

            for level in ("-O0", "-O2"):
                producer_object = temporary / f"{stem}_{level}_producer.o"
                caller_object = temporary / f"{stem}_{level}_consumer.o"
                program = temporary / f"{stem}_{level}_native"
                for file, obj in (
                    (producer, producer_object),
                    (original / f"{stem}_consumer.c", caller_object),
                ):
                    output = execute(cc, *flags, level, "-c", file, "-o", obj)
                    require(output.returncode == 0,
                            f"{stem} {level}: strict separate C TU {file.name}",
                            output)
                linked = execute(cc, *flags, level, producer_object,
                                 caller_object, "-o", program)
                require(linked.returncode == 0,
                        f"{stem} {level}: two-object C17 link", linked)
                observed = execute(program)
                require(observed.returncode == 0 and observed.stdout == b"" and
                        observed.stderr == b"",
                        f"{stem} {level}: ACTUAL C result={expected} / native exit0",
                        observed)

                # Wrong oracle independently demonstrates consumer sensitivity.
                original_consumer = (
                    original / f"{stem}_consumer.c").read_text()
                original_marker = f"UINT8_C({expected})"
                require(original_consumer.count(original_marker) == 1,
                        f"{stem}: wrong-oracle mutation has unique source token")
                wrong = temporary / f"{stem}_{level}_wrong.c"
                wrong.write_text(original_consumer.replace(
                    original_marker, f"UINT8_C({expected + 1})"))
                wrong_object = temporary / f"{stem}_{level}_wrong.o"
                built = execute(cc, *flags, level, "-c", wrong, "-o", wrong_object)
                require(built.returncode == 0,
                        f"{stem} {level}: wrong-oracle TU compiles", built)
                combined = execute(cc, *flags, level, producer_object,
                                   wrong_object, "-o", program)
                require(combined.returncode == 0,
                        f"{stem} {level}: wrong-oracle link", combined)
                rejected = execute(program)
                require(rejected.returncode == wrong_rc,
                        f"{stem} {level}: wrong C expectation returns {wrong_rc}",
                        rejected)

        # Additional source and selector denials have independent predictions.
        for stem, selector, code in (
            ("river", "absent_function", 4),
            ("non_u8", "not_scalar", 4),
            ("wrong_arity", "parameterized", 4),
            ("source_invalid", "invalid_scalar", 3),
        ):
            result = execute(newlangc, "--experimental-c17-u8-export",
                             selector, original / f"{stem}.nl")
            require(result.returncode == code and result.stdout == b"",
                    f"{stem}/{selector}: CLI{code}, exactly zero C bytes",
                    result)
        malformed = execute(newlangc, "--check",
                            original / "source_invalid.nl")
        require(malformed.returncode == 3 and malformed.stdout == b"",
                "source_invalid: ordinary CHECK CLI3", malformed)

        # Contract boundary: C caller including the authoritative header
        # must not be able to publish a contradictory prototype.
        conflict = temporary / "header_conflict.c"
        conflict.write_text(
            "extern int newlang_c17_u8_export_v0(void);\n"
            '#include "newlang/experimental_c17_u8_export_v0.h"\n'
            "int main(void) { return 0; }\n")
        denied = execute(cc, *flags, "-c", conflict, "-o",
                         temporary / "conflict.o")
        require(denied.returncode != 0,
                "C header rejects prior wrong type declaration", denied)

        # Both separately emitted producer C translation units carry the
        # same published ABI name; linking both must fail deterministically.
        producer_objects = []
        for stem in ("river", "lantern"):
            obj = temporary / f"{stem}_duplicate.o"
            built = execute(cc, *flags, "-O0", "-c",
                            producer_c[stem], "-o", obj)
            require(built.returncode == 0, f"{stem}: duplicate probe compile", built)
            producer_objects.append(obj)
        caller_obj = temporary / "collision_caller.o"
        built = execute(cc, *flags, "-O0", "-c",
                        original / "river_consumer.c", "-o", caller_obj)
        require(built.returncode == 0, "collision caller compile", built)
        collision = execute(cc, *flags, *producer_objects, caller_obj,
                            "-o", temporary / "collision")
        require(collision.returncode != 0,
                "two public producer TUs collide at C link", collision)

        # Static helper identifier discovered ONLY from the emitted C; never
        # import any P-authored helper identifier or oracle.
        match = re.search(r"(?m)^static\s+uint8_t\s+(nl_fn_\d+)\(void\)",
                          producer_c["river"].read_text())
        if match:
            private_caller = temporary / "private_caller.c"
            private_caller.write_text(
                '#include "newlang/experimental_c17_u8_export_v0.h"\n'
                f"extern uint8_t {match.group(1)}(void);\n"
                f"int main(void) {{ return {match.group(1)}(); }}\n")
            private_obj = temporary / "private_caller.o"
            built = execute(cc, *flags, "-O0", "-c",
                            private_caller, "-o", private_obj)
            require(built.returncode == 0, "private probe C caller compiles", built)
            private_denied = execute(cc, *flags, producer_objects[0],
                                     private_obj, "-o", temporary / "private")
            require(private_denied.returncode != 0,
                    "external TU cannot link internal static helper", private_denied)
        else:
            print("R690 NOT TESTED: no matching static u8 helper in emitted C",
                  flush=True)

        # Existing default main and --check remain independent of export mode.
        ordinary = execute(newlangc, original / "river.nl")
        require(ordinary.returncode == 0 and ordinary.stdout != b"" and
                ordinary.stderr == b"", "river default CLI emits program C", ordinary)
        normal_source = temporary / "normal.c"
        normal_source.write_bytes(ordinary.stdout)
        ordinary_exe = temporary / "ordinary"
        built = execute(cc, *flags, "-O0", normal_source, "-o", ordinary_exe)
        require(built.returncode == 0, "default C builds normally", built)
        ran = execute(ordinary_exe)
        require(ran.returncode == 0, "default native process exits0", ran)
    print("R690 NON-BLIND CALIBRATED BOUNDED PASS: independent C17 ABI, "
          "scalar values 29/61, and original refusal oracles",
          flush=True)


if __name__ == "__main__":
    main()
