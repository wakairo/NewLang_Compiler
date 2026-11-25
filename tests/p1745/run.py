from contextlib import nullcontext
from functools import partial
from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory


tests = Path(__file__).resolve().parents[1]
support = runpy.run_path(str(tests / "p1738" / "run.py"))
verify, observe = support["verify"], support["observe"]
admitted = {
    "inline-policy-small": (b"1", b"4032,1,9,1,37\n"),
    "inline-policy-oversize": (b"1", b"0,1,9,1,37\n"),
    "variant-boundary": (b"0", b"2048,7,17,7,51\n"),
    "variant-overflow-fallback": (b"0", b"0,2,17,2,51\n"),
}
named = {
    "named-policy-bulk": (b"1", b"4096,4,9,4,37\n"),
    "named-variant-boundary": (b"0", b"2048,7,17,7,51\n"),
    "named-variant-overflow-fallback": (b"0", b"0,2,17,2,51\n"),
    "named-policy-edit": (b"0", b"2032,7,17,7,51\n"),
}
refused = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("named-independent-input", "named-nested-policy",
                 "forwarded-writer-bulk",
                 "constant-writer-bulk", "returns-new-mode",
                 "second-policy-change", "unrelated-budget")
}
refused["scope-exit"] = (3, 3, b"P8-EXIT-DEPENDENCY")


if __name__ == "__main__":
    cli, sources = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(sys.argv) == 4:
        raw = Path(sys.argv[3]).resolve()
        raw.mkdir()
        scope = nullcontext(raw)
    else:
        scope = TemporaryDirectory(prefix="p1745-")
    with scope as directory:
        verify(cli, sources, Path(directory), admitted, refused,
               partial(observe, local_policy=True))
        verify(cli, sources, Path(directory), named, {},
               partial(observe, local_policy=True, named_policy=True))
