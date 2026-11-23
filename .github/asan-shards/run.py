"""Run a direct ASan check against a slice of the current CTest registration."""

from collections import Counter, defaultdict
from graphlib import CycleError, TopologicalSorter
from hashlib import sha256
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
from xml.etree import ElementTree

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
HEAVY = {"p1291_named_oom.integration", "p1299_named_oom.integration"}
LIGHT = ("light-a", "light-b")
WORKERS = {"heavy": 2, "light-a": 3, "light-b": 3}
START = re.compile(r"^\s*Start\s+(\d+):[ \t]*(.*)$", re.MULTILINE)
PASS = re.compile(r"^\s*(\d+)/(\d+)\s+Test\s+#(\d+):\s+(.+?)\s+\.+\s+Passed\s+[\d.]+ sec\s*$", re.MULTILINE)


def require(condition, message):
    if not condition:
        raise SystemExit(f"ASan shard: {message}")


def inventory(ctest, evidence, selection=()):
    with evidence.open("wb") as output:
        subprocess.run([*ctest, "--show-only=json-v1", *selection], stdout=output, check=True)
    data = json.loads(evidence.read_bytes())
    require(data["kind"] == "ctestInfo" and data["version"]["major"] == 1, "unsupported CTest inventory format")
    tests = {}
    for row in data["tests"]:
        name = row["name"]
        require(name and "\n" not in name and "\r" not in name and name not in tests,
                f"invalid or duplicate registered test {name!r}")
        require(row.get("command"), f"missing executable command for {name!r}")
        props = {item["name"]: item["value"] for item in row["properties"]}
        require(len(props) == len(row["properties"]), f"duplicate CTest properties for {name!r}")
        tests[name] = (row["command"], props)
    return tests


def measured_hints(path):
    # Frozen from the measured Light A/B assignment in #1688. Membership comes from CTest.
    hints = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        name, shard = line.split("\t")
        require(name not in hints and shard in LIGHT, f"invalid measured hint for {name!r}")
        hints[name] = shard
    return hints


def fingerprint(names):
    data = json.dumps(sorted(names), ensure_ascii=True, separators=(",", ":"))
    return sha256(data.encode("ascii")).hexdigest()


def plan(tests, hints):
    require(HEAVY <= tests.keys(), "approved Heavy test missing from the CTest inventory")
    require(all(tests[name][1].get("RUN_SERIAL") is True for name in HEAVY), "original Heavy RUN_SERIAL missing")
    neighbors = {name: set() for name in tests}
    prerequisites = {}
    constrained = defaultdict(list)
    for name, (_, props) in tests.items():
        dependencies = set(props.get("DEPENDS", []))
        require(dependencies <= tests.keys(), f"missing CTest dependency for {name!r}: {dependencies - tests.keys()}")
        prerequisites[name] = dependencies
        for dependency in dependencies:
            neighbors[name].add(dependency)
            neighbors[dependency].add(name)
        for property_name in ("FIXTURES_SETUP", "FIXTURES_REQUIRED", "FIXTURES_CLEANUP"):
            for fixture in props.get(property_name, []):
                constrained["fixture", fixture].append(name)
        for lock in props.get("RESOURCE_LOCK", []):
            constrained["lock", lock].append(name)
        if props.get("RESOURCE_GROUPS") or props.get("GENERATED_RESOURCE_SPEC_FILE"):
            constrained["resource allocation", ""].append(name)
    try:
        TopologicalSorter(prerequisites).prepare()
    except CycleError as failure:
        raise SystemExit(f"ASan shard: CTest dependency cycle: {failure.args[1]}") from failure
    for first, *others in constrained.values():
        neighbors[first].update(others)
        for name in others:
            neighbors[name].add(first)

    shards = {shard: set() for shard in WORKERS}
    visited = set()
    for first in sorted(tests):
        if first in visited:
            continue
        component = set()
        remaining = {first}
        while remaining:
            name = remaining.pop()
            component.add(name)
            remaining.update(neighbors[name] - component)
        visited.update(component)
        if component & HEAVY:
            require(component <= HEAVY, f"CTest constraints cross the fixed Heavy boundary: {sorted(component)}")
            shard = "heavy"
        else:
            votes = Counter(hints.get(name) for name in component)
            shard = LIGHT[int(fingerprint(component)[-1], 16) % 2]
            if votes[LIGHT[0]] != votes[LIGHT[1]]:
                shard = max(LIGHT, key=votes.__getitem__)
        shards[shard].update(component)
    require(all(shards.values()), "at least one ASan shard has an empty selection")
    require(set.union(*shards.values()) == tests.keys() and sum(map(len, shards.values())) == len(tests),
            "ASan shard names do not form an exact partition")
    return shards


def check_heavy_overlay(original, overlay):
    require(list(original) == list(overlay), "Heavy configure changed the CTest inventory or ordering")
    for name, (command, props) in original.items():
        expected = {key: value for key, value in props.items() if name not in HEAVY or key != "RUN_SERIAL"}
        require(overlay[name] == (command, expected), f"unexpected CTest command or property change for {name!r}")


def selection_args(tests, selected):
    # CTest's explicit index list keeps exact names and works with the existing CMake 3.28 floor.
    numbers = {index: name for index, name in enumerate(tests, 1) if name in selected}
    return ["-I", "0,0,0," + ",".join(map(str, numbers))], numbers


def check_outcomes(junit, log, numbers):
    suite = ElementTree.parse(junit).getroot()
    cases = suite.findall("./testcase")
    expected_count = len(numbers)
    require(suite.get("tests") == str(expected_count) and len(cases) == expected_count, "JUnit test count differs")
    require(all(suite.get(key) == "0" for key in ("failures", "disabled", "skipped")), "JUnit failed, disabled, or skipped test")
    require(Counter(case.get("name") for case in cases) == Counter(numbers.values()), "JUnit executed names differ")
    require(all(case.get("status") == "run" and not any(case.find(key) is not None for key in ("failure", "error", "skipped"))
                for case in cases), "JUnit reports a test did not pass")

    output = log.read_text(encoding="utf-8", errors="replace")
    expected = Counter((index, name.strip()) for index, name in numbers.items())
    starts = Counter((int(index), name.strip()) for index, name in START.findall(output))
    passes = PASS.findall(output)
    require(starts == expected and Counter((int(index), name.strip()) for _, _, index, name in passes) == expected,
            "CTest started/passed names differ")
    require({int(position) for position, _, _, _ in passes} == set(range(1, expected_count + 1)) and
            {int(total) for _, total, _, _ in passes} == {expected_count}, "CTest progress counts differ")

    active = set()
    workers = 0
    for line in output.splitlines():
        if match := START.fullmatch(line):
            index = int(match[1])
            require(index not in active, "duplicate active CTest index")
            active.add(index)
            workers = max(workers, len(active))
        if match := PASS.fullmatch(line):
            index = int(match[3])
            require(index in active, "CTest completion without a start")
            active.remove(index)
    require(not active, "CTest starts without a completion")
    return workers


def main(shard, workers, build, evidence):
    require(shard in WORKERS and 1 <= workers <= WORKERS[shard], "incorrect shard or worker count")
    evidence.mkdir(parents=True, exist_ok=True)
    ctest = ["ctest", "--test-dir", str(build)]
    tests = inventory(ctest, evidence / "inventory.json")
    shards = plan(tests, measured_hints(HERE / "light-hints.tsv"))
    selected = shards[shard]
    plan_record = {
        "inventory": {"count": len(tests), "sha256": fingerprint(tests)},
        "shard": shard,
        "slices": {lane: {"count": len(names), "sha256": fingerprint(names), "names": sorted(names)}
                   for lane, names in shards.items()},
    }
    (evidence / "plan.json").write_text(json.dumps(plan_record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"ASan inventory count={len(tests)} sha256={fingerprint(tests)}", flush=True)
    for lane, names in shards.items():
        print(f"ASan slice {lane} count={len(names)} sha256={fingerprint(names)}", flush=True)

    registration = tests
    if shard == "heavy":
        subprocess.run(["cmake", "-S", str(ROOT), "-B", str(build), f"-DCMAKE_PROJECT_INCLUDE={HERE / 'heavy.cmake'}"], check=True)
        registration = inventory(ctest, evidence / "heavy-inventory.json")
        check_heavy_overlay(tests, registration)
    selection, numbers = selection_args(registration, selected)
    chosen = inventory(ctest, evidence / "selection.json", ["--no-tests=error", *selection])
    require(chosen == {name: registration[name] for name in selected}, "actual selected command, property, or name differs")

    junit = evidence / "ctest.xml"
    log = evidence / "ctest.log"
    result = subprocess.run([*ctest, "--parallel", str(workers), *selection, "--no-tests=error", "--output-on-failure",
                             "--output-junit", str(junit), "--output-log", str(log)], check=False)
    last_test = build / "Testing/Temporary/LastTest.log"
    if last_test.is_file():
        shutil.copyfile(last_test, evidence / last_test.name)
    require(result.returncode == 0, f"CTest exited {result.returncode}")
    observed = check_outcomes(junit, log, numbers)
    require(1 <= observed <= workers, "CTest observed workers outside the configured limit")
    print(f"ASan {shard} passed={len(selected)} skipped=0 configured_workers={workers} observed_workers={observed}", flush=True)


if __name__ == "__main__":
    require(len(sys.argv) == 5, "usage: run.py SHARD WORKERS BUILD_DIR EVIDENCE_DIR")
    main(sys.argv[1], int(sys.argv[2]), Path(sys.argv[3]).resolve(), Path(sys.argv[4]).resolve())
