"""The mutable successor cannot launder the externally fixed v61 predecessor."""
from pathlib import Path
import json
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/support'))
sys.path.insert(0, str(ROOT / 'tests/p1236'))
from versioned_freeze import FreezeError, digest
from history_v62 import verify

CURRENT = 'tests/p990/current-freeze-v62.json'
OLD = 'tests/p990/current-freeze-v61.json'
current = json.loads((ROOT / CURRENT).read_bytes())
with tempfile.TemporaryDirectory(prefix='c288-v61-coedit-') as directory:
    work = Path(directory)
    for name in set(current['files']) | {CURRENT}:
        p = work / name
        p.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, p)
    verify(work, CURRENT)
    p = work / OLD
    old = json.loads(p.read_bytes())
    old['files']['src/current_loop_check_internal.h'] = '0' * 64
    p.write_text(json.dumps(old, indent=2) + '\n')
    modified = json.loads((work / CURRENT).read_bytes())
    modified['previous']['sha256'] = digest(p.read_bytes())
    modified['files'][OLD] = digest(p.read_bytes())
    (work / CURRENT).write_text(json.dumps(modified, indent=2) + '\n')
    try:
        verify(work, CURRENT)
    except FreezeError as error:
        assert 'C288 fixed-main2931 v61 SHA/blob' in str(error), str(error)
        print('C288 v61/current simultaneous hash laundering REJECT', error)
    else:
        raise AssertionError('accepted rewritten original v61')
print('C288 original v61 fixed external SHA/OID and inherited coverage PASS')
