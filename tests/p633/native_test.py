"""Issue633 integrated CLI local Copy profile; immutable historic source/oracles."""
from pathlib import Path
import importlib.util
import hashlib
import json
import sys


def main():
    compiler, probe, local_probe, identity_gate, oom_gate, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    spec = importlib.util.spec_from_file_location('p633_arm_local',root/'arm_local.py')
    local = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(local)
    h = local.h
    freeze = json.loads((root/'stage0-freeze.json').read_text())
    for name,digest in freeze['files'].items():
        if name in ('tests/p626/native_test.py','tests/p628/native_test.py'):
            continue # two exact complex-arm case migrations explicitly authorized
        h.require(hashlib.sha256((root.parents[1]/name).read_bytes()).hexdigest() == digest,f'historical evidence changed: {name}')
    cases = json.loads((root.parent/'p630/frozen.json').read_text())['cases'] | json.loads((root.parent/'p630/additional-freeze.json').read_text())['cases']
    for name,oracle in cases.items():
        source = root.parent/'p630'/name
        if oracle['intent'].startswith('native'):
            expected = dict(bool=oracle['bool'],selected=oracle['selected'])
            paired = oracle['intent'] != 'native_pure'
            if paired: expected.update(old=oracle['old'],sibling=oracle['sibling'])
            result = local.observe_case(compiler,source,cc,sanitizer,expected,probe=probe if paired else None,local_probe=local_probe)
            if paired:
                fault = h.run([identity_gate,str(source)])
                h.require(fault.returncode == 0 and 'worlds=UNCHANGED driver_allocations=0' in fault.stdout,f'existing identity gate regression: {fault}')
                result['existing_gate'] = fault.stdout.strip()
            print(json.dumps(result))
        else:
            check,emitted = h.run([compiler,'--check',str(source)]),h.run([compiler,str(source)])
            if oracle['intent'] == 'semantic_reject':
                h.require(all(p.returncode == 3 and not p.stdout and oracle['code'] in p.stderr for p in (check,emitted)),f'invalid source misclassified: {check} {emitted}')
                rollback = h.run([oom_gate,'reject',str(source)])
                h.require(rollback.returncode == 0 and 'rollback=PASS artifact=NULL' in rollback.stdout,'semantic refusal rollback failed')
                extra = dict(rollback=rollback.stdout.strip())
            else:
                h.require(check.returncode == 0 and not check.stdout and not check.stderr and emitted.returncode == 4 and not emitted.stdout and 'V1-BACKEND-UNSUPPORTED' in emitted.stderr,'unsupported broad source admitted')
                extra = {}
            print(json.dumps(dict(source=str(source),check=check.returncode,c17=emitted.returncode,c_bytes=len(emitted.stdout),diagnostic=emitted.stderr.strip(),**extra)))
    for filename,oracle in json.loads((root.parent/'p631/freeze.json').read_text())['new_cases'].items():
        print(json.dumps(local.observe_case(compiler,root.parent/'p631'/filename,cc,sanitizer,oracle,probe=probe,local_probe=local_probe)))
    # P713 advances the unchanged P514 effectful source; preserve original SHA.
    source = root.parent/'p514/branch_effect.nl'
    h.require(hashlib.sha256(source.read_bytes()).hexdigest() == '757f895f06c84bcbcfc1efa60bcb3cb1b4b39fa7245ebb2bb090d861f64feceb','effect control changed')
    sys.path.insert(0, str(root.parent / 'p713'))
    from legacy import exercise
    exercise(compiler, source, cc, sanitizer)


if __name__ == '__main__':
    main()
