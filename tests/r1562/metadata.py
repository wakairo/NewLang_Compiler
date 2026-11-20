import json
import os
import pathlib
import re
import sys

from common import invoke, require, sha, write_json

config = json.loads(pathlib.Path(sys.argv[1]).read_text())
out = pathlib.Path(config["evidence_dir"]) / "metadata"
out.mkdir(parents=True, exist_ok=True)
write_json(out / "build-config.json", config)
rc, raw, err = invoke(out, "ctest-inventory",
                      ["ctest", "--test-dir", config["binary_dir"], "-N"])
require(rc == 0, "CTest inventory command failed")
names = re.findall(rb"(?m)^\s*Test\s+#\d+:\s*(\S+)\s*$", raw)
total = re.search(rb"(?m)^Total Tests:\s*(\d+)\s*$", raw)
own = [n.decode() for n in names if n.startswith(b"r1562.")]
require(total is not None and int(total[1]) == len(names), "inventory total")
require(len(set(names)) == len(names), "duplicate CTest names")
require(sorted(own) == ["r1562.cli_native.integration",
                        "r1562.public.integration"], "own tests not registered")
inventory = {"count": len(names), "unique": len(set(names)), "own": own,
             "raw_sha256": sha(raw), "raw_bytes": len(raw),
             "tests_executed": 0, "format": "plain ctest-N; no JSON parsing"}
write_json(out / "inventory.json", inventory)
print("R1562_REGISTRATION " + json.dumps(inventory, sort_keys=True), flush=True)
rc, head, err = invoke(out, "checkout-head", ["git", "rev-parse", "HEAD"],
                      cwd=config["source_dir"])
require(rc == 0, "checkout HEAD unavailable")
rc, commit, err = invoke(out, "checkout-commit", ["git", "cat-file", "-p", "HEAD"],
                        cwd=config["source_dir"])
require(rc == 0, "raw commit object unavailable")
headers = commit.split(b"\n\n", 1)[0].decode()
actual = {"head": head.decode().strip(),
          "tree": re.findall(r"(?m)^tree ([0-9a-f]{40})$", headers),
          "ordered_parents": re.findall(r"(?m)^parent ([0-9a-f]{40})$", headers),
          "event_name": os.environ.get("GITHUB_EVENT_NAME"),
          "event_sha": os.environ.get("GITHUB_SHA"),
          "ref": os.environ.get("GITHUB_REF"),
          "run_id": os.environ.get("GITHUB_RUN_ID"),
          "run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT")}
event_path = os.environ.get("GITHUB_EVENT_PATH")
if event_path:
    event = json.loads(pathlib.Path(event_path).read_text())
    pr = event.get("pull_request")
    if pr:
        actual["pr"] = {"number": pr["number"],
                        "head_sha": pr["head"]["sha"],
                        "head_ref": pr["head"]["ref"],
                        "base_sha": pr["base"]["sha"],
                        "base_ref": pr["base"]["ref"]}
        require(pr["base"]["sha"] == config["fixed_p"],
                "PR base drift from released P069")
require(not actual["event_sha"] or actual["head"] == actual["event_sha"],
        "checkout/event SHA drift")
rc, _, _ = invoke(out, "fixed-parent-object-presence",
                   ["git", "cat-file", "-e", config["fixed_p"] + "^{commit}"],
                   cwd=config["source_dir"])
actual["fixed_p_object_present"] = rc == 0
write_json(out / "checkout.json", actual)
print("R1562_CHECKOUT " + json.dumps(actual, sort_keys=True), flush=True)
print("R1562_METADATA_ONLY CTest0 SOURCE0 PUBLIC0 NATIVE0", flush=True)
