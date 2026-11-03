"""Prospective C220 expectation for the preserved P1115/P1117 LOOP source.

The original predicate-only oracle correctly rejected this loop before C220.
Keep its bytes and all other checks; require the new genuine loop consumer and
native path for this one now-implemented source. No compiler result is changed.
"""
from pathlib import Path
import hashlib
import sys

root = Path(__file__).resolve().parents[2]
old = root / "tests/p1117/run_v2.py"
raw = old.read_bytes()
assert hashlib.sha256(raw).hexdigest() == (
    "a91805fcb088cc5e2e8453563015907fe2ea50506ae5bee3935463d910181a92"
)
oracle = root / "tests/p1117/oracle.json"
assert hashlib.sha256(oracle.read_bytes()).hexdigest() == (
    "8e3c9d206902a7a55d6195d0e083cb574d5e0d07663c347cf5809fb2ea65f325"
)
legacy = root / "tests/p1117/sources/07-p1115-loop-control.nl"
assert hashlib.sha256(legacy.read_bytes()).hexdigest() == (
    "7d71183a846f28e378974aa4c38013bb87caf86a24778d9f5872da1ced3841b7"
)
source = raw.decode()
before = "oracle['cases'].update(extension)"
assert source.count(before) == 1
after = before + """
if mode == 'candidate':
    previous_loop = oracle['cases']['07-p1115-loop-control']
    assert previous_loop['candidate'] == {'check': 3, 'emit': 3}
    previous_loop['candidate'] = {'check': 0, 'emit': 0}
    print('C220 prospective LOOP capability; original P1117 precision oracle retained', flush=True)
"""
exec(compile(source.replace(before, after, 1), str(old) + ":C220-current", "exec"),
     {"__file__": str(old), "__name__": "__main__"})
