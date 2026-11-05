#!/usr/bin/env python3
"""One finite, offline, synthetic T1166 suite. No actual-P test is claimed."""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import stat
import subprocess
import sys
import tempfile
import time
import traceback
from unittest.mock import patch

sys.dont_write_bytecode = True
from fixed_git_context import FixedGitContext, SnapshotError, git_environment

HERE = Path(__file__).resolve().parent
CASES = [
    "nested_context_and_complete_inventory",
    "consumer_argv_and_environment",
    "missing_commit",
    "wrong_fixed_tree",
    "missing_source_blob",
    "index_missing_entry",
    "index_nonzero_stage",
    "index_wrong_mode",
    "index_wrong_blob",
    "working_bytes_changed",
    "working_mode_changed",
    "same_bytes_untracked_rejected",
    "copy_failure_cleanup",
    "outward_input_symlinks_rejected",
    "owned_cleanup_does_not_follow_symlink",
    "cleanup_ownership_refusal",
    "independent_objects_survive_source_unavailable",
    "bootstrap_io_failure_cleanup_or_retained_receipt",
]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def encoded(data):
    return None if data is None else base64.b64encode(data).decode("ascii")


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def rejected(action, marker=None):
    try:
        action()
    except (SnapshotError, OSError) as error:
        if marker is not None:
            require(marker in str(error), "unexpected rejection: " + str(error))
        return {"expected_rejection": type(error).__name__, "message": str(error)}
    raise AssertionError("fault was not rejected")


class Recorder:
    def __init__(self, path):
        self.path = path
        self.count = 0

    def __call__(self, argv, *, cwd, env, input=None):
        self.count += 1
        started = time.monotonic()
        record = {"sequence": self.count, "argv": list(argv), "cwd": str(cwd),
                  "stdin_base64": encoded(input),
                  "stdin_sha256": None if input is None else digest(input)}
        try:
            result = subprocess.run(argv, cwd=cwd, env=env, input=input,
                                    capture_output=True, timeout=30, check=False)
            record.update(returncode=result.returncode,
                          stdout_base64=encoded(result.stdout),
                          stderr_base64=encoded(result.stderr),
                          stdout_sha256=digest(result.stdout),
                          stderr_sha256=digest(result.stderr))
            return result
        except BaseException as error:
            record.update(exception=type(error).__name__, message=str(error),
                          stdout_base64=encoded(getattr(error, "stdout", None)),
                          stderr_base64=encoded(getattr(error, "stderr", None)))
            raise
        finally:
            record["elapsed_seconds"] = time.monotonic() - started
            with self.path.open("a", encoding="utf-8") as stream:
                stream.write(json.dumps(record, sort_keys=True) + "\n")


def filesystem_fingerprint(root):
    """Evidence of source invariance, never the snapshot/index authority."""
    result = {}
    for current, directories, files in os.walk(root, followlinks=False):
        for name in sorted(directories + files):
            path = Path(current) / name
            relative = path.relative_to(root).as_posix()
            info = path.lstat()
            row = {"mode": stat.S_IMODE(info.st_mode)}
            if path.is_symlink():
                row.update(kind="symlink", target=os.readlink(path))
            elif path.is_file():
                row.update(kind="file", size=info.st_size,
                           mtime_ns=info.st_mtime_ns, sha256=digest(path.read_bytes()))
            else:
                row.update(kind="directory")
            result[relative] = row
    return result


def size_of(root):
    return sum(path.stat().st_size for path in root.rglob("*") if path.is_file())


class Suite:
    def __init__(self, root, evidence, spec, recorder):
        self.root, self.evidence, self.spec, self.command = root, evidence, spec, recorder
        self.source = root / "parent-repository"
        self.copy = root / "fixed-copy"
        self.source.mkdir()
        self.copy.mkdir()
        self.parent = self.source / "build" / "nested"
        self.parent.mkdir(parents=True)
        self.sentinel = root / "outside-sentinel"
        self.sentinel.write_text("outside must remain untouched\n")
        self.sentinel_hash = digest(self.sentinel.read_bytes())
        self.clean_env = git_environment()
        self.git = shutil.which("git", path=self.clean_env.get("PATH"))
        require(self.git is not None, "Git unavailable")
        template = root / "empty-template"
        template.mkdir()
        self.git_call(self.source, "init", "--quiet", "--object-format=sha1", "--template=" + str(template))
        for oid, item in spec["objects"].items():
            data = base64.b64decode(item["data_base64"])
            observed = self.git_call(self.source, "hash-object", "-w", "-t", item["kind"], "--stdin", input=data)
            require(observed.strip().decode() == oid, "frozen synthetic object identity mismatch")
        (self.source / ".git" / "HEAD").write_text(spec["commit"] + "\n")
        for name, item in spec["files"].items():
            data = base64.b64decode(item["data_base64"])
            require(digest(data) == item["sha256"], "frozen fixture input hash mismatch")
            for base in (self.source, self.copy):
                target = base / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
                target.chmod(0o755 if item["mode"] == "100755" else 0o644)
        self.git_call(self.source, "read-tree", spec["tree"])
        self.baseline = filesystem_fingerprint(self.source)
        (evidence / "source-before.json").write_text(json.dumps(self.baseline, indent=2, sort_keys=True) + "\n")
        self.env = dict(os.environ)
        self.env.update(GIT_DIR=str(self.source / ".git"), GIT_WORK_TREE=str(self.source),
                        GIT_INDEX_FILE=str(self.source / ".git" / "index"),
                        GIT_COMMON_DIR=str(self.source / ".git"),
                        GIT_OBJECT_DIRECTORY=str(root / "absent-objects"),
                        GIT_ALTERNATE_OBJECT_DIRECTORIES=str(root / "absent-alternates"),
                        GIT_CONFIG_COUNT="1", GIT_CONFIG_KEY_0="core.bare", GIT_CONFIG_VALUE_0="true",
                        GIT_CONFIG_GLOBAL=str(root / "absent-config"),
                        GIT_EXEC_PATH=str(root / "absent-exec"), GIT_CEILING_DIRECTORIES="/",
                        GIT_SSH_COMMAND="must-never-be-run", GIT_SSL_NO_VERIFY="true",
                        CC="t1166-preserved-cc", NEWLANG_SANITIZER="t1166-preserved-san",
                        ASAN_OPTIONS="t1166-preserved-asan")
        self.inventory = {name: tuple(item) for name, item in spec["inventory"].items()}
        self.receipts = []

    def git_call(self, cwd, *args, input=None):
        result = self.command([self.git, "-C", str(cwd), *args], cwd=str(cwd), env=self.clean_env, input=input)
        require(result.returncode == 0, "fixture/fault Git command failed: " + repr(args))
        return result.stdout

    def context(self, source=None, copy=None, commit=None, tree=None):
        started = time.monotonic()
        ctx = FixedGitContext.create(source or self.source, copy or self.copy, self.parent,
                                     commit or self.spec["commit"], tree or self.spec["tree"],
                                     environ=self.env, command=self.command)
        self.receipts.append(dict(ctx.receipt, preparation_seconds=time.monotonic() - started,
                                  filesystem_bytes=size_of(ctx.root)))
        return ctx

    def source_unchanged(self):
        require(filesystem_fingerprint(self.source) == self.baseline, "source repository or metadata changed")
        require(not list(self.parent.iterdir()), "temporary context residue")
        require(digest(self.sentinel.read_bytes()) == self.sentinel_hash, "outside sentinel changed")

    def nested_context_and_complete_inventory(self):
        with self.context() as ctx:
            require(ctx.inventory == self.inventory, "six frozen paths/modes/blob IDs differ")
            require(ctx.work.is_relative_to(self.parent), "context is not nested under parent repository")
            require(ctx.work != self.source, "parent repository selected")
            require(ctx.verify() == len(self.inventory), "inventory readback incomplete")
            require(not (ctx.gitdir / "objects/info/alternates").exists(), "shared object store")
            require((ctx.gitdir / "shallow").read_text().strip() == self.spec["commit"], "fixed commit boundary missing")
            root = ctx.root
        require(not root.exists(), "success cleanup incomplete")
        return {"leaves": len(self.inventory), "modes": ["100644", "100755"]}

    def consumer_argv_and_environment(self):
        code = ("import base64,json,os,subprocess,sys; "
                "a=['git','rev-parse','--show-toplevel'];r=subprocess.run(a,capture_output=True); "
                "print(json.dumps({'argv':sys.argv[1:],'root':r.stdout.decode().strip(),"
                "'child_argv':a,'child_returncode':r.returncode,"
                "'child_stdout_base64':base64.b64encode(r.stdout).decode(),"
                "'child_stderr_base64':base64.b64encode(r.stderr).decode(),"
                "'CC':os.environ['CC'],'SAN':os.environ['NEWLANG_SANITIZER'],"
                "'ASAN':os.environ['ASAN_OPTIONS'],'git_keys':sorted(k for k in os.environ if k.startswith('GIT_'))}));sys.exit(r.returncode)")
        argv = [sys.executable, "-c", code, "literal space", "$(not-a-shell)", "--unchanged"]
        with self.context() as ctx:
            result = ctx.run(argv)
            require(result.returncode == 0, "consumer failed")
            observed = json.loads(result.stdout)
            require(observed["argv"] == argv[3:], "consumer argv changed")
            require(observed["root"] == str(ctx.work), "consumer escaped context")
            require((observed["CC"], observed["SAN"], observed["ASAN"]) ==
                    (self.env["CC"], self.env["NEWLANG_SANITIZER"], self.env["ASAN_OPTIONS"]), "non-Git env changed")
            require(observed["git_keys"] == sorted(k for k in git_environment({}) if k.startswith("GIT_")), "ambient Git env leaked")
            require(all(ctx.environ[k] == v for k, v in self.env.items() if not k.startswith("GIT_")), "non-Git env changed")
            # A nonzero synchronous consumer return is preserved; cleanup still occurs.
            failed = ctx.run([sys.executable, "-c", "import sys;sys.exit(23)"])
            require(failed.returncode == 23, "consumer status changed")
        return {"argv_preserved": True, "non_git_environment_preserved": True, "nonzero_return_preserved": 23}

    def missing_commit(self):
        return rejected(lambda: self.context(commit="f" * 40), "Git failed")

    def wrong_fixed_tree(self):
        other = next(oid for oid, item in self.spec["objects"].items() if item["kind"] == "tree" and oid != self.spec["tree"])
        return rejected(lambda: self.context(tree=other), "commit/tree")

    def missing_source_blob(self):
        broken = self.root / "source-missing-blob"
        shutil.copytree(self.source, broken)
        oid = self.inventory["fixtures/raw.bin"][1]
        (broken / ".git/objects" / oid[:2] / oid[2:]).unlink()
        return rejected(lambda: self.context(source=broken), "Git failed")

    def index_missing_entry(self):
        with self.context() as ctx:
            self.git_call(ctx.work, "update-index", "--force-remove", "inherited/inputs.json")
            return rejected(ctx.verify, "index inventory")

    def index_nonzero_stage(self):
        name = "inherited/inputs.json"
        mode, oid = self.inventory[name]
        data = ("0 " + "0" * 40 + "\t" + name + "\n" + mode + " " + oid + " 1\t" + name + "\n").encode()
        with self.context() as ctx:
            self.git_call(ctx.work, "update-index", "--index-info", input=data)
            return rejected(ctx.verify, "index stage")

    def index_wrong_mode(self):
        name = "inherited/inputs.json"
        with self.context() as ctx:
            self.git_call(ctx.work, "update-index", "--cacheinfo", "100755," + self.inventory[name][1] + "," + name)
            return rejected(ctx.verify, "index inventory")

    def index_wrong_blob(self):
        with self.context() as ctx:
            self.git_call(ctx.work, "update-index", "--cacheinfo", "100644," + self.inventory["tests/history.txt"][1] + ",inherited/inputs.json")
            return rejected(ctx.verify, "index inventory")

    def working_bytes_changed(self):
        with self.context() as ctx:
            (ctx.work / "inherited/inputs.json").write_text("changed\n")
            return rejected(ctx.verify, "working mode/blob")

    def working_mode_changed(self):
        with self.context() as ctx:
            (ctx.work / "tools/unexecuted").chmod(0o644)
            return rejected(ctx.verify, "working mode/blob")

    def same_bytes_untracked_rejected(self):
        with self.context() as ctx:
            extra = "inherited/ambient.json"
            (ctx.work / extra).write_bytes((ctx.work / "inherited/inputs.json").read_bytes())
            ctx.require_tracked(["inherited/inputs.json"])
            outcome = rejected(lambda: ctx.require_tracked([extra]), "untracked prospective input")
            require(extra not in ctx.inventory, "untracked path became authority")
            return outcome

    def copy_failure_cleanup(self):
        broken = self.root / "copy-with-bad-bytes"
        shutil.copytree(self.copy, broken)
        (broken / "inherited/inputs.json").write_text("same filename, different bytes\n")
        return rejected(lambda: self.context(copy=broken), "fixed copy mode/blob")

    def outward_input_symlinks_rejected(self):
        linked_root = self.root / "linked-copy"
        linked_root.symlink_to(self.copy, target_is_directory=True)
        rejected(lambda: self.context(copy=linked_root), "symlink directory")
        broken = self.root / "copy-with-outward-link"
        shutil.copytree(self.copy, broken)
        target = broken / "fixtures/space name.txt"
        target.unlink()
        target.symlink_to(self.sentinel)
        return rejected(lambda: self.context(copy=broken))

    def owned_cleanup_does_not_follow_symlink(self):
        with self.context() as ctx:
            (ctx.root / "outward-link").symlink_to(self.sentinel)
            saved = ctx.root
        require(not saved.exists(), "owned directory not removed")
        require(digest(self.sentinel.read_bytes()) == self.sentinel_hash, "cleanup followed outward symlink")
        return {"owned_root_removed": True, "outside_sentinel_unchanged": True}

    def cleanup_ownership_refusal(self):
        ctx = self.context()
        original = ctx.marker.read_bytes()
        try:
            ctx.marker.write_text('{"token":"injected-wrong-owner"}\n')
            outcome = rejected(ctx.close, "cleanup ownership mismatch; retained")
            require(ctx.root.is_dir(), "ownership mismatch deleted directory")
        finally:
            # Restore only this test's known owned marker, then finish cleanup.
            ctx.marker.write_bytes(original)
            ctx.close()
        return dict(outcome, restored_known_test_marker_and_cleaned=True)

    def independent_objects_survive_source_unavailable(self):
        provider = self.root / "short-lived-provider"
        shutil.copytree(self.source, provider)
        with self.context(source=provider) as ctx:
            gitdir = provider / ".git"
            away = self.root / "temporarily-unavailable-source-gitdir"
            gitdir.rename(away)
            try:
                require(ctx.verify() == 6, "context relies on source repository")
                result = self.git_call(ctx.work, "cat-file", "commit", self.spec["commit"])
                require(result == base64.b64decode(self.spec["objects"][self.spec["commit"]]["data_base64"]), "copied commit changed")
            finally:
                away.rename(gitdir)
        return {"source_gitdir_temporarily_unavailable": True, "context_still_valid": True}


    def bootstrap_io_failure_cleanup_or_retained_receipt(self):
        original_write = Path.write_text

        def fail_marker(path, data, *args, **kwargs):
            if path.name == "owner.json" and path.parent.parent == self.parent:
                original_write(path, '{"token":', *args, **kwargs)
                raise OSError("injected partial owner-marker write failure")
            return original_write(path, data, *args, **kwargs)

        with patch.object(Path, "write_text", fail_marker):
            marker_result = rejected(lambda: self.context(), "injected partial owner-marker")
        require(not list(self.parent.iterdir()), "bootstrap marker failure leaked owned context")
        original_lstat = Path.lstat
        captured = {}

        def fail_initial_identity(path, *args, **kwargs):
            info = original_lstat(path, *args, **kwargs)
            if path.parent == self.parent and path.name.startswith("t1166-"):
                captured.update(path=path, identity=(info.st_dev, info.st_ino))
                raise OSError("injected unavailable initial inode identity")
            return info

        with patch.object(Path, "lstat", fail_initial_identity):
            identity_result = rejected(lambda: self.context(), "cleanup ownership mismatch; retained")
        retained = captured["path"]
        require(str(retained) in identity_result["message"], "failure omitted retained-path receipt")
        require(retained.is_dir(), "unproven owner path was deleted")
        # Only the fault injector knows this exact newly allocated inode.
        info = original_lstat(retained)
        require((info.st_dev, info.st_ino) == captured["identity"], "test cleanup identity changed")
        shutil.rmtree(retained)
        return {"partial_marker_failure": marker_result,
                "unproven_identity_failure": identity_result,
                "retained_path_reported": True, "test_owned_cleanup_complete": True}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--suite-slot", required=True, type=int, choices=(1, 2, 3))
    args = parser.parse_args()
    output = args.output.absolute()
    output.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    spec = json.loads((HERE / "fixture.json").read_text())
    recorder = Recorder(output / "commands.jsonl")
    summary = {"classification": "SYNTHETIC_ONLY_NOT_ACTUAL_P", "suite_slot": args.suite_slot,
               "argv": sys.argv, "python": sys.version, "os": platform.platform(),
               "expected_cases": CASES, "actual_P_integration": "NOT_RUN_ACCESS_STOP",
               "actual_P_fetch_attempts": 0, "compiler_native_CMake_CTest_SDK_Actions_CI": 0,
               "input_sha256": {name: digest((HERE / name).read_bytes()) for name in
                                ("fixed_git_context.py", "run_suite.py", "fixture.json", "README.md")},
               "results": [], "first_failure": None}
    temporary = Path(tempfile.mkdtemp(prefix="suite-owned-", dir=HERE))
    try:
        suite = Suite(temporary, output, spec, recorder)
        summary["git"] = suite.git_call(suite.source, "--version").decode().strip()
        for name in CASES:
            case_started = time.monotonic()
            try:
                details = getattr(suite, name)()
                suite.source_unchanged()
                row = {"case": name, "status": "PASS", "details": details}
            except BaseException as error:
                row = {"case": name, "status": "FAIL", "error": type(error).__name__, "message": str(error)}
                traceback.print_exc()
                if summary["first_failure"] is None:
                    summary["first_failure"] = dict(row)
                # Preserve the failure and stop affected work; no in-suite retry.
                summary["results"].append(dict(row, elapsed_seconds=time.monotonic() - case_started))
                break
            summary["results"].append(dict(row, elapsed_seconds=time.monotonic() - case_started))
            print(name + " PASS", flush=True)
        after = filesystem_fingerprint(suite.source)
        (output / "source-after.json").write_text(json.dumps(after, indent=2, sort_keys=True) + "\n")
        summary["source_fingerprint_equal"] = after == suite.baseline
        summary["receipts"] = suite.receipts
    except BaseException as error:
        traceback.print_exc()
        if summary["first_failure"] is None:
            summary["first_failure"] = {"case": "setup_or_final_inventory", "status": "FAIL",
                                        "error": type(error).__name__, "message": str(error)}
    if summary["first_failure"] is None and len(summary["results"]) == len(CASES):
        shutil.rmtree(temporary)
        summary["suite_fixture_cleanup"] = "complete"
    else:
        summary["suite_fixture_cleanup"] = "retained_for_failure_review"
        summary["retained_fixture_path"] = str(temporary)
    summary["command_count"] = recorder.count
    summary["elapsed_seconds"] = time.monotonic() - started
    summary["status"] = "PASS" if len(summary["results"]) == len(CASES) and summary["first_failure"] is None else "FAIL"
    summary["commands_sha256"] = digest((output / "commands.jsonl").read_bytes())
    (output / "results.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    print(json.dumps({key: summary[key] for key in ("status", "command_count", "elapsed_seconds", "first_failure")}), flush=True)
    return 0 if summary["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
