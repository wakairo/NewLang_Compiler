"""New independent mutation controls; old v6 negative suite executes unmodified."""
from pathlib import Path
import json
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/p1217'))
import evidence_current_v7 as gate


def edit(root, name, mutate):
    p = root / name
    d = json.loads(p.read_bytes())
    mutate(d)
    p.write_text(json.dumps(d, indent=2) + '\n')


def repin(root, name):
    edit(root, gate.CONTRACT, lambda d: d['files'].update({name: gate.digest((root / name).read_bytes())}))


for fault in ['old-v6-coedit', 'drop-old-key', 'current-selector', 'root-selector',
              'drop-current-key', 'ambient-enrollment', 'new-oracle']:
    with tempfile.TemporaryDirectory(prefix='p1227-v7-negative-') as directory:
        root = Path(directory)
        for name in ['p1217', 'p1227']:
            shutil.copytree(ROOT / 'tests' / name, root / 'tests' / name)
        shutil.copyfile(ROOT / 'CMakeLists.txt', root / 'CMakeLists.txt')
        support = root / 'tests/support'
        support.mkdir()
        shutil.copyfile(ROOT / 'tests/support/versioned_freeze.py', support / 'versioned_freeze.py')
        if fault == 'old-v6-coedit':
            name = 'tests/p1217/execution-freeze-v6.json'
            edit(root, name, lambda d: d['files'].update({gate.CMAKE: gate.digest((root / gate.CMAKE).read_bytes())}))
            repin(root, name)
            expected = 'fixed-aa blob: ' + name
        elif fault == 'drop-old-key':
            edit(root, gate.CAPSULE, lambda d: d['files'].pop(gate.CMAKE))
            repin(root, gate.CAPSULE)
            expected = 'fixed-aa capsule digest'
        elif fault in ['current-selector', 'root-selector']:
            name = gate.CMAKE if fault == 'current-selector' else 'CMakeLists.txt'
            p = root / name
            p.write_bytes(p.read_bytes().replace(b'evidence_current_v7.py', b'no_checks.py') if fault == 'current-selector'
                         else p.read_bytes() + b'\n# dishonest root edit\n')
            repin(root, name)
            expected = ('CURRENT registration independently derived' if fault == 'current-selector' else 'fixed-aa blob: CMakeLists.txt')
        elif fault == 'drop-current-key':
            edit(root, gate.CONTRACT, lambda d: d['files'].pop(gate.CMAKE))
            expected = 'CURRENT exact dependency keys'
        elif fault == 'ambient-enrollment':
            name = 'tests/p1227/untracked.nl'
            (root / name).write_bytes(b'ambient lookalike\n')
            repin(root, name)
            expected = 'CURRENT exact dependency keys'
        else:
            name = 'tests/p1227/oracle-v1.json'
            edit(root, name, lambda d: d['positive_expected'].update({'field_after': 0}))
            expected = 'CURRENT source digest: ' + name
        try:
            gate.authenticate(root)
        except gate.FreezeError as error:
            assert expected in str(error), (fault, expected, str(error))
            print('P1227 REJECT', fault, str(error), flush=True)
        else:
            raise AssertionError('accepted forged history/current: ' + fault)
print('P1227 all seven history/CURRENT/oracle mutation controls PASS')
