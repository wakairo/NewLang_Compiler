"""One-cohort C353 V2 observer. Never changes CTest registration or admission policy."""
from __future__ import annotations
import argparse
import copy
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = Path(__file__).with_name("c353_manifest_v2.json")
ALLOWED = {".github/workflows/t-c353-fast-shadow-v2.yml",
           "tests/tooling/c353_manifest_v2.json", "tests/tooling/c353_shadow_v2.py"}

def require(condition, detail):
    if not condition:
        raise ValueError(detail)

def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))

def write_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")

def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()

def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()

def names(manifest):
    result = sorted(n for group in manifest["groups"].values() for n in group)
    require(len(result) == len(set(result)) == 81, "baseline81 missing/duplicate")
    require(all(re.fullmatch(r"[A-Za-z0-9_.-]+", n) for n in result), "unsafe name")
    return result

def name_digest(values):
    return hashlib.sha256(("\n".join(sorted(values)) + "\n").encode()).hexdigest()

def properties(test):
    return {p["name"]: p["value"] for p in test.get("properties", [])}

def validate_inventory(document, manifest, frontend=False, commands=True):
    tests = document["tests"]
    mapping = {test["name"]: test for test in tests}
    require(len(mapping) == len(tests), "duplicate CTest names")
    count = manifest["expected_partial_count" if frontend else "expected_full_count"]
    digest = manifest["partial_names_sha256" if frontend else "full_names_sha256"]
    require(len(mapping) == count and name_digest(mapping) == digest,
            "full inventory does not match fixed baseline")
    require(sum(bool(properties(t).get("RUN_SERIAL")) for t in tests) == 48,
            "RUN_SERIAL inventory changed")
    selected = set(names(manifest))
    require(selected <= mapping.keys(), "selected CTest missing")
    for name in selected:
        test = mapping[name]
        p = properties(test)
        for key in ("DISABLED", "WILL_FAIL", "SKIP_RETURN_CODE", "SKIP_REGULAR_EXPRESSION",
                    "FIXTURES_REQUIRED", "FIXTURES_SETUP", "FIXTURES_CLEANUP"):
            require(not p.get(key), (name, key, p.get(key)))
        require(set(p.get("DEPENDS", [])) <= selected, (name, "dependency omitted"))
        require(p.get("TIMEOUT", 0) > 0, (name, "missing timeout"))
        if commands:
            require(bool(test.get("command")), (name, "executable not built"))
    require(properties(mapping["p1227_c283_alias_oom.unit"]).get("DEPENDS")
            == ["p1227_oom.unit"], "alias must follow original census")
    require(properties(mapping["p1260_oom.unit"])["TIMEOUT"] == 300,
            "p1260 OOM timeout changed")
    return mapping

def lane_verdict(record, expected):
    if record is None:
        return "PENDING"
    for key in ("head", "checkout", "tree", "base", "app", "attempt",
                "event", "workflow", "inventory"):
        if record.get(key) != expected[key]:
            return "INVALID"
    if record.get("status") in ("queued", "in_progress", "waiting", "pending", "requested"):
        return "PENDING"
    if record.get("status") != "completed" or record.get("conclusion") != "success":
        return "STOP"
    return "SUCCESS"

def validate_junit(text, manifest):
    root = ET.fromstring(text)
    cases = list(root.iter("testcase"))
    actual = [case.attrib["name"] for case in cases]
    require(sorted(actual) == names(manifest) and len(set(actual)) == len(actual),
            "executed inventory missing/extra/duplicate")
    rows = []
    for case in cases:
        name = case.attrib["name"]
        require(case.get("status", "run") == "run", (name, "not run"))
        require(case.find("failure") is None and case.find("error") is None
                and case.find("skipped") is None, (name, "failure/error/skip"))
        output = "\n".join("".join(node.itertext())
                           for node in case if node.tag in ("system-out", "system-err"))
        for marker in manifest["output_markers"].get(name, []):
            require(marker in output, (name, "lost original assertion marker", marker))
        rows.append({"name": name, "seconds": float(case.get("time", "0")),
                     "status": "PASS", "output": output})
    return rows

def selfcheck(manifest):
    expected = dict(head="H", checkout="M", tree="T", base="B", app=15368,
                    attempt=1, event="pull_request", workflow="W", inventory="81")
    good = dict(expected, status="completed", conclusion="success")
    cases = [("exact", good, "SUCCESS"), ("missing", None, "PENDING"),
             ("queued", dict(good, status="queued", conclusion=None), "PENDING")]
    for state in ("failure", "cancelled", "skipped", "neutral", "timed_out"):
        cases.append((state, dict(good, conclusion=state), "STOP"))
    for key in ("head", "checkout", "tree", "base", "app", "attempt",
                "event", "workflow", "inventory"):
        cases.append(("wrong-" + key, dict(good, **{key: "WRONG"}), "INVALID"))
    for name, record, wanted in cases:
        require(lane_verdict(record, expected) == wanted, name)
    test_xml = ET.Element("testsuite")
    for name in names(manifest):
        case = ET.SubElement(test_xml, "testcase", name=name, status="run", time="0")
        ET.SubElement(case, "system-out").text = "\n".join(
            manifest["output_markers"].get(name, []))
    require(len(validate_junit(ET.tostring(test_xml), manifest)) == 81, "XML control")
    rejected = 0
    for defect in ("missing", "duplicate", "failure", "skipped", "marker"):
        bad = copy.deepcopy(test_xml)
        if defect == "missing":
            bad.remove(bad[0])
        elif defect == "duplicate":
            bad.append(copy.deepcopy(bad[0]))
        elif defect in ("failure", "skipped"):
            ET.SubElement(bad[0], defect)
        else:
            target = next(c for c in bad
                          if c.get("name") in manifest["output_markers"])
            target.find("system-out").text = "truncated"
        try:
            validate_junit(ET.tostring(bad), manifest)
        except ValueError:
            rejected += 1
    require(rejected == 5, "XML negative controls not rejected")
    return {"identity_cases": len(cases), "XML_negative_cases": rejected,
            "status": "PASS", "admission": "FULL_REQUIRED"}

def verify_event(output, manifest):
    require(os.environ.get("GITHUB_EVENT_NAME") == "pull_request", "PR event only")
    require(os.environ.get("GITHUB_WORKFLOW") == manifest["expected_workflow"], "workflow identity")
    selector = (ROOT / "docs/reference/CURRENT_SPEC.md").read_bytes()
    require(hashlib.sha256(selector).hexdigest() == manifest["selector_sha256"], "canonical selector changed")
    require(manifest["canonical_spec"].encode() in selector.split(b"```text\n", 1)[1].split(b"```", 1)[0], "wrong canonical Draft")
    require(git("rev-parse", manifest["baseline_sha"] + "^{tree}") == manifest["baseline_tree"], "base tree mismatch")

    require(os.environ.get("GITHUB_RUN_ATTEMPT") == "1", "attempt1 only")
    require(os.environ.get("GITHUB_REPOSITORY") == "wakairo/NewLang_Compiler", "repo")
    event = read_json(os.environ["GITHUB_EVENT_PATH"])
    require(event["action"] == "opened", "one natural opened event only")
    pr = event["pull_request"]
    require(pr["draft"] is True, "DRAFT NEVERMERGE only")
    require(pr["head"]["ref"] == manifest["branch"], "dedicated T branch only")
    require(pr["head"]["repo"]["full_name"] == os.environ["GITHUB_REPOSITORY"],
            "same-repository branch only")
    require(pr["base"]["sha"] == manifest["baseline_sha"], "base drift: STOP")
    head, checkout = pr["head"]["sha"], git("rev-parse", "HEAD")
    require(checkout == os.environ["GITHUB_SHA"], "checkout identity mismatch")
    parents = git("show", "-s", "--format=%P", "HEAD").split()
    require(parents == [manifest["baseline_sha"], head], "synthetic merge parents")
    tree = git("rev-parse", "HEAD^{tree}")
    require(tree == git("rev-parse", head + "^{tree}"), "candidate tree mismatch")
    changed = set(git("diff", "--name-only", manifest["baseline_sha"], "HEAD").splitlines())
    require(changed == ALLOWED, ("unexpected production/required change", sorted(changed)))
    output.mkdir(parents=True, exist_ok=False)
    record = dict(created_at=utc(), source_sha=manifest["baseline_sha"], head_sha=head,
                  checkout_sha=checkout, tree=tree, parents=parents,
                  pr=pr["number"], event="pull_request", action=event["action"],
                  run_id=os.environ["GITHUB_RUN_ID"], attempt=1,
                  job_key=os.environ["GITHUB_JOB"], workflow=os.environ["GITHUB_WORKFLOW"],
                  canonical_spec=manifest["canonical_spec"], selector_sha256=manifest["selector_sha256"],
                  version=2, changed_paths=sorted(changed),
                  inventory=names(manifest), admission="FULL_REQUIRED")
    write_json(output / "identity-FIRST.json", record)
    print("C353 V2 IDENTITY " + json.dumps(record, sort_keys=True), flush=True)

def collect_raw(build, output, manifest):
    destination = output / "original-harness-raw"
    suffixes = {".json", ".stdout", ".stderr", ".out", ".err", ".log", ".txt",
                ".points", ".sha256", ".c", ".h"}
    for dirname in manifest["raw_directories"]:
        source = build / dirname
        if not source.is_dir():
            continue
        for path in source.rglob("*"):
            if path.is_file() and not path.is_symlink() and path.suffix in suffixes:
                relative = path.relative_to(build)
                target = destination / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)

def run_suite(build, output, sanitizer, manifest):
    identity = read_json(output / "identity-FIRST.json")
    require(git("rev-parse", "HEAD") == identity["checkout_sha"], "checkout changed")
    require(sanitizer in ("address", "undefined"), "two sanitizer lanes only")
    stages = {}
    def execute(argv, label):
        start = time.monotonic()
        print("C353 V2 START " + label + " " + utc(), flush=True)
        with (output / (label + "-FIRST.log")).open("x") as raw:
            result = subprocess.run(list(map(str, argv)), cwd=ROOT,
                                    stdout=raw, stderr=subprocess.STDOUT, check=False)
        stages[label] = dict(seconds=time.monotonic() - start, exit=result.returncode)
        write_json(output / "stages.json", stages)
        print("C353 V2 STAGE " + label + " " + json.dumps(stages[label]), flush=True)
        require(result.returncode == 0, (label, "original FIRST failure"))
    workers = int(subprocess.check_output(["nproc"], text=True).strip())
    require(workers > 0, "invalid worker count")
    execute(["cmake", "-S", ROOT, "-B", build, "-DCMAKE_BUILD_TYPE=Debug",
             "-DNEWLANG_SANITIZER=" + sanitizer,
             "-DCMAKE_PROJECT_INCLUDE=" + str(ROOT / "cmake/independent-r-tests-hook.cmake")],
            "configure")
    execute(["cmake", "--build", build, "--parallel", str(workers)], "build")
    with (output / "full-ctest-inventory.json").open("x") as raw:
        subprocess.run(["ctest", "--test-dir", str(build), "--show-only=json-v1"],
                       stdout=raw, check=True)
    inventory = read_json(output / "full-ctest-inventory.json")
    mapping = validate_inventory(inventory, manifest)
    selected = names(manifest)
    regex = "^(" + "|".join(re.escape(n) for n in selected) + ")$"
    with (output / "selected-ctest-inventory.json").open("x") as raw:
        subprocess.run(["ctest", "--test-dir", str(build), "--show-only=json-v1", "-R", regex],
                       stdout=raw, check=True)
    actual = read_json(output / "selected-ctest-inventory.json")["tests"]
    require(sorted(t["name"] for t in actual) == selected, "CTest regex mismatch")
    cache = (build / "CMakeCache.txt").read_text()
    require("NEWLANG_SANITIZER:STRING=" + sanitizer in cache, "wrong host sanitizer")
    commands = read_json(build / "compile_commands.json")
    main = [c for c in commands if c["file"] == str(ROOT / "src/main.c")]
    require(main and all("-fsanitize=" + sanitizer in c.get("command", "") for c in main),
            "compiler host sanitizer not present")
    write_json(output / "selection-before-execution.json",
               dict(names=selected, selected_count=81, full_count=len(mapping),
                    full_names_sha256=name_digest(mapping), sanitizer=sanitizer,
                    workers=min(workers, 3), source_sha=manifest["baseline_sha"],
                    checkout_sha=identity["checkout_sha"], full_pending=True,
                    admission="FULL_REQUIRED",
                    binary_sha256=hashlib.sha256((build / "newlangc").read_bytes()).hexdigest()))
    start = time.monotonic()
    try:
        execute(["ctest", "--test-dir", build, "--parallel", str(min(workers, 3)),
                 "--output-on-failure", "--no-tests=error", "-R", regex,
                 "--output-junit", output / "results-FIRST.xml",
                 "--output-log", output / "ctest-FIRST.log"], "ctest")
        rows = validate_junit((output / "results-FIRST.xml").read_text(), manifest)
        case_dir = output / "per-case"
        case_dir.mkdir()
        for row in rows:
            (case_dir / (row["name"] + ".txt")).write_text(row.pop("output"))
        write_json(output / "per-case-results.json", rows)
    finally:
        collect_raw(build, output, manifest)
    result = dict(status="FAST_OBSERVED", completed_at=utc(), selected_passed=81,
                  sanitizer=sanitizer, ctest_seconds=stages["ctest"]["seconds"],
                  post_ctest_record_seconds=time.monotonic()-start-stages["ctest"]["seconds"],
                  stages=stages, full_status="FULL_PENDING", admission="FULL_REQUIRED",
                  late_only_bug_found=None, full_catches_before_next_code=None,
                  dependent_commits_built_before_late_error=None,
                  rework_time_if_late_error=None)
    write_json(output / "fast-FIRST-result.json", result)
    print("C353 V2 FAST FIRST " + json.dumps(result, sort_keys=True), flush=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("selfcheck", "verify-event", "inventory", "run"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--build", type=Path)
    parser.add_argument("--sanitizer", choices=("address", "undefined"))
    parser.add_argument("--frontend", action="store_true")
    args = parser.parse_args()
    manifest = read_json(MANIFEST)
    names(manifest)
    try:
        if args.command == "selfcheck":
            print(json.dumps(selfcheck(manifest), sort_keys=True))
        elif args.command == "verify-event":
            verify_event(args.output, manifest)
        elif args.command == "inventory":
            document = read_json(args.build)
            validate_inventory(document, manifest, args.frontend, commands=False)
            print("C353 V2 inventory/closure/timeout/serial PASS")
        else:
            run_suite(args.build.resolve(), args.output.resolve(), args.sanitizer, manifest)
    except Exception as error:
        if args.output and args.output.is_dir():
            path = args.output / "failure-FIRST.json"
            if not path.exists():
                write_json(path, {"at": utc(), "status": "STOP", "error": str(error)})
        raise

if __name__ == "__main__":
    main()

