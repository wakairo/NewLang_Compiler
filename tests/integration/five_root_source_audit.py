"""Issue #237 observation tool, NOT an acceptance test or a READY certificate.

Run against a supplied production CLI. Keep the full canonical primary intact;
reduced controls isolate current implementation limits, never replace it.
"""
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile

compiler = pathlib.Path(sys.argv[1]).resolve()
repo = pathlib.Path(__file__).resolve().parents[2]
draft = (repo / "docs/reference/NewLang_v0_spec_Draft17_30.md").read_text()
canonical = draft.split("### 3.2b.3 Exact five-site full source-shaped positive witness\n", 1)[1].split("~~~newlang\n", 1)[1].split("~~~", 1)[0]
primary = "".join(line for line in canonical.splitlines(keepends=True)
                  if not line.lstrip().startswith("//"))
assert primary == (repo / "tests/fixtures/five_root_three_field.nl").read_text()
assert primary.count("try_allocate_one<Node>()") == 5
assert primary.count("replace(") == 6
cases = [
    ("canonical", primary),
    ("alpha", primary.replace("Node", "Cell").replace("ptr_", "pointer_").replace("life_", "domain_")),
    ("payload", primary.replace("u8(1)", "u8(11)").replace("u8(2)", "u8(22)")),
    ("three-no-links", (repo / "tests/fixtures/three_allocation_no_links.nl").read_text()),
    ("two-no-links", (repo / "tests/fixtures/two_allocation_no_links.nl").read_text()),
]
observations = []
with tempfile.TemporaryDirectory(prefix="five-root-audit-") as directory:
    root = pathlib.Path(directory)
    for name, source in cases:
        filename = name + ".nl"
        (root / filename).write_text(source)
        before = set(root.iterdir())
        a = subprocess.run([str(compiler), filename], cwd=root, capture_output=True)
        b = subprocess.run([str(compiler), filename], cwd=root, capture_output=True)
        assert (a.returncode, a.stdout, a.stderr) == (b.returncode, b.stdout, b.stderr)
        assert set(root.iterdir()) == before, "CLI unexpectedly wrote an artifact"
        observations.append({"input": name, "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
                             "exit": a.returncode, "stdout": a.stdout.decode(),
                             "stderr": a.stderr.decode(), "repeat_identical": True,
                             "output_files_created": []})
print(json.dumps({"kind": "observations-only-not-ready-proof",
                  "canonical_sha256_including_terminal_lf": hashlib.sha256(canonical.encode()).hexdigest(),
                  "canonical_characters_excluding_terminal_lf": len(canonical.rstrip("\n")),
                  "fixture_sha256": hashlib.sha256(primary.encode()).hexdigest(),
                  "observations": observations}, indent=2))
