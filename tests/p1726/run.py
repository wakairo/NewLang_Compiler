from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory


support = runpy.run_path(str(Path(__file__).resolve().parents[1] / "p1722" / "run.py"))
one, observe, verify = support["one"], support["observe"], support["verify"]
admitted = {
    "early-0": (b"0", b"0,41,11,41,213\n"),
    "early-65472": (b"0", b"65472,97,11,97,213\n"),
    "early-second-field": (b"1", b"65472,97,11,97,213\n"),
}
refused = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("early-independent-literal", "early-duplicate-target-read")
}


def observe_selector(code, field):
    selector_binding = rb"^    const uint8_t (nl_local_\d+) = (nl_call_\d+);\n"
    binding = one(code, selector_binding)
    call = one(code, rb"^    const uint8_t " + binding[2] +
               rb" = (nl_fn_\d+)\((nl_local_\d+)\);\n")
    ctor = one(code, rb"^    nl_type_\d+ nl_local_\d+ = \{[^\n]*\};\n")
    arg = one(code, rb"^    const size_t " + call[2] + rb" = [^;]+;\n")
    one(code, rb"^static uint8_t " + call[1] + rb"\(size_t nl_local_\d+\)\n\{")
    assert ctor.end() < arg.start() < call.start() < binding.start()
    return observe(code, field, selector_binding)


if __name__ == "__main__":
    compiler, fixtures = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        verify(compiler, fixtures, destination, admitted, refused, observe_selector)
    else:
        with TemporaryDirectory(prefix="p1726-") as directory:
            verify(compiler, fixtures, Path(directory), admitted, refused, observe_selector)
