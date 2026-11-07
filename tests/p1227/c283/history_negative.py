"""Current successor may not launder the independently pinned original v60."""
from pathlib import Path
import json
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/support'))
sys.path.insert(0, str(ROOT / 'tests/p1227/c283'))
from versioned_freeze import FreezeError, digest
from history_v61 import verify

CURRENT = 'tests/p990/current-freeze-v61.json'
OLD = 'tests/p990/current-freeze-v60.json'
current = json.loads((ROOT / CURRENT).read_bytes())
with tempfile.TemporaryDirectory(prefix='c283-v60-coedit-') as directory:
    work = Path(directory)
    for name in set(current['files']) | {CURRENT}:
        p = work / name
        p.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, p)
    verify(work, CURRENT)
    p = work / OLD
    old = json.loads(p.read_bytes())
    old['files']['src/checked_c_current_internal.h'] = '0' * 64
    p.write_text(json.dumps(old, indent=2) + '\n')
    modified = json.loads((work / CURRENT).read_bytes())
    modified['previous']['sha256'] = digest(p.read_bytes())
    modified['files'][OLD] = digest(p.read_bytes())
    (work / CURRENT).write_text(json.dumps(modified, indent=2) + '\n')
    try:
        verify(work, CURRENT)
    except FreezeError as error:
        assert 'C283 fixed-c2bbb v60 SHA/blob' in str(error), str(error)
        print('C283 original v60/current simultaneous hash laundering REJECT', error)
    else:
        raise AssertionError('accepted rewritten original wrong-code history')
print('C283 original v60 fixed external SHA/OID and inherited coverage PASS')
