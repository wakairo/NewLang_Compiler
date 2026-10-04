"""Invoke actual M7.5 checker and CLI from the verified archive."""
import subprocess
import sys

from harness import (Outcome, compare_outcomes, frozen_oracle,
                     oracle_environment, run_oracle, run_production)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


with frozen_oracle() as oracle:
    fixtures = oracle / "tests/programs"
    cases = [
        ("01_qualified_make_ok.nl", True, None),
        ("06_private_field_outside_error.nl", False, "E_PRIVATE_FIELD"),
        ("13_semantic_aliased_actuals_conflict.nl", True, None),
        ("241_ref_locator_capture_ok.nl", True, None),
    ]
    for name, accepted, code in cases:
        actual = run_oracle(oracle, fixtures / name)
        require(actual == Outcome(True, accepted, code), f"oracle fixture failed: {name}")
    cli = subprocess.run([sys.executable, "-m", "newlang_frontend", "check",
                          str(fixtures / cases[0][0])], cwd=oracle,
                         env=oracle_environment(oracle), capture_output=True,
                         text=True, timeout=30, check=False)
    require(cli.returncode == 0 and cli.stdout.startswith("ACCEPT: ") and
            cli.stderr == "", "oracle CLI failed")
    production = run_production(sys.argv[1], fixtures / cases[0][0])
    try:
        compare_outcomes(production, Outcome(True, True, None))
    except ValueError:
        pass
    else:
        raise SystemExit("differential harness counted unsupported as agreement")
print("M7.5 oracle: 4 fixtures + CLI passed; production differential is explicitly unsupported")
