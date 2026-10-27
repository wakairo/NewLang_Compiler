"""Finite integration only: unchanged public +/- proofs and genuine Copy bool.
No new proof issuer. Supplementary sources frozen before execution in P605.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    compiler, probe, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    helper_path = root.parent / 'p603' / 'integration_test.py'
    spec = importlib.util.spec_from_file_location('p603_integration_helper', helper_path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    for name, digest in json.loads((root / 'frozen.json').read_text())['adopted_proof_files'].items():
        data = (root.parent.parent / name).read_bytes()
        if name == 'src/semantic_internal.h':
            full_digest = hashlib.sha256(data).hexdigest()
            # P961: exactly one root-bound private H origin attachment.
            # Preserve the historical full header and every older block gate.
            h_fields = b'    struct NLHOriginOwner *h_origin;\n'
            h_record = b'struct NLCheckedFragment {\n' + h_fields
            assert data.count(h_fields) == data.count(h_record) == 1
            data = data.replace(h_record, b'struct NLCheckedFragment {\n', 1)
            # P733: one exact owned producer-local observation attachment.
            # Keep the historical entire-header digest and arithmetic bytes.
            owner_fields = (b'    struct NLProducerLocalProof *producer_local;\n'
                            b'    void (*destroy_producer_local)(NLCheckedFragment *);\n')
            owner_record = b'struct NLCheckedFragment {\n' + owner_fields
            assert data.count(owner_fields) == data.count(owner_record) == 1
            data = data.replace(owner_record,b'struct NLCheckedFragment {\n',1)
            # P713: exact artifact-owned IF history attachment, no arithmetic
            # authority or historical bytes changed. Reject every other delta.
            if_fields = (b'    struct NLIfFieldProof *if_fields;\n'
                         b'    void (*destroy_if_fields)(NLCheckedFragment *);\n')
            if_record = b'struct NLCheckedFragment {\n' + if_fields
            assert data.count(if_fields) == data.count(if_record) == 1
            data = data.replace(if_record, b'struct NLCheckedFragment {\n', 1)
            # P701: exact private artifact attachment only; preserve the
            # original entire-header digest and all arithmetic proof hashes.
            copy_fields = (b'    struct NLCopyBodyProof *copy_body;\n'
                           b'    void (*destroy_copy_body)(NLCheckedFragment *);\n')
            copy_record = b'struct NLCheckedFragment {\n' + copy_fields
            assert data.count(copy_fields) == data.count(copy_record) == 1
            data = data.replace(copy_record, b'struct NLCheckedFragment {\n', 1)
            # Issue634: preserve the original full-header checksum, allowing
            # exactly the private P633 arm correlation field in its record.
            addition = (b'    /* Borrowed issuance-time identity, independent of the current child\n'
                        b'     * view. Only the authentic parent-owned record is a trusted anchor;\n'
                        b'     * this is not a seal for arbitrary writable/caller-supplied parents. */\n'
                        b'    const NLSemanticContext *context_origin;\n')
            record = (b'typedef struct {\n    NLCheckedNodeId match;\n'
                      b'    struct NLCheckedFragment *artifact; /* owns hypothetical arm context */\n'
                      + addition + b'} NLCheckedArm;')
            assert data.count(addition) == data.count(record) == 1
            data = data.replace(record, record.replace(addition, b''), 1)
            assert hashlib.sha256(data).hexdigest() == digest
            print(json.dumps(dict(header=name, candidate_sha256=full_digest,
                                  historical_sha256=digest, exact_private_block_removed=1)))
        assert hashlib.sha256(data).hexdigest() == digest
    with tempfile.TemporaryDirectory(prefix='p605-integration-') as tmp:
        for name, case in json.loads((root / 'composition-frozen.json').read_text())['cases'].items():
            source = root / name
            assert hashlib.sha256(source.read_bytes()).hexdigest() == case['sha256']
            check = helper.run([compiler, '--check', str(source)])
            emitted = helper.run([compiler, str(source)])
            helper.require(check.returncode == emitted.returncode == 0 and not check.stdout and
                           not check.stderr and not emitted.stderr, 'integrated source/check/C17 failed')
            observed = helper.run([probe, str(source)])
            helper.require(observed.returncode == 0 and not observed.stderr, f'public +/- proof or bool observation failed: {observed}')
            lines = observed.stdout.splitlines()
            evidence = json.loads(lines[0])
            helper.require(evidence['add'] == case['add'] and evidence['sub'] == case['subtract'] and
                           evidence['bool'] == case['bool'] and evidence['authority'] == 0,
                           'wrong original arithmetic or bool values')
            bindings = {}
            for line in lines[1:]:
                _, symbol, kind, value = line.split()
                bindings[int(symbol)] = (kind, int(value))
            watched, _ = helper.observe(emitted.stdout, evidence, bindings)
            results = {k: helper.execute(Path(tmp), cc, name+k, text, 0, sanitizer)
                       for k, text in [('plain', emitted.stdout), ('observed', watched)]}
            helper.require(' + ' in emitted.stdout and ' - ' in emitted.stdout and ' < ' in emitted.stdout,
                           'original guest arithmetic/comparison missing')
            print(json.dumps(dict(source=name, public=evidence, c_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                                  native=results, sanitizer=sanitizer)))
    for name, case in json.loads((root / 'mixed-frozen.json').read_text())['cases'].items():
        source = root / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == case['sha256']
        for args in (['--check'], []):
            result = helper.run([compiler, *args, str(source)])
            helper.require(result.returncode == 3 and not result.stdout and
                           ('error(unsupported)' in result.stderr or 'error(syntax)' in result.stderr),
                           f'mixed operator admitted: {result}')
        print(json.dumps(dict(source=name, exit=3, diagnostic=result.stderr.strip(), c_bytes=0)))
    print('P605 adopted +/- public revalidation and < Copy composition FINITE; no new public proof or Product GO')


if __name__ == '__main__':
    main()
