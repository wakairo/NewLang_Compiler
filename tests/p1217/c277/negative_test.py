"""Independent mutations of archived v5 custody and CURRENT registration."""
from pathlib import Path
import hashlib
import json
import shutil
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/p1217'))
import evidence_current_v6 as gate

sha = lambda b: hashlib.sha256(b).hexdigest()


def document(root, name, change):
    path = root / name
    value = json.loads(path.read_bytes())
    change(value)
    path.write_text(json.dumps(value, indent=2) + '\n')


def coedit_current_hash(root, name):
    document(root, 'tests/p1217/execution-freeze-v6.json',
             lambda value: value['files'].update({name: sha((root / name).read_bytes())}))


def mutate(root, fault):
    prefix = 'tests/p1217/'
    if fault == 'coedit-v5':
        name = prefix + 'execution-freeze-v5.json'
        document(root, name, lambda value: value['files'].update({
            'CMakeLists.txt': sha((root / gate.CMAKE).read_bytes())}))
        coedit_current_hash(root, name)
        return 'v5 blob: ' + name
    if fault == 'old-checker':
        name = prefix + 'evidence_test.py'
        p = root / name
        p.write_bytes(p.read_bytes().replace(b'assert sha((here / name).read_bytes()) == expected, name',
                                            b'pass # dishonest disabled original assertion'))
        coedit_current_hash(root, name)
        return 'v5 blob: ' + name
    if fault == 'old-archive':
        p = root / gate.OLD_ARCHIVE
        with zipfile.ZipFile(p) as z:
            members = {n: z.read(n) for n in z.namelist()}
        members[gate.CMAKE] += b'\n# forged historical registration\n'
        with zipfile.ZipFile(p, 'w', zipfile.ZIP_DEFLATED) as z:
            for name, data in members.items():
                z.writestr(name, data)
        coedit_current_hash(root, gate.OLD_ARCHIVE)
        return 'baseline blob: ' + gate.OLD_ARCHIVE
    if fault == 'drop-old-key':
        document(root, gate.CAPSULE,
                 lambda value: value['snapshots']['v5']['files'].pop(gate.CMAKE))
        coedit_current_hash(root, gate.CAPSULE)
        return 'snapshot capsule digest'
    if fault in ['current-oom-selector', 'current-evidence-selector']:
        p = root / gate.CMAKE
        before, after = ((b'oom_v3.py', b'oom_v2.py') if fault == 'current-oom-selector'
                         else (b'evidence_current_v6.py', b'no_checks.py'))
        p.write_bytes(p.read_bytes().replace(before, after))
        coedit_current_hash(root, gate.CMAKE)
        return 'CURRENT registration independently derived'
    if fault == 'root-current-selector':
        p = root / 'CMakeLists.txt'
        p.write_bytes(p.read_bytes().replace(b'p1217_c276_p733_current',
                                            b'p1217_c275_p733_current'))
        coedit_current_hash(root, 'CMakeLists.txt')
        return 'baseline blob: CMakeLists.txt'
    if fault == 'current-oom-body':
        name = prefix + 'oom_v3.py'
        p = root / name
        p.write_bytes(p.read_bytes().replace(b'<= 512', b'<= 1024'))
        coedit_current_hash(root, name)
        return 'baseline blob: ' + name
    if fault == 'drop-current-key':
        document(root, prefix + 'execution-freeze-v6.json',
                 lambda value: value['files'].pop(gate.CMAKE))
        return 'CURRENT exact dependency keys'
    if fault == 'ambient-enrollment':
        name = prefix + 'c277/untracked.nl'
        (root / name).write_bytes(b'untracked archival lookalike\n')
        coedit_current_hash(root, name)
        return 'CURRENT exact dependency keys'
    raise AssertionError('unknown mutation ' + fault)


def copied(root):
    shutil.copytree(ROOT / 'tests/p1217', root / 'tests/p1217')
    shutil.copyfile(ROOT / 'CMakeLists.txt', root / 'CMakeLists.txt')


if __name__ == '__main__':
    print('P1217 V6 genuine baseline', gate.verify(ROOT), flush=True)
    faults = ['coedit-v5', 'old-checker', 'old-archive', 'drop-old-key',
              'current-oom-selector', 'current-evidence-selector',
              'root-current-selector', 'current-oom-body',
              'drop-current-key', 'ambient-enrollment']
    for fault in faults:
        with tempfile.TemporaryDirectory(prefix='p1217-v6-mutation-') as directory:
            work = Path(directory)
            copied(work)
            expected = mutate(work, fault)
            try:
                gate.verify(work)
            except gate.FreezeError as error:
                if expected not in str(error):
                    raise AssertionError((fault, expected, str(error)))
                print('P1217 V6 REJECT', fault, str(error), flush=True)
            else:
                raise AssertionError('accepted tampered history/current: ' + fault)
    with tempfile.TemporaryDirectory(prefix='p1217-v6-ambient-') as directory:
        work = Path(directory)
        copied(work)
        (work / 'tests/p1217/c277/untracked.nl').write_bytes(b'not enrolled\n')
        gate.verify(work)
    print('P1217 V6 original Git/source/history + independent CURRENT + all10 negative/ambient controls PASS')
