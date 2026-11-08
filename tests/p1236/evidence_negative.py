"""New finite CURRENT controls; authentic old v8 checks execute separately."""
from pathlib import Path
import json
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/p1217'))
import evidence_current_v9 as gate


def edit(root, name, mutate):
    p = root / name
    d = json.loads(p.read_bytes())
    mutate(d)
    p.write_text(json.dumps(d, indent=2) + '\n')


def repin(root, name):
    edit(root, gate.CONTRACT, lambda d: d['files'].update({name: gate.digest((root / name).read_bytes())}))


gate.authenticate(ROOT)
for fault in ['old-v8-coedit', 'drop-old-key', 'current-selector', 'root-selector',
              'drop-current-key', 'ambient-enrollment', 'new-oracle-coedit', 'FIRST-coedit',
              'old-negative-bypass']:
    with tempfile.TemporaryDirectory(prefix='p1236-v9-negative-') as directory:
        root = Path(directory)
        for name in ['p1217', 'p1227', 'p1236', 'support']:
            shutil.copytree(ROOT / 'tests' / name, root / 'tests' / name)
        for name in gate.CURRENT_FILES:
            target = root / name
            if not target.exists():
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, target)
        shutil.copyfile(ROOT / 'CMakeLists.txt', root / 'CMakeLists.txt')
        if fault == 'old-v8-coedit':
            name = 'tests/p1217/execution-freeze-v8.json'
            edit(root, name, lambda d: d['files'].update({gate.CMAKE: gate.digest((root / gate.CMAKE).read_bytes())}))
            repin(root, name)
            expected = 'fixed-main2931 blob: ' + name
        elif fault == 'drop-old-key':
            edit(root, gate.CAPSULE, lambda d: d['files'].pop(gate.CMAKE))
            repin(root, gate.CAPSULE)
            expected = 'fixed-main2931 capsule digest'
        elif fault in ['current-selector', 'root-selector']:
            name = gate.CMAKE if fault == 'current-selector' else 'CMakeLists.txt'
            p = root / name
            p.write_bytes(p.read_bytes().replace(b'evidence_current_v9.py', b'no_checks.py')
                          if fault == 'current-selector' else p.read_bytes() + b'\n# dishonest edit\n')
            repin(root, name)
            expected = ('CURRENT v9 registration independently derived' if fault == 'current-selector'
                        else 'fixed-main2931 blob: CMakeLists.txt')
        elif fault == 'drop-current-key':
            edit(root, gate.CONTRACT, lambda d: d['files'].pop(gate.CMAKE))
            expected = 'CURRENT v9 exact dependency keys'
        elif fault == 'ambient-enrollment':
            name = 'tests/p1236/ambient.nl'
            (root / name).write_bytes(b'ambient lookalike\n')
            repin(root, name)
            expected = 'CURRENT v9 exact dependency keys'
        elif fault == 'FIRST-coedit':
            name = gate.MATERIAL_FIRST
            p = root / name
            p.write_bytes(p.read_bytes() + b'forged original FIRST')
            repin(root, name)
            expected = 'original MATERIAL FIRST packet'
        elif fault == 'old-negative-bypass':
            name = 'tests/p1227/c283/CMakeLists.txt'
            p = root / name
            p.write_bytes(p.read_bytes().replace(b'p1236/historical_v61_negative.py', b'p1236/no_checks.py'))
            repin(root, name)
            expected = 'CURRENT v9 authentic historical-negative registration'
        else:
            name = 'tests/p1236/oracle-v2.json'
            edit(root, name, lambda d: d.update({'forged': True}))
            repin(root, name)
            expected = 'C288 externally frozen source oracle'
        try:
            gate.authenticate(root)
        except gate.FreezeError as error:
            assert expected in str(error), (fault, expected, str(error))
            print('C288 REJECT', fault, str(error), flush=True)
        else:
            raise AssertionError('accepted forged history/current: ' + fault)
print('C288 all nine history/CURRENT/oracle mutation controls PASS')
