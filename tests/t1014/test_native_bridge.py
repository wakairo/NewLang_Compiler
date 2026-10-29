"""Isolated T-only synthetic negative suite. No R, no P CLI, no CMake/native."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

BASE = Path(__file__).resolve().parents[2]
BRIDGE = BASE / "cmake/independent-r-native-bridge.py"
FIX = Path(__file__).parent / "fixtures"
spec = importlib.util.spec_from_file_location("t1014_bridge_preview", BRIDGE)
bridge = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = bridge
spec.loader.exec_module(bridge)


def digest(data):
    return hashlib.sha256(data).hexdigest()


class TestSyntheticNativeBridge(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="t1014-synthetic-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "tests/r").mkdir(parents=True)
        self.source_dir = self.root / "tests/r113/sources"
        self.native_dir = self.root / "tests/r113/native"
        self.source_dir.mkdir(parents=True)
        self.native_dir.mkdir(parents=True)
        for local, target in (("runner.py", self.native_dir / "runner.py"),
                              ("observer.py", self.native_dir / "observer.py"),
                              ("oracle.json", self.native_dir / "oracle.json"),
                              ("positive.nl", self.source_dir / "positive.nl"),
                              ("negative.nl", self.source_dir / "negative.nl")):
            shutil.copyfile(FIX / local, target)
        self.manifest = self.root / bridge.MANIFEST_PATH
        self.populate()

    def populate(self, cases=None):
        if cases is None:
            cases = [("r113_alpha", "positive", "positive.nl"),
                     ("r113_beta", "negative", "negative.nl")]
        self.cases = cases
        rows = [bridge.HEADER, "cohort|r113"]
        for kind, filename in (("runner", "runner.py"), ("observer", "observer.py"),
                               ("oracle", "oracle.json")):
            rel = f"tests/r113/native/{filename}"
            rows.append(f"{kind}|{rel}|{digest((self.root / rel).read_bytes())}")
        for name, mode, filename in cases:
            rel = f"tests/r113/sources/{filename}"
            data = (self.root / rel).read_bytes() if (self.root / rel).is_file() else b"empty"
            rows.append(f"case|{name}|{mode}|{rel}|{digest(data)}")
        self.manifest.write_text("\n".join(rows) + "\n", encoding="ascii")
        self.grant()

    def grant(self):
        self.approval = bridge.Approval(
            cohort="r113", manifest_sha256=digest(self.manifest.read_bytes()),
            case_ids=tuple(c[0] for c in self.cases),
            head="1" * 40, parent="2" * 40)
        self.approvals = {"r113": self.approval}

    def parse(self):
        return bridge.parse(self.root, self.approvals)

    def assertStop(self, fn, keyword=None):
        with self.assertRaises(bridge.BridgeStop) as ctx:
            fn()
        if keyword:
            self.assertIn(keyword, str(ctx.exception))

    def tweak_line(self, old, new, reapprove=False):
        s = self.manifest.read_text()
        self.assertIn(old, s)
        self.manifest.write_text(s.replace(old, new), encoding="ascii")
        if reapprove:
            self.grant()

    def test_01_no_manifest_means_zero(self):
        self.manifest.unlink()
        out = self.root / "generated.cmake"
        self.assertEqual(bridge.configure(self.root, out), 0)
        self.assertIn("0 CTests", out.read_text())

    def test_02_no_approval_blocks(self):
        self.assertEqual(bridge.APPROVED, {})
        self.assertStop(lambda: bridge.parse(self.root), "EXTERNAL C approval")
        self.assertStop(lambda: bridge.run(self.root, "UNUSED"), "EXTERNAL C approval")

    def test_03_valid_external_synthetic_approval_and_no_R_execution(self):
        out = self.root / "generated.cmake"
        self.assertEqual(bridge.configure(self.root, out, self.approvals), 1)
        text = out.read_text()
        self.assertIn("r113_native_suite.integration", text)
        self.assertIn("$<TARGET_FILE:newlangc>", text)
        self.assertNotIn("tests/r113/native/runner.py", text)
        result = bridge.run(self.root, "NOT_A_REAL_COMPILER", self.approvals)
        self.assertIn("PHYSICAL_UNAUTHENTICATED", result)

    def test_04_manifest_self_updated_SHA_not_external_approval(self):
        self.tweak_line("r113_alpha", "r113_delta")
        self.assertStop(self.parse, "external C approval")

    def test_05_wrong_header(self):
        self.tweak_line(bridge.HEADER, "NL_R_COHORT_V1", True)
        self.assertStop(self.parse, "header")

    def test_06_duplicate_name(self):
        self.cases = [("r113_alpha", "positive", "positive.nl"),
                      ("r113_alpha", "negative", "negative.nl")]
        self.populate(self.cases)
        self.assertStop(self.parse, "duplicate")

    def test_07_mixed_cohort(self):
        self.tweak_line("r113_beta", "r114_beta", True)
        self.assertStop(self.parse, "bad case")

    def test_08_path_traversal_and_absolute(self):
        for replacement in ("../positive.nl", "/etc/passwd"):
            with self.subTest(replacement=replacement):
                old = self.manifest.read_text()
                self.tweak_line("tests/r113/sources/positive.nl", replacement, True)
                self.assertStop(self.parse)
                self.manifest.write_text(old)
                self.grant()

    def test_09_symlink_and_hardlink_source(self):
        target = self.source_dir / "positive.nl"
        data = target.read_bytes()
        target.unlink()
        target.symlink_to(self.source_dir / "negative.nl")
        self.assertStop(self.parse, "symlink")
        target.unlink()
        target.write_bytes(data)
        (self.root / "alias.nl").hardlink_to(target)
        self.assertStop(self.parse, "hard-linked")

    def test_10_non_regular_or_missing_source(self):
        (self.source_dir / "positive.nl").unlink()
        self.assertStop(self.parse, "missing")
        (self.source_dir / "positive.nl").mkdir()
        self.assertStop(self.parse, "nonregular")

    def test_11_source_runner_observer_oracle_sha_drift(self):
        for file in (self.source_dir / "positive.nl", self.native_dir / "runner.py",
                     self.native_dir / "observer.py", self.native_dir / "oracle.json"):
            with self.subTest(file=file.name):
                old = file.read_bytes()
                file.write_bytes(old + b"changed")
                self.assertStop(self.parse, "SHA mismatch")
                file.write_bytes(old)

    def test_12_bad_mode(self):
        self.tweak_line("|positive|", "|native_untrusted|", True)
        self.assertStop(self.parse, "bad case")

    def test_13_CRLF_and_oversized_manifest(self):
        original = self.manifest.read_bytes()
        self.manifest.write_bytes(original.replace(b"\n", b"\r\n"))
        self.grant()
        self.assertStop(self.parse, "LF-only")
        self.manifest.write_bytes(original + b"a" * 9000)
        self.grant()
        self.assertStop(self.parse, "oversized")

    def test_14_untrusted_manifest_command_not_evaluated(self):
        self.manifest.write_text(self.manifest.read_text() + 'execute_process(COMMAND evil)\n')
        self.grant()
        self.assertStop(self.parse, "bad case")

    def test_15_missing_receipt_even_child_exit0(self):
        runner = self.native_dir / "runner.py"
        runner.write_text("import sys\nsys.exit(0)\n")
        self.populate()
        self.assertStop(lambda: bridge.run(self.root, "UNUSED", self.approvals), "missing")

    def test_16_duplicate_JSON_keys_and_missing_case(self):
        _, ap, _f, rows, mh = self.parse()
        self.assertStop(lambda: bridge.validate_receipt(b'{"schema":1,"schema":2}', "r113", ap, rows, mh), "duplicate")
        data = self.valid_receipt()
        data["cases"].pop()
        self.assertStop(lambda: bridge.validate_receipt(json.dumps(data).encode(), "r113", ap, rows, mh), "missing")

    def valid_receipt(self):
        # T-owned mock runner is deliberately a CLAIM, not independent guest evidence.
        with tempfile.TemporaryDirectory() as t:
            temp = Path(t)
            target = temp / "r.json"
            subprocess.run([sys.executable, str(FIX / "runner.py"), "--native-suite",
                            "FAKE", str(self.manifest), str(self.native_dir / "observer.py"),
                            str(self.native_dir / "oracle.json"), str(target)],
                           capture_output=True, check=True, timeout=5)
            return json.loads(target.read_text())

    def invalid_receipt(self, edit, message=None):
        cohort, ap, _files, rows, mh = self.parse()
        report = self.valid_receipt()
        edit(report)
        self.assertStop(lambda: bridge.validate_receipt(json.dumps(report).encode(),
                                                        cohort, ap, rows, mh), message)

    def test_17_missing_guest2_not_a_pass(self):
        self.invalid_receipt(lambda r: r["cases"][0]["stages"].pop("guest2"), "incomplete")

    def test_18_guest0_not_reached_and_wrong_alloc(self):
        self.invalid_receipt(lambda r: r["cases"][0]["stages"]["guest0"].update(reached=False), "guest")
        self.invalid_receipt(lambda r: r["cases"][0]["stages"]["guest2"].update(allocation_successes=0), "guest")

    def test_19_missing_clang_and_modified_C17(self):
        self.invalid_receipt(lambda r: r["cases"][0]["stages"].pop("clang_o2"), "incomplete")
        self.invalid_receipt(lambda r: r["cases"][0]["stages"]["emitted_c17_original"].update(unchanged=False), "C17")

    def test_20_negative_generic_parser_rejection_is_not_semantic(self):
        self.invalid_receipt(lambda r: r["cases"][1]["stages"]["check_reject"].update(reason="parser"), "generic parser")
        self.invalid_receipt(lambda r: r["cases"][1]["stages"]["check_reject"].update(safety_reject=False), "generic parser")

    def test_21_negative_no_clobber_and_fake_observed_false(self):
        self.invalid_receipt(lambda r: r["cases"][1]["stages"]["no_clobber"].update(unchanged=False), "no-clobber")
        self.invalid_receipt(lambda r: r["cases"][0]["stages"]["owned_current"].update(observed=False), "unobserved")

    def test_22_receipt_case_order_and_approval_head_drift(self):
        self.invalid_receipt(lambda r: r["cases"].reverse(), "reordered")
        self.invalid_receipt(lambda r: r.update(head="f" * 40), "identity")

    def test_23_runner_mutates_source_after_its_own_exit(self):
        runner = self.native_dir / "runner.py"
        source = runner.read_text()
        runner.write_text(source + '\n(Path(manifest).resolve().parents[2] / "tests/r113/sources/positive.nl").write_bytes(b"mutated")\n')
        self.populate()
        self.assertStop(lambda: bridge.run(self.root, "FAKE", self.approvals), "SHA mismatch")

    def test_24_forced_child_failure(self):
        runner = self.native_dir / "runner.py"
        runner.write_text("import sys\nsys.exit(9)\n")
        self.populate()
        self.assertStop(lambda: bridge.run(self.root, "UNUSED", self.approvals), "child failed")

    def test_25_forced_child_timeout(self):
        runner = self.native_dir / "runner.py"
        runner.write_text("import time\ntime.sleep(1)\n")
        self.populate()
        prev = bridge.CHILD_TIMEOUT
        bridge.CHILD_TIMEOUT = 0.02
        try:
            self.assertStop(lambda: bridge.run(self.root, "UNUSED", self.approvals), "timeout")
        finally:
            bridge.CHILD_TIMEOUT = prev


if __name__ == "__main__":
    unittest.main(verbosity=2)
