"""C156 release-specific Git integration audit, not a future feature gate.

No compiler authority is created. Negative tests mutate in-memory inventories,
not Production, frozen inputs or a live compiler graph. Run from a clean full
Git checkout of the consolidation candidate, with Python bytecode disabled.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
DATA = Path(__file__).resolve().parent
PLAN = json.loads((DATA / "plan-v1.json").read_bytes())


class IntegrationError(AssertionError):
    pass


def require(condition, message):
    if not condition:
        raise IntegrationError(message)


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args])


def inventory(revision):
    result = {}
    for record in git("ls-tree", "-rz", "--full-tree", revision).split(b"\0"):
        if record:
            header, name = record.split(b"\t", 1)
            result[name.decode()] = tuple(header.decode().split())
    return result


def ancestry(states):
    require(states["main"] and states["main_first_parent"], "main ancestry")
    require(states["tip"], "fixed tip ancestry")
    require(not states["excluded"], "stopped sibling inherited")


def contents(actual, expected):
    for name, entry in expected.items():
        require(actual.get(name) == entry, "fixed content changed: " + name)
    for name in actual.keys() - expected.keys():
        require(name == PLAN["new_document"] or name.startswith("tests/p1067/"),
                "unexpected integration input: " + name)


def main():
    require(sys.argv[1:] in ([], ["--self-test"]), "usage: check_integration.py [--self-test]")
    require(not git("status", "--porcelain"), "dirty checkout")
    main_ref, tip, excluded = (PLAN[k] for k in ("main", "tip", "excluded"))
    ancestor = lambda ref: subprocess.run(
        ["git", "-C", str(ROOT), "merge-base", "--is-ancestor", ref, "HEAD"],
        capture_output=True).returncode
    codes = {k: ancestor(ref) for k, ref in
             (("main", main_ref), ("tip", tip), ("excluded", excluded))}
    require(set(codes.values()) <= {0, 1}, "unavailable ancestry")
    states = {k: code == 0 for k, code in codes.items()}
    states["main_first_parent"] = main_ref.encode() in git(
        "rev-list", "--first-parent", "HEAD").splitlines()
    ancestry(states)
    expected, authority = inventory(tip), inventory(main_ref)
    for name in PLAN["main_authority"]:
        require(name in authority, "missing main authority: " + name)
        expected[name] = authority[name]
    actual = inventory("HEAD")
    contents(actual, expected)
    for name, digest in json.loads((DATA / "pre-run-v1.json").read_bytes())["files"].items():
        require(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == digest,
                "pre-run input changed: " + name)
    frozen = json.loads((ROOT / "tests/p1062/source-freeze-v1.json").read_bytes())
    require(len(frozen["files"]) == 22, "original source count")
    for name, digest in frozen["files"].items():
        require(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == digest,
                "original source changed: " + name)
    ignored = git("ls-files", "--others", "--ignored", "--exclude-standard", "-z")
    require(not any(b"__pycache__" in n or n.endswith(b".pyc")
                    for n in ignored.split(b"\0")), "ambient Python cache")
    if sys.argv[1:]:
        faults = [
            ("main-authority", PLAN["main_authority"][0]),
            ("Production", "src/checked_c_node.c"),
            ("first-RED", "tests/p982/evidence/first-reds.json"),
            ("v12", "tests/p990/current-freeze-v12.json"),
        ]
        for label, name in faults:
            changed = dict(actual)
            changed.pop(name)
            try:
                contents(changed, expected)
            except IntegrationError as error:
                require(str(error) == "fixed content changed: " + name,
                        "wrong inventory rejection")
                print("P1067 REJECT", label, flush=True)
            else:
                raise IntegrationError("accepted changed " + label)
        changed = dict(actual)
        changed["src/unapproved.c"] = ("100644", "blob", "0" * 40)
        try:
            contents(changed, expected)
        except IntegrationError as error:
            require(str(error) == "unexpected integration input: src/unapproved.c",
                    "wrong extra-input rejection")
            print("P1067 REJECT unapproved input", flush=True)
        else:
            raise IntegrationError("accepted extra input")
        try:
            ancestry(dict(states, excluded=True))
        except IntegrationError as error:
            require(str(error) == "stopped sibling inherited", "wrong ancestry rejection")
            print("P1067 REJECT stopped-sibling graph fault", flush=True)
        else:
            raise IntegrationError("accepted stopped sibling")
    print(json.dumps(dict(P1067="integration audit PASS",
                          HEAD=git("rev-parse", "HEAD").decode().strip(),
                          tree=git("rev-parse", "HEAD^{tree}").decode().strip(),
                          inherited_entries=len(expected),
                          original_source_inputs=22,
                          Production="byte-identical fixed tip",
                          main_authority="exact fixed main",
                          independent_R_inputs="not imported")), flush=True)


if __name__ == "__main__":
    main()

