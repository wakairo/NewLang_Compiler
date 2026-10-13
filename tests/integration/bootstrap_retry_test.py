"""Deterministic offline bootstrap tests; urlopen is always mocked."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import ssl
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import urllib.error

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("locked_bootstrap", ROOT / "scripts/bootstrap.py")
bootstrap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bootstrap)
LOCKED = json.loads((ROOT / "scripts/toolchain-lock.json").read_text())
URL = next(p["url"] for p in LOCKED["platforms"]["debian-13-x86_64"]["packages"]
           if p["name"] == "libclang-common-23-dev")
DATA = b"offline verified package fixture"
DIGEST = hashlib.sha256(DATA).hexdigest()


def http_error(code=404):
    return urllib.error.HTTPError(URL, code, "offline status", None, None)


def versions(args, environment=None):
    if "llvm-config-23" in args[0]:
        return "23.1.2"
    if "clang-format-23" in args[0]:
        return "clang-format version 23.1.2"
    if "clang-23" in args[0]:
        return "clang version 23.1.2"
    return "offline tool version"


class BootstrapRetry(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.package = {"name": "libclang-common-23-dev", "version": "23.1.2",
                        "url": URL, "sha256": DIGEST}
        self.lock = self.root / "lock.json"
        self.lock.write_text(json.dumps({"llvm_version": "23.1.2", "platforms": {
            "debian-13-x86_64": {"packages": [self.package]}}}))
        identity = hashlib.sha256(self.lock.read_bytes()).hexdigest()[:16]
        self.prefix = self.root / ".deps/toolchains" / f"debian-13-x86_64-{identity}"
        self.cache = self.root / ".deps/downloads"
        self.path = self.cache / f"{self.package['name']}-{DIGEST}.deb"
        self.partial = self.path.with_suffix(".partial")
        for name, value in (("ROOT", self.root), ("LOCK", self.lock)):
            self.enterContext(patch.object(bootstrap, name, value))
        self.enterContext(patch.object(bootstrap.platform, "system", return_value="Linux"))
        self.enterContext(patch.object(bootstrap.platform, "machine", return_value="x86_64"))
        self.enterContext(patch.object(bootstrap.platform, "freedesktop_os_release",
                                      return_value={"ID": "debian", "VERSION_ID": "13"}))
        self.enterContext(patch.object(bootstrap.shutil, "which", return_value="offline-tool"))
        self.requests = self.enterContext(patch.object(bootstrap.urllib.request, "urlopen"))
        self.wait = self.enterContext(patch.object(bootstrap.time, "sleep"))
        self.extract = self.enterContext(patch.object(bootstrap.subprocess, "run"))
        self.version = self.enterContext(patch.object(bootstrap, "run", side_effect=versions))
        self.enterContext(contextlib.redirect_stdout(io.StringIO()))

    def failed_outputs(self):
        self.assertFalse(self.path.exists())
        self.assertFalse(self.partial.exists())
        self.assertFalse((self.prefix / ".complete").exists())
        self.assertFalse((self.root / ".deps/activate.sh").exists())
        self.extract.assert_not_called()

    def exact_requests(self, count):
        self.assertEqual(self.requests.call_count, count)
        for call in self.requests.call_args_list:
            self.assertEqual(call.args, (URL,))
            # No altered URL, SSL context, verification override or timeout.
            self.assertEqual(call.kwargs, {"timeout": 60})

    def test_transient_404_then_verified_bytes(self):
        self.requests.side_effect = [http_error(), io.BytesIO(DATA)]
        bootstrap.main()
        self.exact_requests(2)
        self.wait.assert_called_once_with(1)
        self.assertEqual(self.path.read_bytes(), DATA)
        self.assertFalse(self.partial.exists())
        self.assertTrue((self.prefix / ".complete").is_file())
        self.assertEqual(self.extract.call_count, 1)
        self.assertEqual(self.extract.call_args.args[0][0:2], ["dpkg-deb", "-x"])
        self.assertTrue(self.extract.call_args.kwargs["check"])

    def test_third_attempt_can_succeed(self):
        self.requests.side_effect = [http_error(), http_error(), io.BytesIO(DATA)]
        bootstrap.main()
        self.exact_requests(3)
        self.assertEqual([c.args for c in self.wait.call_args_list], [(1,), (2,)])
        self.assertEqual(self.path.read_bytes(), DATA)

    def test_permanent_404_exhausts_budget(self):
        self.requests.side_effect = http_error()
        with self.assertRaises(urllib.error.HTTPError) as caught:
            bootstrap.main()
        self.assertEqual(caught.exception.code, 404)
        self.exact_requests(3)
        self.assertEqual([c.args for c in self.wait.call_args_list], [(1,), (2,)])
        self.failed_outputs()

    def test_permanent_404_process_exits_nonzero(self):
        self.cache.mkdir(parents=True)
        code = """import importlib.util,json,sys,urllib.error
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('bootstrap',sys.argv[1])
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
p=json.loads(sys.argv[2])
with patch.object(b.urllib.request,'urlopen',side_effect=urllib.error.HTTPError(p['url'],404,'offline',None,None)) as get, patch.object(b.time,'sleep'):
    try: b.download(p,Path(sys.argv[3]))
    except urllib.error.HTTPError:
        print('requests='+str(get.call_count),flush=True)
        raise
"""
        process = subprocess.Popen([sys.executable, "-c", code,
                                    str(ROOT / "scripts/bootstrap.py"),
                                    json.dumps(self.package), str(self.path)],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate(timeout=10)
        self.assertNotEqual(process.returncode, 0)
        self.assertIn(b"requests=3", stdout)
        self.assertIn(b"HTTP Error 404", stderr)
        self.failed_outputs()

    def test_hash_mismatch_after_404_still_stops_immediately(self):
        self.requests.side_effect = [http_error(), io.BytesIO(b"wrong bytes")]
        with self.assertRaisesRegex(SystemExit, "checksum mismatch"):
            bootstrap.main()
        self.exact_requests(2)
        self.wait.assert_called_once_with(1)
        self.failed_outputs()

    def test_hash_mismatch_is_terminal_without_install(self):
        self.requests.return_value = io.BytesIO(b"wrong bytes")
        with self.assertRaisesRegex(SystemExit, "checksum mismatch"):
            bootstrap.main()
        self.exact_requests(1)
        self.wait.assert_not_called()
        self.failed_outputs()

    def test_verified_cache_reused_and_versions_rechecked(self):
        self.cache.mkdir(parents=True)
        self.path.write_bytes(DATA)
        self.requests.side_effect = AssertionError("offline test must not request verified cache")
        bootstrap.main()
        bootstrap.main()
        self.requests.assert_not_called()
        self.wait.assert_not_called()
        self.assertEqual(self.extract.call_count, 1)  # complete prefix on second run
        self.assertEqual(sum("llvm-config-23" in c.args[0][0]
                             for c in self.version.call_args_list), 2)

    def test_bad_cache_is_not_extracted(self):
        self.cache.mkdir(parents=True)
        self.path.write_bytes(b"corrupt cache")
        self.requests.side_effect = http_error()
        with self.assertRaises(urllib.error.HTTPError):
            bootstrap.main()
        self.assertEqual(self.path.read_bytes(), b"corrupt cache")
        self.extract.assert_not_called()
        self.assertFalse(self.partial.exists())
        self.assertFalse((self.prefix / ".complete").exists())
        self.exact_requests(3)

    def test_partial_read_failure_cleans_untrusted_file(self):
        class Broken(io.BytesIO):
            def read(self, count=-1):
                if self.tell():
                    raise OSError("offline interrupted stream")
                return super().read(5)
        self.requests.return_value = Broken(DATA)
        with self.assertRaisesRegex(OSError, "interrupted stream"):
            bootstrap.main()
        self.exact_requests(1)
        self.wait.assert_not_called()
        self.failed_outputs()

    def test_stale_partial_never_becomes_verified_cache(self):
        self.cache.mkdir(parents=True)
        self.partial.write_bytes(DATA)  # even a correctly hashed partial is not a cache
        self.requests.side_effect = http_error()
        with self.assertRaises(urllib.error.HTTPError):
            bootstrap.main()
        self.exact_requests(3)
        self.failed_outputs()

    def test_tls_error_is_not_retried(self):
        self.requests.side_effect = urllib.error.URLError(ssl.SSLCertVerificationError("offline TLS"))
        with self.assertRaises(urllib.error.URLError):
            bootstrap.main()
        self.exact_requests(1)
        self.wait.assert_not_called()
        self.failed_outputs()

    def test_other_http_error_is_not_retried(self):
        self.requests.side_effect = http_error(403)
        with self.assertRaises(urllib.error.HTTPError):
            bootstrap.main()
        self.exact_requests(1)
        self.wait.assert_not_called()
        self.failed_outputs()

    def test_extraction_error_does_not_write_complete(self):
        self.requests.return_value = io.BytesIO(DATA)
        self.extract.side_effect = subprocess.CalledProcessError(1, ["dpkg-deb"])
        with self.assertRaises(subprocess.CalledProcessError):
            bootstrap.main()
        self.assertEqual(self.path.read_bytes(), DATA)  # verified cache retained
        self.assertFalse(self.partial.exists())
        self.assertFalse((self.prefix / ".complete").exists())
        self.version.assert_not_called()

    def test_wrong_llvm_version_does_not_write_complete(self):
        self.requests.return_value = io.BytesIO(DATA)
        self.version.return_value = "wrong-version"
        self.version.side_effect = None
        with self.assertRaisesRegex(SystemExit, "LLVM version mismatch"):
            bootstrap.main()
        self.assertFalse((self.prefix / ".complete").exists())
        self.assertFalse((self.root / ".deps/activate.sh").exists())

    def test_malformed_lock_is_terminal_without_network(self):
        self.lock.write_text("{invalid JSON")
        with self.assertRaises(json.JSONDecodeError):
            bootstrap.main()
        self.requests.assert_not_called()
        self.failed_outputs()

    def test_old_complete_marker_does_not_authorize_bad_download(self):
        self.prefix.mkdir(parents=True)
        (self.prefix / ".complete").write_text("old marker")
        self.requests.side_effect = http_error()
        with self.assertRaises(urllib.error.HTTPError):
            bootstrap.main()
        self.exact_requests(3)
        self.extract.assert_not_called()
        self.version.assert_not_called()
        self.assertFalse((self.root / ".deps/activate.sh").exists())
        self.assertFalse(self.partial.exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
