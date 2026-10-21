"""Check full exit/stdout/stderr contracts, twice to check determinism."""
import subprocess
import sys

HELP = (
    "Usage: newlangc [--version | --help | SOURCE]\n"
    "       newlangc --check SOURCE\n"
    "       newlangc --emit-c17-to OUTPUT SOURCE\n"
    "       newlangc --experimental-c17-u8-export FUNCTION SOURCE\n"
    "NewLang production compiler P0 bootstrap.\n"
    "  --version  Print the deterministic compiler version.\n"
    "  --help     Print this help.\n"
    "  --check SOURCE  Validate source without backend emission.\n"
    "  SOURCE          Emit supported checked-profile C17 to stdout.\n"
    "  --emit-c17-to OUTPUT SOURCE\n"
    "             Stage and replace a named C17 file (POSIX local host).\n"
    "  --experimental-c17-u8-export FUNCTION SOURCE\n"
    "             Experimental Copy-only u8 export with a fixed C17 header;\n"
    "             not general FFI.\n"
    "Only supported checked profiles emit C17; --check success does not "
    "guarantee\n"
    "backend emission. LLVM remains the planned primary backend.\n"
)
CASES = {
    "version": (["--version"], 0, "newlangc 0.1.0 (P0 bootstrap)\n", ""),
    "help": (["--help"], 0, HELP, ""),
    "no_arguments": ([], 0, HELP, ""),
    "invalid_option": (["--bogus"], 2, "",
                       "error(cli)[P0-CLI-OPTION]: unknown option\nnote: --bogus\n"),
    "extra_argument": (["--version", "extra"], 2, "",
                       "error(cli)[P0-CLI-ARGUMENT]: expected exactly one argument\n"
                       "note: use --help for usage\n"),
    "unsupported": (["sample.nl"], 3, "",
                    "error(cli)[P0-COMPILE-UNSUPPORTED]: "
                    "source compilation is not implemented in P0\nnote: sample.nl\n"),
}

if __name__ == "__main__":
    binary, case = sys.argv[1:]
    args, status, stdout, stderr = CASES[case]
    for _ in range(2):
        result = subprocess.run([binary, *args], capture_output=True, text=True,
                                timeout=20, check=False)
        actual = (result.returncode, result.stdout, result.stderr)
        expected = (status, stdout, stderr)
        if actual != expected:
            raise SystemExit(f"{case}: expected {expected!r}, got {actual!r}")
    print(f"cli.{case}: deterministic exit/output contract passed")
