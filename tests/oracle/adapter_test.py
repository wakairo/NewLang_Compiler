"""Regression checks for the V0 oracle adapter's fail-closed boundary."""
from pathlib import Path
import subprocess
from unittest.mock import patch

from harness import Outcome, run_production


def require_raises(source: Path, result: subprocess.CompletedProcess[str],
                   label: str) -> None:
    with patch("harness.subprocess.run", return_value=result):
        try:
            run_production("newlangc", source)
        except ValueError:
            return
    raise SystemExit(f"{label}: unexpected production failure was not fail-closed")


# Reviewed comparison exclusion is selected from fixture identity before any
# production process is run. Execution outcome cannot create this classification.
reviewed = Path("/frozen/tests/programs/01_qualified_make_ok.nl")
with patch("harness.subprocess.run",
           side_effect=AssertionError("excluded fixture must not execute production")):
    actual = run_production("newlangc", reviewed)
if actual != Outcome(False, None, "V0-OUTSIDE-REVIEWED-SPINE"):
    raise SystemExit("reviewed overlap: explicit comparison exclusion missing")

# Outside the explicit exclusion, preserve the exact pre-V0 unsupported contract.
legacy = Path("/tmp/legacy_non_overlap.nl")
legacy_stderr = (
    "error(cli)[P0-COMPILE-UNSUPPORTED]: "
    "source compilation is not implemented in P0\n"
    f"note: {legacy}\n"
)
with patch(
    "harness.subprocess.run",
    return_value=subprocess.CompletedProcess(
        ["newlangc", str(legacy)], 3, "", legacy_stderr
    ),
):
    actual = run_production("newlangc", legacy)
if actual != Outcome(False, None, "P0-COMPILE-UNSUPPORTED"):
    raise SystemExit("legacy non-overlap: strict unsupported contract changed")

# R1: unexpected nonzero status must not become unsupported.
require_raises(
    legacy,
    subprocess.CompletedProcess(["newlangc", str(legacy)], 4, "", legacy_stderr),
    "unexpected status",
)

# R2: signal-like process death must hard-fail.
require_raises(
    legacy,
    subprocess.CompletedProcess(["newlangc", str(legacy)], -11, "", ""),
    "signal-like failure",
)

# R3: exit 3 with an unexpected diagnostic must hard-fail.
require_raises(
    legacy,
    subprocess.CompletedProcess(
        ["newlangc", str(legacy)],
        3,
        "",
        "error(cli)[V0-CHECK]: unexpected diagnostic\n",
    ),
    "unexpected diagnostic",
)

# R4: empty stdout alone is never an unsupported classification.
require_raises(
    legacy,
    subprocess.CompletedProcess(["newlangc", str(legacy)], 3, "", ""),
    "empty stdout alone",
)

print("oracle.adapter: explicit exclusion + strict legacy + R1/R2/R3/R4 fail-closed")
