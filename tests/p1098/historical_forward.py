"""Exact prospective private-attachment projection; original P605/P634 run intact."""
from pathlib import Path
import json
import runpy
import shutil
import sys
import tempfile

FIELDS = (b'    struct NLLinearBreakProof *linear_breaks;\n'
          b'    void (*destroy_linear_breaks)(NLCheckedFragment *);\n')
PRECEDING = (b'    struct NLIfFieldProof *if_fields;\n'
             b'    void (*destroy_if_fields)(NLCheckedFragment *);\n')
FOLLOWING = b'    struct NLCopyBodyProof *copy_body;\n'
CAPTURED_INCLUDE = b'#include "captured_trace_internal.h"\n'
CAPTURED_HEADER = b'#define NEWLANG_SEMANTIC_INTERNAL_H\n\n' + CAPTURED_INCLUDE
CAPTURED_FIELD = b'    NLCapturedChange *field_changes[NL_CAPTURED_CHANGE_CAPACITY];\n'
CAPTURED_STORAGE = CAPTURED_FIELD + b'    size_t field_change_count;\n'
def project(data):
    # P1387 changes only private storage spelling here. This projection is
    # confined to old checksum fixtures, never CHECK or the live compiler.
    # Keep their original full-header digest and every old corruption gate.
    assert data.count(CAPTURED_INCLUDE) == data.count(CAPTURED_HEADER) == 1
    assert data.count(CAPTURED_FIELD) == data.count(CAPTURED_STORAGE) == 1
    data = data.replace(CAPTURED_HEADER, CAPTURED_HEADER.replace(CAPTURED_INCLUDE, b''), 1)
    data = data.replace(CAPTURED_FIELD, b'    NLCapturedChange *field_changes[6];\n', 1)
    assert data.count(FIELDS) == 1
    assert data.count(PRECEDING + FIELDS + FOLLOWING) == 1
    return data.replace(PRECEDING + FIELDS + FOLLOWING, PRECEDING + FOLLOWING, 1)

def main():
    mode, *args = sys.argv[1:]
    repo = Path(__file__).resolve().parents[2]
    header = (repo / 'src/semantic_internal.h').read_bytes()
    projected = project(header)
    # Preserve the new attachment's own safety gate as well as all original
    # older-field corruption counterexamples run by P634.
    for label, bad in {
        'missing': header.replace(FIELDS, b'', 1),
        'duplicate': header + FIELDS,
        'misplaced': header.replace(FIELDS, b'', 1) + FIELDS,
        'wrong_type': header.replace(b'NLLinearBreakProof *linear_breaks', b'NLForeignProof *linear_breaks', 1),
        'wrong_destructor': header.replace(b'destroy_linear_breaks)', b'destroy_foreign)', 1),
        'missing_capture_include': header.replace(CAPTURED_INCLUDE, b'', 1),
        'duplicate_capture_include': header + CAPTURED_INCLUDE,
        'misplaced_capture_include': header.replace(CAPTURED_INCLUDE, b'', 1) + CAPTURED_INCLUDE,
        'missing_capture_storage': header.replace(CAPTURED_FIELD, b'', 1),
        'duplicate_capture_storage': header + CAPTURED_FIELD,
        'misplaced_capture_storage': header.replace(CAPTURED_FIELD, b'', 1) + CAPTURED_FIELD,
        'wrong_capture_capacity': header.replace(CAPTURED_FIELD, CAPTURED_FIELD.replace(b'NL_CAPTURED_CHANGE_CAPACITY', b'8'), 1),
    }.items():
        try:
            project(bad)
        except AssertionError:
            print('P1098 new private attachment counterexample rejected', label)
        else:
            raise AssertionError('accepted malformed private attachment: ' + label)
    with tempfile.TemporaryDirectory(prefix='p1098-historical-') as temp:
        work = Path(temp)
        files = json.loads((repo / 'tests/p605/frozen.json').read_bytes())['adopted_proof_files']
        for name in files:
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(projected if name == 'src/semantic_internal.h' else (repo / name).read_bytes())
        for name in ('p605', 'p603'):
            shutil.copytree(repo / 'tests' / name, work / 'tests' / name, dirs_exist_ok=True)
        print('P1098 prospective exact header projection only; old scripts/oracle unchanged', flush=True)
        if mode == 'p634':
            assert len(args) == 1 and Path(args[0]).resolve() == repo.resolve()
            sys.argv = [str(repo / 'tests/p634/hash_gate_test.py'), str(work)]
            runpy.run_path(str(repo / 'tests/p634/hash_gate_test.py'), run_name='__main__')
        elif mode == 'p605':
            assert len(args) == 5
            args[3] = str(work / 'tests/p605')
            sys.argv = [str(repo / 'tests/p605/integration_test.py'), *args]
            runpy.run_path(str(repo / 'tests/p605/integration_test.py'), run_name='__main__')
        else:
            raise SystemExit('expected p634 or p605')

if __name__ == '__main__':
    main()
