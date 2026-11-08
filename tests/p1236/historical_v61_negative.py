"""Run the unchanged v61 negative suite on its genuine fixed-main inputs."""
from pathlib import Path
import io
import json
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/p1217'))
import evidence_current_v9 as gate

gate.authenticate(ROOT)
name = 'tests/p990/current-freeze-v61.json'
raw = (ROOT / name).read_bytes()
gate.require(gate.digest(raw) == '0f8b4ee26b4d3f6dd9caac7daec7fec4bbe73e822cac3fae97d3f974df99013d'
             and gate.git_oid('blob', raw) == '0222e61cfc8bb4f3201f51522ae2553e6398bb06',
             'genuine v61 manifest SHA/blob')
archive = (ROOT / 'tests/p1236/evidence/history-predecessors-v3.zip').read_bytes()
gate.require(gate.digest(archive) == '486b143940867e99e039b2cfe43e8ec0edf56fe95c4c7ecd8bb92b4dd9addb2d',
             'genuine v61 changed-input archive')
with tempfile.TemporaryDirectory(prefix='p1236-genuine-v61-') as directory:
    view = Path(directory)
    files = dict(json.loads(raw)['files'], **{name: gate.digest(raw)})
    with zipfile.ZipFile(io.BytesIO(archive)) as z:
        for name, expected in files.items():
            data = z.read(name) if name in z.namelist() else (ROOT / name).read_bytes()
            gate.require(gate.digest(data) == expected, 'genuine v61 input: ' + name)
            p = view / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(data)
    result = gate.previous.previous.old.original_run(view / 'tests/p1227/c283/history_negative.py')
    gate.require(result.returncode == 0, 'unchanged v61 negative suite: ' +
                 result.stderr.decode(errors='replace'))
    print(result.stdout.decode(), end='')
print('C288 unchanged v61 negative suite / genuine original inputs PASS')
