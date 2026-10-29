"""T1039 isolated synthetic regressions; no R program, oracle or GitHub Actions."""
import tempfile
import unittest
from pathlib import Path

from artifact_custody_preflight import PreflightError, render, validate_files, validate_names

SHA = "a" * 40
V1 = "r-calibrated/r1016-pr1031"  # historical v1 actual ref; not an invented -v1 run
V2 = "r-calibrated/r1016-pr1031-v2"
BAD_V1 = "r1032-CALIBRATED-{run_id}-attempt{attempt}-{ref}-{sha}"
GOOD = "r1032-CALIBRATED-{run_id}-attempt{attempt}-{ref_slug}-{sha}"
FIXED_V2 = "r1032-CALIBRATED-{run_id}-attempt{attempt}-r1016-pr1031-v2-{sha}"


def record(ref=V2, run_id="40874742688", attempt=1, sha=SHA):
    return dict(ref=ref, run_id=run_id, attempt=attempt, sha=sha)


class NameTests(unittest.TestCase):
    def test_initial_v1_red_preserved(self):
        with self.assertRaisesRegex(PreflightError, "unsafe artifact name"):
            render(BAD_V1, **record(ref=V1))

    def test_simulated_two_refs_safe_and_distinct(self):
        x, y = validate_names(GOOD, [record(ref=V1), record(ref=V2)])
        self.assertNotEqual(x, y)
        self.assertNotIn("/", x + y)

    def test_exact_existing_v2_literal_is_valid(self):
        name = render(FIXED_V2, **record())
        self.assertIn("r1016-pr1031-v2", name)
        self.assertNotIn("/", name)

    def test_attempt_and_run_and_sha_custody_is_unique(self):
        base = record()
        records = [base, record(attempt=2), record(run_id="40874742689"), record(sha="b" * 40)]
        self.assertEqual(len(set(validate_names(GOOD, records))), 4)

    def test_duplicate_rejected(self):
        with self.assertRaisesRegex(PreflightError, "duplicate"):
            validate_names(GOOD, [record(), record()])

    def test_missing_unique_field_rejected(self):
        for t in ("fixed-{run_id}-{sha}", "fixed-{attempt}-{sha}", "fixed-{run_id}-{attempt}"):
            with self.subTest(t=t), self.assertRaises(PreflightError):
                render(t, **record())

    def test_bad_platform_characters_rejected(self):
        for c in ('/', '\\', ':', '<', '>', '|', '*', '?', '"', '\n', '\r', ' '):
            with self.subTest(char=repr(c)), self.assertRaises(PreflightError):
                render("R" + c + "-{run_id}-attempt{attempt}-{sha}", **record())

    def test_overlong_empty_and_windows_reserved_rejected(self):
        for t in ("x" * 161 + "-{run_id}-{attempt}-{sha}", "CON.{run_id}-{attempt}-{sha}",
                  "a..b-{run_id}-{attempt}-{sha}", ".bad-{run_id}-{attempt}-{sha}"):
            with self.subTest(t=t[:15]), self.assertRaises(PreflightError):
                render(t, **record())

    def test_unsafe_format_expressions_rejected(self):
        for t in ("{ref.__class__}-{run_id}-{attempt}-{sha}",
                  "{ref[0]}-{run_id}-{attempt}-{sha}",
                  "{run_id:>30}-{attempt}-{sha}",
                  "{unknown}-{run_id}-{attempt}-{sha}"):
            with self.subTest(t=t), self.assertRaises(PreflightError):
                render(t, **record())

    def test_ref_and_metadata_drift_rejected(self):
        for kv in (dict(ref="r-calibrated/other"), dict(run_id="abc"),
                   dict(attempt=0), dict(sha="deadbeef")):
            with self.subTest(kv=kv), self.assertRaises(PreflightError):
                render(GOOD, **(record() | kv))


class RawTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="t1039-synthetic-")
        self.root = Path(self.tmp.name)
        (self.root / "journal.jsonl").write_text('{"case":"fake"}\n')
        (self.root / "status.txt").write_text("SYNTHETIC\n")

    def tearDown(self):
        self.tmp.cleanup()

    def check(self, expected=2):
        return validate_files(self.root, required=["journal.jsonl", "status.txt"], expected_count=expected)

    def test_two_nonempty_files_hashed(self):
        digests = self.check()
        self.assertEqual(len(digests), 2)
        self.assertEqual(len(digests["journal.jsonl"]), 64)

    def test_missing_file_red(self):
        (self.root / "journal.jsonl").unlink()
        with self.assertRaises(PreflightError):
            self.check()

    def test_empty_file_red(self):
        (self.root / "journal.jsonl").write_bytes(b"")
        with self.assertRaises(PreflightError):
            self.check()

    def test_synthetic_43_raw_files(self):
        for i in range(41):
            (self.root / f"synthetic{i:02d}.txt").write_text(f"fake-{i}\n")
        self.assertEqual(len(self.check(expected=43)), 43)

    def test_nonportable_raw_filename_red(self):
        (self.root / "unsafe:name.txt").write_text("fake")
        with self.assertRaises(PreflightError):
            self.check(expected=3)

    def test_duplicate_required_receipt_red(self):
        with self.assertRaises(PreflightError):
            validate_files(self.root, required=["journal.jsonl", "journal.jsonl"], expected_count=2)

    def test_unexpected_count_red(self):
        with self.assertRaises(PreflightError):
            self.check(expected=3)

    def test_symlink_red(self):
        (self.root / "status.txt").unlink()
        (self.root / "status.txt").symlink_to(self.root / "journal.jsonl")
        with self.assertRaises(PreflightError):
            self.check()

    def test_required_receipt_red(self):
        with self.assertRaises(PreflightError):
            validate_files(self.root, required=["missing.jsonl"], expected_count=2)


if __name__ == "__main__":
    unittest.main()
