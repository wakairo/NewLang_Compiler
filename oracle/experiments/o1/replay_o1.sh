#!/usr/bin/env bash
# O2 evidence-only replay of the UNMODIFIED O1 Python experiment; no network, no C.
set -euo pipefail
HERE="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PYTHON="${PYTHON:-python3}"
TMP="$(mktemp -d)"
trap 'rm -rf -- "$TMP"' EXIT
"$PYTHON" -m venv --without-pip "$TMP/venv"
VENV_PY="$TMP/venv/bin/python"
"$VENV_PY" -c 'import sys; assert sys.version_info >= (3, 12), sys.version'
export PYTHONDONTWRITEBYTECODE=1 PYTHONHASHSEED=771
cd "$HERE"
"$VENV_PY" -m unittest discover -p 'test_*.py' -q
"$VENV_PY" run_experiment.py > "$TMP/o1-results.json"
"$VENV_PY" - "$TMP/o1-results.json" <<'PY'
import hashlib, json, pathlib, sys
path = pathlib.Path(sys.argv[1])
actual = hashlib.sha256(path.read_bytes()).hexdigest()
expected = "cee5353583b856127537ec623795d4570c995b4699433923e221a56579af5b02"
assert actual == expected, ("O1 JSON replay diverged", actual, expected)
summary = json.loads(path.read_text())["summary"]
assert summary == {
    "both_accept": 266, "case_count": 550, "critical_false_accept": 0,
    "precision_cases": 4, "semantic_reject_cases": 280,
    "unsupported_pairs_not_counted_as_agreement": 2,
}, summary
print("O2 REPLAY VERIFIED", actual, summary)
PY
