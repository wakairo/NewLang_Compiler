"""C907: publish only R1421's already-executed CTest LastTest.log receipts.

No subprocess, source parse, semantic CHECK, native execution, or owner creation.
Run as an additive if:always() step after the existing job's final step.
"""

import pathlib
import re
import sys


CASES = {
    b"r1421_author_route_public_check.unit": b"positive",
    b"r1421_wrong_domain_public_check.unit": b"wrong",
    b"r1421_ended_original_public_check.unit": b"ended",
}
HEADER = re.compile(rb"^\s*[0-9]+/[0-9]+\s+Testing:\s+(\S+)\s*$")
MAX_LOG_BYTES = 64 * 1024 * 1024
MAX_CASE_BYTES = 256 * 1024


def emit(message):
    print("R1421_RECEIPT " + message, flush=True)


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in ("build", "build-frontend"):
        emit("FAIL invalid fixed build-directory argument")
        return 2

    build = pathlib.Path(sys.argv[1])
    if not build.is_dir():
        emit("FAIL stage=BUILD_NOT_REACHED directory_missing")
        return 1
    if not (build / "CMakeCache.txt").is_file():
        emit("FAIL stage=CONFIGURE_NOT_REACHED cache_missing")
        return 1
    registration = build / "CTestTestfile.cmake"
    if not registration.is_file():
        emit("FAIL stage=REGISTRATION_NOT_REACHED ctest_file_missing")
        return 1
    registered = registration.read_bytes()
    missing_registration = sorted(
        name.decode("ascii") for name in CASES if name not in registered
    )
    if missing_registration:
        emit(
            "FAIL stage=REGISTRATION_NOT_REACHED missing_cases="
            + ",".join(missing_registration)
        )
        return 1
    if not (build / "r1421_public_observer").is_file():
        emit("FAIL stage=BUILD_NOT_REACHED observer_binary_missing")
        return 1

    log = build / "Testing" / "Temporary" / "LastTest.log"
    if not log.is_file():
        emit("FAIL stage=TEST_NOT_REACHED_OR_LOG_MISSING LastTest.log_missing")
        return 1
    raw = log.read_bytes()
    if not raw or len(raw) > MAX_LOG_BYTES:
        emit("FAIL stage=LOG_INVALID empty_or_oversized")
        return 1

    lines = raw.splitlines(keepends=True)
    boundaries = []
    for index, line in enumerate(lines):
        match = HEADER.fullmatch(line.rstrip(b"\r\n"))
        if match:
            boundaries.append((index, match.group(1)))
    found = {name: [] for name in CASES}
    for pos, (start, name) in enumerate(boundaries):
        if name not in found:
            continue
        end = boundaries[pos + 1][0] if pos + 1 < len(boundaries) else len(lines)
        body = b"".join(lines[start:end])
        found[name].append(body)

    failures = 0
    for name, case in CASES.items():
        label = name.decode("ascii")
        blocks = found[name]
        if len(blocks) != 1:
            emit(f"FAIL stage=CASE_LOG_MISSING_OR_DUPLICATE name={label} count={len(blocks)}")
            failures += 1
            continue
        body = blocks[0]
        if len(body) > MAX_CASE_BYTES:
            emit(f"FAIL stage=CASE_LOG_OVERSIZED name={label} bytes={len(body)}")
            failures += 1
            continue

        emit(f"BEGIN name={label} case={case.decode('ascii')} bytes={len(body)}")
        sys.stdout.buffer.write(body)
        if not body.endswith(b"\n"):
            sys.stdout.buffer.write(b"\n")
        sys.stdout.buffer.flush()
        emit(f"END name={label}")

        # Real source/check results are printed verbatim; no synthetic success.
        required = (
            b"R1421 FIRST_PRESERVED case=" + case,
            b"R1421 phase=source_create",
            b"R1421 phase=parse_function_unit",
            b"R1421 C902 CHECK_ONLY_COMPLETE case=" + case,
        )
        missing = [str(i) for i, needle in enumerate(required) if needle not in body]
        if missing:
            emit(f"FAIL stage=CASE_PHASE_MISSING name={label} required_indices={','.join(missing)}")
            failures += 1
        if not (
            b"R1421 result=NOT_REACHED" in body
            or b"R1421 positive_accepted=1" in body
        ):
            emit(f"FAIL stage=TERMINAL_CLASSIFICATION_MISSING name={label}")
            failures += 1

    if failures:
        emit(f"FAIL case_receipts={len(CASES) - failures}/{len(CASES)}")
        return 1
    emit("COMPLETE all_three_original_cases_raw_output_preserved=1 "
         "no_semantic_replay=1 no_proof_inference=1")
    return 0


if __name__ == "__main__":
    sys.exit(main())
