"""Frozen Class O adapter; never interprets unsupported production as agreement.

Future cases can use normalized Outcome objects and compare_outcomes only after
the production side implements the relevant normative rule. Oracle diagnostics
are evidence, not normative expectations for C diagnostic wording.
"""
from contextlib import contextmanager
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = ROOT / "oracle/NewLang_FrontEnd_Prototype_M7_5.zip"
IDENTITY = json.loads((ROOT / "oracle/identity.json").read_text())


@dataclass(frozen=True)
class Outcome:
    supported: bool
    accepted: bool | None
    diagnostic_code: str | None


@contextmanager
def frozen_oracle():
    if hashlib.sha256(ARCHIVE.read_bytes()).hexdigest() != IDENTITY["sha256"]:
        raise ValueError("oracle archive hash mismatch")
    with tempfile.TemporaryDirectory(prefix="newlang-oracle-") as temporary:
        destination = Path(temporary).resolve()
        with zipfile.ZipFile(ARCHIVE) as archive:
            for entry in archive.infolist():
                path = (destination / entry.filename).resolve()
                if not path.is_relative_to(destination) or (
                    entry.external_attr >> 16
                ) & 0o170000 == 0o120000:
                    raise ValueError("unsafe oracle archive entry")
            archive.extractall(destination)
        yield destination / IDENTITY["root"]


def oracle_environment(oracle_root: Path) -> dict[str, str]:
    environment = os.environ.copy()
    environment.update(PYTHONPATH=os.pathsep.join(
        [str(oracle_root), str(oracle_root / "vendor")]),
        PYTHONDONTWRITEBYTECODE="1", PYTHONHASHSEED="0")
    return environment


def run_oracle(oracle_root: Path, source: Path) -> Outcome:
    adapter = """
import json, sys
from pathlib import Path
from newlang_frontend.driver import check_source
p = Path(sys.argv[1])
r = check_source(p.read_text(encoding='utf-8'), str(p))
print(json.dumps({'accepted': r.accepted,
                  'diagnostic_code': r.diagnostic.code if r.diagnostic else None},
                 sort_keys=True))
"""
    result = subprocess.run([sys.executable, "-c", adapter, str(source)],
                            cwd=oracle_root, env=oracle_environment(oracle_root),
                            capture_output=True, text=True, timeout=30, check=True)
    data = json.loads(result.stdout)
    return Outcome(True, data["accepted"], data["diagnostic_code"])


def run_production(binary: str, source: Path) -> Outcome:
    result = subprocess.run([binary, str(source)], capture_output=True,
                            text=True, timeout=20, check=False)
    if result.returncode != 0 and result.stdout == "":
        return Outcome(False, None, "V0-OUTSIDE-REVIEWED-SPINE")
    if result.returncode == 0 and result.stderr == "":
        return Outcome(True, True, None)
    raise ValueError("unexpected V0 production result; define a reviewed adapter")


def compare_outcomes(production: Outcome, oracle: Outcome) -> bool:
    if not production.supported or not oracle.supported:
        raise ValueError("unsupported result cannot participate in a differential comparison")
    return (production.accepted, production.diagnostic_code) == (
        oracle.accepted, oracle.diagnostic_code)
