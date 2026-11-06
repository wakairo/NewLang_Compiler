"""Rename only the two alternative test entry points in the unchanged P harness."""
from pathlib import Path
import sys

source = (Path(__file__).resolve().parents[1] / 'p1184' / 'safety.c').read_text()
old = 'int main(int argc, char **argv)'
assert source.count(old) == 2
Path(sys.argv[1]).write_text(source.replace(
    old, 'static int p1188_previous_safety_main(int argc, char **argv)'))
