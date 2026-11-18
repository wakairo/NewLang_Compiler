"""P934 ordinary CLI; raw C17 and separate scope-local passive observation."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile

compiler, repository = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
root = repository / 'tests/p934'
frozen = json.loads((root / 'freeze.json').read_text())
cache = (compiler.parent / 'CMakeCache.txt').read_text()
cc = re.search(r'^CMAKE_C_COMPILER:[^=]+=(.+)$', cache, re.M)[1]
sanitizer = re.search(r'^NEWLANG_SANITIZER:STRING=(.+)$', cache, re.M)[1]
save = os.environ.get('P934_EVIDENCE_DIR')
if save:
    Path(save).mkdir(parents=True, exist_ok=True)

def run(args, expected=0):
    result = subprocess.run(list(map(str, args)), capture_output=True, timeout=65)
    assert result.returncode == expected, (args, result.returncode, result.stdout, result.stderr)
    return result

def digest(data):
    return hashlib.sha256(data).hexdigest()

def passive(text, case):
    function = re.findall(r'static void (nl_fn_\d+)\(void\)\n\{', text)
    assert len(function) == 1
    declarations = list(re.finditer(r'^    const (nl_type_\d+) (nl_local_\d+) = \{[^\n]*\};$', text, re.M))
    assert len(declarations) == 2
    types, names = zip(*(d.groups() for d in declarations))
    assert types[0] == types[1] and names[0] != names[1]
    choice = list(re.finditer(r'^    if \([^\n]+\) \{$', text, re.M))
    otherwise = list(re.finditer(r'^    \} else \{$', text, re.M))
    assert len(choice) == len(otherwise) == 1
    closing = text.index('\n    }\n', otherwise[0].end()) + len('\n    }')
    assert declarations[1].end() < choice[0].start() < otherwise[0].start() < closing
    edits = {}
    def add(position, code):
        edits[position] = edits.get(position, '') + '\n' + code
    pointers = '\n'.join(f'    const volatile uint8_t *const watch_{i}_{j} = &{n}.f{j};'
                         for i, n in enumerate(names) for j in range(2))
    states = ' || '.join(f'*watch_{i}_{j} != {v}' for i, vs in enumerate(case['fields_in_declaration_order']) for j, v in enumerate(vs))
    addresses = ' || '.join(f'watch_{i}_{j} != &{n}.f{j}' for i, n in enumerate(names) for j in range(2))
    distinct = f'&{names[0]} == &{names[1]} || ' + ' || '.join(f'watch_{i}_0 == watch_{i}_1' for i in range(2))
    add(declarations[1].end(), pointers + '\n    unsigned int observed_arm = 2;\n' +
        f'    if ({states} || {distinct}) abort();')
    add(choice[0].end(), '        observed_arm = 0;')
    add(otherwise[0].end(), '        observed_arm = 1;')
    arm = 0 if case['truth'] else 1
    add(closing, f'    if (observed_arm != {arm} || {states} || {addresses} || {distinct}) abort();\n' +
        '    printf("P934 roots=%p/%p values=%u,%u/%u,%u arm=%u size=%zu offset=%zu\\n",\n' +
        f'           (const void *)&{names[0]}, (const void *)&{names[1]},\n' +
        '           (unsigned)*watch_0_0, (unsigned)*watch_0_1, (unsigned)*watch_1_0, (unsigned)*watch_1_1,\n' +
        f'           observed_arm, sizeof({names[0]}), offsetof({types[0]}, f1));')
    for position, code in sorted(edits.items(), reverse=True):
        text = text[:position] + code + text[position:]
    # SAME original declarations/initializers; only observer locals/read-only
    # accesses are added inside the owning function. No pointer escapes.
    return text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>\n#include <stdio.h>', 1)

version = run([cc, '--version']).stdout.decode().splitlines()[0]
for path, sha in frozen['historical_files_unchanged'].items():
    assert digest((repository / path).read_bytes()) == sha
with tempfile.TemporaryDirectory(prefix='p934-native-') as directory:
    tmp = Path(directory)
    records = []
    for case in frozen['positives']:
        source = repository / case['path']
        assert digest(source.read_bytes()) == case['sha256']
        checked = run([compiler, '--check', source])
        assert not checked.stdout and not checked.stderr
        original = run([compiler, source])
        assert original.stdout and not original.stderr
        assert run([compiler, source]).stdout == original.stdout
        output = tmp / 'named.c'
        output.write_bytes(b'previous-valid-artifact\n')
        named = run([compiler, '--emit-c17-to', output, source])
        assert not named.stdout and not named.stderr and output.read_bytes() == original.stdout
        assert not list(tmp.glob('named.c.tmp.*'))
        text = original.stdout.decode()
        observed = passive(text, case)
        if save:
            (Path(save) / (source.stem + '.c.txt')).write_bytes(original.stdout)
            (Path(save) / (source.stem + '.passive.c.txt')).write_text(observed)
        natives = []
        for label, code in [('unmodified', text), ('passive', observed)]:
            path = tmp / (label + '.c')
            path.write_text(code)
            for optimization in ['-O0', '-O2']:
                exe = tmp / ('native-' + label + optimization)
                flags = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
                built = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', optimization, *flags, path, '-o', exe])
                assert not built.stdout and not built.stderr
                guest = run([exe])
                assert not guest.stderr
                if label == 'unmodified':
                    assert not guest.stdout
                else:
                    witness = re.fullmatch(r'P934 roots=(\S+)/(\S+) values=(\d+),(\d+)/(\d+),(\d+) arm=(\d+) size=(\d+) offset=(\d+)\n', guest.stdout.decode())
                    assert witness and witness[1] != witness[2]
                    assert list(map(int, witness.group(3, 4, 5, 6))) == sum(case['fields_in_declaration_order'], [])
                    assert int(witness[7]) == (0 if case['truth'] else 1)
                natives.append({'mode':label,'optimization':optimization,'exit':guest.returncode,'stdout':guest.stdout.decode()})
        records.append({'source':case['path'],'source_sha256':case['sha256'],'unmodified_c17_sha256':digest(original.stdout),'passive_c17_sha256':digest(observed.encode()),'native':natives})
    for case in frozen['negative_controls']:
        source = repository / case['path']
        assert digest(source.read_bytes()) == case['sha256']
        check = run([compiler, '--check', source], case['check'])
        emitted = run([compiler, source], case['C17'])
        assert not check.stdout and not emitted.stdout
        if case['diagnostic']:
            assert case['diagnostic'].encode() in emitted.stderr
        output.write_bytes(b'previous-valid-artifact\n')
        named = run([compiler, '--emit-c17-to', output, source], case['C17'])
        assert not named.stdout and output.read_bytes() == b'previous-valid-artifact\n'
        assert not list(tmp.glob('named.c.tmp.*'))
    # Existing driver wraps REAL malloc/calloc/realloc and output stream/
    # commit faults. Its original return-status and hit/cleanup checks apply.
    fault = run([compiler.with_name('p694_fault_test'), repository / frozen['positives'][-2]['path'], tmp / 'fault.c'])
    assert b'wrapped allocation failures=' in fault.stdout and b'recovery=PASS' in fault.stdout
    result = {'compiler':version,'sanitizer':sanitizer,'positives':records,'negative_controls':len(frozen['negative_controls']),'real_named_output_faults':fault.stdout.decode(),'raw_and_instrumented_distinguished':True}
    if save:
        (Path(save) / 'native-first.json').write_text(json.dumps(result,indent=2)+'\n')
        (Path(save) / 'fault-stderr-first.log').write_bytes(fault.stderr)
    print(json.dumps(result))
