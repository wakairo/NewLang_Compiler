from contextlib import nullcontext
from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory


tests = Path(__file__).resolve().parents[1]
support = runpy.run_path(str(tests / "p1718" / "run.py"))
verify = support["verify"]
original = {
    "descriptor-1023": (b"1023,65472,97,11,97,213\n", b"0"),
    "descriptor-1024": (b"1024,0,41,11,41,213\n", b"0"),
}
admitted = {
    "early-reverse": (b"1023,65472,97,11,97,213\n", b"1"),
}
refused = (
    "early-wrong-selector-actual",
    "early-independent-literal",
    "early-duplicate-target-read",
    "middle-birth",
)


if __name__ == "__main__":
    compiler, fixtures = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1730-")
    with scope as directory:
        raw = Path(directory)
        verify(compiler, tests / "p1712" / "source", raw, original, (), early=True)
        verify(compiler, fixtures, raw, admitted, refused, early=True)
