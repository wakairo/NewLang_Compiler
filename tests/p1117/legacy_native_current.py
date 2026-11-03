"""Prospective C212 oracle for the unchanged legacy unknown-quantity source.

Original scripts, sources, baselines and first RED remain byte-identical.
Only the obsolete precision refusal is versioned; real commands still run and
all other native/type/loan/mutant/C-byte assertions execute unchanged.
"""
from pathlib import Path
import hashlib
import json
import sys

data = Path(__file__).resolve().parent
root = data.parents[1]
family = sys.argv.pop(1)
oracle = json.loads((data / 'legacy-native-oracle-v9.json').read_bytes())
source = root / oracle['source']
assert hashlib.sha256(source.read_bytes()).hexdigest() == oracle['source_sha256']
assert source.read_text() == oracle['source_text']
path = root / oracle['scripts'][family]['path']
raw = path.read_bytes()
assert hashlib.sha256(raw).hexdigest() == oracle['scripts'][family]['sha256']
text = raw.decode()
if family == 'p603':
    before = "            old = baseline['cases'][path][mode]"
    after = before + '''
            if path == 'tests/p504/unknown.nl':
                old = current_oracle[mode]
                if mode == 'check':
                    require(not result.stderr, 'current unknown CHECK diagnostic')'''
else:
    assert family == 'p504'
    before = '''            else:
                status, category, code = failures[name]'''
    after = '''            elif name == 'unknown.nl':
                for mode, result in (('check', checked), ('c17', emitted)):
                    expected = current_oracle[mode]
                    require(result.returncode == expected['exit'] and
                            hashlib.sha256(result.stdout.encode()).hexdigest() == expected['sha256'],
                            f'current unknown boundary: {result}')
                    require(expected['diagnostic'] in result.stderr if expected['exit'] else
                            not result.stderr, 'current unknown diagnostic')
                print(json.dumps(dict(source=name, check=0, c17=4, c_bytes=0,
                                      oracle='P1117 prospective v9; old oracle retained')))
''' + before
assert text.count(before) == 1
text = text.replace(before, after, 1)
sys.argv[0] = str(path)
exec(compile(text, str(path) + ':C212-prospective-v9', 'exec'),
     {'__file__': str(path), '__name__': '__main__', 'current_oracle': oracle['current']})
