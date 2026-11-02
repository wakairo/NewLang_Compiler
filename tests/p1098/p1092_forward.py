"""Prospective C193: preserve old P1092 runner/oracle, change only five C17 predictions."""
from pathlib import Path
import hashlib
import sys

root = Path(sys.argv[2])
old = root / 'tests/p1092/cli.py'
# Original main1de byte identity is independently pinned in evidence_test.py.
text = old.read_text()
needle = 'expected = 0 if number == 0 else 4 if number in positive else 3'
assert text.count(needle) == 1
text = text.replace(needle, 'expected = 0 if number in {0, 2, 3, 6, 16, 17} else 4 if number in positive else 3')
exec(compile(text, str(old) + ':C193-prospective', 'exec'), {'__name__': '__main__'})
