from pathlib import Path
import hashlib
import json
import zipfile

p = Path(__file__).resolve().parent
sha = lambda b: hashlib.sha256(b).hexdigest()
with zipfile.ZipFile(p/'evidence/gate0-v1.zip') as archive:
    original = json.loads(archive.read('freeze.json'))
    for name, expected in original['files'].items():
        assert sha(archive.read(name)) == expected, name
    assert sha(archive.read('FIRST.log')) == '634fc8c56f4521944aa25a9d8321baa408692e8db888ad05244fa2d29e569731'
    assert sha(archive.read('raw/observations.json')) == 'b7b4285d360f1c3294a929080833a02763dbd9f14291705a14e6b49d4d6bf959'
    for name in ['oracle.json', *['sources/'+q.name for q in sorted((p/'sources').glob('*.nl'))]]:
        if name in original['files']:
            assert sha((p/name).read_bytes()) == original['files'][name], name
with zipfile.ZipFile(p/'evidence/inputs-v1.zip') as archive:
    original = json.loads(archive.read('freeze.json'))
    for name, expected in original['files'].items():
        assert sha(archive.read(name)) == expected, name
with zipfile.ZipFile(p/'evidence/native-FIRST-v1.zip') as archive:
    assert sha(archive.read('FIRST.log')) == '54dc96d97a428b7894928a56f5ca155e4e90b0ff18ed21cd3c5a9367155ee33b'
for name, expected in json.loads((p/'freeze-v2.json').read_bytes())['files'].items():
    assert sha((p/name).read_bytes()) == expected, name
print('P1133 independent input, original FIRST/raw and harness freeze PASS')
