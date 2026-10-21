"""Execute the actual P605 hash-loop on isolated candidate-file copies.
Reject unrelated edits, missing/duplicate/misplaced block, and other proof edits.
The complete P605 test still independently runs original source/native checks.
"""
import ast
import contextlib
import hashlib
import io
import json
from pathlib import Path
import shutil
import sys
import tempfile


def main():
    repo = Path(sys.argv[1])
    script = (repo/'tests/p605/integration_test.py').read_text()
    module = ast.parse(script)
    function = next(n for n in module.body if isinstance(n, ast.FunctionDef) and n.name == 'main')
    loop = next(n for n in function.body if isinstance(n, ast.For))
    assert 'adopted_proof_files' in ast.get_source_segment(script,loop)
    code = compile(ast.fix_missing_locations(ast.Module(body=[loop],type_ignores=[])),str(repo/'tests/p605/integration_test.py'),'exec')
    frozen = (repo/'tests/p605/frozen.json').read_bytes()
    files = json.loads(frozen)['adopted_proof_files']
    header = (repo/'src/semantic_internal.h').read_bytes()
    start = header.index(b'    /* Borrowed issuance-time identity, independent of the current child\n')
    end = header.index(b'    const NLSemanticContext *context_origin;\n', start) + len(b'    const NLSemanticContext *context_origin;\n')
    block = header[start:end]
    with tempfile.TemporaryDirectory(prefix='p634-hash-') as temp:
        fixture = Path(temp)
        root = fixture/'tests/p605'
        root.mkdir(parents=True)
        (root/'frozen.json').write_bytes(frozen)
        for name in files:
            target = fixture/name
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(repo/name,target)
        def check():
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                exec(code,{'root':root,'hashlib':hashlib,'json':json})
            return json.loads(out.getvalue())
        provenance = check()
        assert provenance['historical_sha256'] == files['src/semantic_internal.h']
        assert provenance['candidate_sha256'] == hashlib.sha256(header).hexdigest()
        # P713 exact attachment receives the same missing/duplicate/position/
        # original-header mutation checks; the historical digest is unchanged.
        if_fields = (b'    struct NLIfFieldProof *if_fields;\n'
                     b'    void (*destroy_if_fields)(NLCheckedFragment *);\n')
        if_record = b'struct NLCheckedFragment {\n' + if_fields
        assert header.count(if_record) == header.count(if_fields) == 1
        attacks = {
            'missing_if_block': header.replace(if_fields,b'',1),
            'duplicate_if_block': header+if_fields,
            'misplaced_if_block': header.replace(if_fields,b'',1)+if_fields,
            'wrong_if_type': header.replace(b'NLIfFieldProof *if_fields',b'NLIfFieldProof *bogus'),
            'missing_block': header.replace(block,b'',1),
            'duplicate_block': header+block,
            'misplaced_block': header.replace(block,b'',1)+block,
            'changed_pointer': header.replace(b'*context_origin;',b'*other_origin;',1),
            'unrelated_header_change': header+b'\n/* unrelated change */\n',
            'missing_p701_field': header.replace(b'    struct NLCopyBodyProof *copy_body;\n',b'',1),
            'duplicate_p701_field': header+b'    struct NLCopyBodyProof *copy_body;\n',
            'changed_p701_field': header.replace(b'*copy_body;',b'*other_copy_body;',1),
        }
        rejected=[]
        for label,data in attacks.items():
            (fixture/'src/semantic_internal.h').write_bytes(data)
            try:
                check()
            except AssertionError:
                rejected.append(label)
            else:
                raise AssertionError('actual P605 gate accepted '+label)
        (fixture/'src/semantic_internal.h').write_bytes(header)
        for name in files:
            if name == 'src/semantic_internal.h':
                continue
            saved=(fixture/name).read_bytes()
            (fixture/name).write_bytes(saved+b'\n')
            try:
                check()
            except AssertionError:
                rejected.append(name)
            else:
                raise AssertionError('other adopted proof edit accepted '+name)
            (fixture/name).write_bytes(saved)
        assert check() == provenance
        assert (repo/'tests/p605/frozen.json').read_bytes() == frozen
        print(json.dumps(dict(genuine='PASS',provenance=provenance,rejected=rejected,historical_manifest='UNCHANGED')))


if __name__ == '__main__':
    main()
