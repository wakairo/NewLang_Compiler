"""Materialize exactly the fixed-P tree for unchanged history hash tests.

Only the twelve recorded history-checker launch paths are relocated. Every leaf is verified as
its Git blob and mode; no expectation, assertion, producer test or source changes.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys


def git_blob(data):
    return hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()


def main():
    current, destination = map(Path, sys.argv[1:])
    own = current / 'tests/r1163'
    manifest = json.loads((own / 'fixed_p_manifest.json').read_text())
    expected = set()
    for item in manifest:
        path = item['path']
        parts = Path(path).parts
        if not parts or Path(path).is_absolute() or '..' in parts:
            raise RuntimeError('unsafe manifest path')
        expected.add(path)
        source = own / 'fixed_p_CMakeLists.txt' if path == 'CMakeLists.txt' else current/path
        target = destination/path
        target.parent.mkdir(parents=True, exist_ok=True)
        if item['mode'] == '120000':
            data = os.readlink(source).encode()
        else:
            data = source.read_bytes()
            actual_mode = '100755' if source.stat().st_mode & 0o111 else '100644'
            if actual_mode != item['mode']:
                raise RuntimeError('fixed-P source mode mismatch: '+path)
        if git_blob(data) != item['sha']:
            raise RuntimeError('fixed-P source blob mismatch: '+path)
        if target.exists() or target.is_symlink():
            target.unlink()
        if item['mode'] == '120000':
            target.symlink_to(data.decode())
        else:
            target.write_bytes(data)
            target.chmod(0o755 if item['mode'] == '100755' else 0o644)
    observed = {str(p.relative_to(destination)) for p in destination.rglob('*')
                if p.is_file() or p.is_symlink()}
    if observed != expected:
        raise RuntimeError('fixed-P snapshot file set differs from manifest')
    for item in manifest:
        p = destination/item['path']
        data = os.readlink(p).encode() if item['mode'] == '120000' else p.read_bytes()
        if git_blob(data) != item['sha']:
            raise RuntimeError('fixed-P snapshot readback mismatch: '+item['path'])
    receipt = {'compiler':'9030055314600b804fafde64adebb03c4d6315a1',
               'tree':'261a8e404a1cf1982e13d3b94e3223d5da0e0499',
               'verified_leaves':len(manifest),
               'manifest_sha256':hashlib.sha256((own/'fixed_p_manifest.json').read_bytes()).hexdigest(),
               'only_relocated_tests':[
                   'p965_freeze.unit','p982_freeze.unit','p1088_evidence.unit',
                   'p990_new_native.integration','p1107_evidence_history.unit',
                   'p1117_evidence_history.unit','p1098_evidence_history.unit',
                   'p990_freeze_protocol.unit','p1045_two_original_native.integration',
                   'p1055_old_origin_0.integration','p1058_closed_h_0.integration',
                   'p1062_h_body_0.integration'],
               'source':str(current),'snapshot':str(destination)}
    destination.with_suffix('.receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('R1163 FIXED-P history snapshot: '+json.dumps(receipt,sort_keys=True))


if __name__ == '__main__':
    main()
