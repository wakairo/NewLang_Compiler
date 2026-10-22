"""Post-finding P diagnostics. No Production/R source or predicate is changed.

Usage: python3 tests/p727/reproduce.py BUILD_DIR C_COMPILER OUTPUT_DIRECTORY
BUILD_DIR must be built from frozen P724 production sources (frontend-only OK).
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIXED = '13532ab30c55b74a6685be278fa2669b8300d6a1'
R_HEAD = 'e33d903738dcba43b45ddc75a978c63f5674c84d'


def run(args):
    return subprocess.run(list(map(str, args)), cwd=ROOT, capture_output=True)


def main():
    build, cc, output = sys.argv[1:]
    build, output = Path(build).resolve(), Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    assert run(['git', 'diff', '--quiet', FIXED, '--', 'src', 'include', 'CMakeLists.txt']).returncode == 0
    evidence = json.loads((ROOT/'tests/p727/evidence/r-evidence-hashes.json').read_text())
    for entry in evidence['files']:
        data = (ROOT/entry['p_copy']).read_bytes()
        assert hashlib.sha256(data).hexdigest() == entry['sha256']
        assert data == run(['git', 'show', f"{R_HEAD}:{entry['original']}"]).stdout
    for entry in json.loads((ROOT/'tests/p727/evidence/variant-freeze.json').read_text()):
        assert hashlib.sha256((ROOT/entry['source']).read_bytes()).hexdigest() == entry['sha256']
    source = (ROOT/'src/copy_body.c').read_text()
    assert source.encode() == run(['git', 'show', f'{FIXED}:src/copy_body.c']).stdout
    instrumented = source
    for name, end in [('parameter_entry', 'NLCheckStatus nl_checked_copy_body_validate'),
                      ('nl_checked_copy_unit_validate', None)]:
        start = instrumented.index(('static bool ' if name == 'parameter_entry' else 'NLCheckStatus ')+name+'(')
        stop = instrumented.index(end, start) if end else len(instrumented)
        lines = instrumented[start:stop].splitlines(True)
        line_base = instrumented[:start].count('\n')+1
        for index, line in enumerate(lines):
            if line.strip() in ('return false;', 'return INVALID;'):
                ret = line.strip()
                lines[index] = line.replace(ret, '{ fprintf(stderr,"P727 '+name+' reject original-line='+str(line_base+index)+'\\n"); '+ret+' }')
        instrumented = instrumented[:start]+''.join(lines)+instrumented[stop:]
    instrumented = '#include <stdio.h>\n'+instrumented
    # Text evidence is not part of either compiler or automatic build.
    (output/'instrumented-copy-body.c.txt').write_text(instrumented)
    libraries = [build/('libnewlang_'+name+'.a') for name in
                 ['parser', 'semantic', 'parser', 'lexer', 'diagnostic', 'checked', 'syntax', 'source']]
    with tempfile.TemporaryDirectory() as temporary:
        tmp = Path(temporary)
        copy = tmp/'copy.c'
        copy.write_text(instrumented)
        compiled = run([cc, '-std=c17', '-g', '-Iinclude', '-Isrc', '-c', copy, '-o', tmp/'copy.o'])
        assert compiled.returncode == 0, compiled.stderr
        programs = {}
        for name, path, extra in [('r-gate', 'tests/p727/repro/public_gate.c', []),
                                  ('public-status', 'tests/p727/forensic.c', []),
                                  ('instrumented-status', 'tests/p727/forensic.c', [tmp/'copy.o'])]:
            exe = tmp/name
            compiled = run([cc, '-std=c17', '-g', '-Iinclude', '-Itests/p727', '-Isrc', path, *extra, *libraries, '-o', exe])
            assert compiled.returncode == 0, compiled.stderr
            programs[name] = exe
        records = []
        cases = [ROOT/'tests/p727/repro/A0v2-usize-terminal.nl']
        cases += sorted((ROOT/'tests/p727/variants').glob('*.nl'))
        for path in cases:
            original = path == cases[0]
            stages = [('check', [build/'newlangc', '--check', path]),
                      ('c17', [build/'newlangc', path]),
                      ('public-status', [programs['public-status'], path]),
                      ('instrumented-status', [programs['instrumented-status'], path])]
            if original:
                stages.append(('R-gate-unmodified', [programs['r-gate'], path, '1']))
            for name, args in stages:
                result = run(args)
                records.append(dict(source=str(path.relative_to(ROOT)), stage=name,
                                    exit=result.returncode, stdout_bytes=len(result.stdout),
                                    stdout_sha256=hashlib.sha256(result.stdout).hexdigest(),
                                    stdout=result.stdout.decode() if name != 'c17' else '',
                                    stderr=result.stderr.decode()))
                if name == 'c17' and result.returncode == 0:
                    (output/(path.stem+'.c17.txt')).write_bytes(result.stdout)
        (output/'results.json').write_text(json.dumps(records, indent=2)+'\n')
        print(json.dumps([dict(source=x['source'],stage=x['stage'],exit=x['exit'],
                               stdout_bytes=x['stdout_bytes']) for x in records],indent=2))


if __name__ == '__main__':
    main()
