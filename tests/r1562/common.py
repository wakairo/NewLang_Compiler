import hashlib
import json
import os
import pathlib
import subprocess


class Stop(RuntimeError):
    pass


def write_json(path, value):
    path = pathlib.Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def invoke(directory, label, argv, *, cwd=None, guest=False):
    directory = pathlib.Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    if guest:
        ledger_path = directory / "guest-attempts.json"
        ledger = json.loads(ledger_path.read_text()) if ledger_path.exists() else []
        if len(ledger) >= 8:
            raise Stop("current lane/set guest cap8; aggregate cap16 requires C ledger")
        ledger.append({"attempt": len(ledger) + 1, "label": label,
                       "argv": list(map(str, argv)), "state": "attempted"})
        write_json(ledger_path, ledger)
    record = {"argv": list(map(str, argv)), "cwd": str(cwd or directory),
              "guest": guest}
    write_json(directory / (label + ".command.json"), record)
    try:
        run = subprocess.run(record["argv"], cwd=record["cwd"],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             timeout=90, check=False)
        out, err, rc = run.stdout, run.stderr, run.returncode
    except subprocess.TimeoutExpired as exc:
        out, err, rc = exc.stdout or b"", exc.stderr or b"", "TIMEOUT"
    except OSError as exc:
        out, err, rc = b"", str(exc).encode(), "OS_ERROR"
    (directory / (label + ".stdout")).write_bytes(out)
    (directory / (label + ".stderr")).write_bytes(err)
    record.update(returncode=rc, stdout_bytes=len(out), stdout_sha256=sha(out),
                  stderr_bytes=len(err), stderr_sha256=sha(err))
    write_json(directory / (label + ".command.json"), record)
    if guest:
        ledger[-1].update(state="completed", returncode=rc,
                          stdout_sha256=sha(out), stderr_sha256=sha(err))
        write_json(ledger_path, ledger)
    return rc, out, err


def require(condition, message):
    if not condition:
        raise Stop(message)


def source_manifest(config):
    root = pathlib.Path(config["source_dir"]) / "tests" / "r1562"
    manifest = json.loads((root / "manifest.json").read_text())
    for item in manifest["sources"]:
        raw = (root / item["name"]).read_bytes()
        require(len(raw) == item["bytes"] and sha(raw) == item["sha256"],
                "FIRST immutable source hash mismatch: " + item["id"])
    return root, manifest
