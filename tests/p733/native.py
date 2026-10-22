"""Reuse the existing real-platform return observer, with P-owned frozen oracles.
No synthetic semantic worlds or observer-supplied guest values/ownership."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

compiler, probe, public, cc, sanitizer, directory = sys.argv[1:]
root = Path(directory).resolve()
frozen = json.loads((root/'freeze.json').read_text())
helper = root.parent/'integration/live_return_native_test.py'
# Reuse only established harness functions, before its old variant runner.
# Keep the original helper/source/oracles intact; all operations use real CLI.
prefix = helper.read_text().split('\nvariants = {', 1)[0]
prefix = prefix.replace('"-O2"', 'optimization')
prefix = prefix.replace('path.write_text(code)', '''if not plain:
        bound = re.search(r"nl_live_tail (nl_b_\\d+_\\d+) = (nl_v_\\d+);", code)
        assert bound, "actual local carrier"
        local, packed = bound.groups()
        code = code.replace(bound[0], bound[0]+f"\\nP733_BOUND({local},{packed});", 1)
        move = re.search(r"nl_live_tail (nl_v_\\d+) = "+local+r";\\n"+local+r" = \\(nl_live_tail\\)\\{0\\};", code)
        assert move, "actual whole consume at RETURN"
        code = code.replace(move[0], move[0]+f"\\nP733_MOVED({local},{move[1]});", 1)
    path.write_text(code)''')
prefix = prefix.replace('str(support / "live_return_observer.h")', repr(str(root/'local_observer.h')))
original_argv = sys.argv[:]
sys.argv = [str(helper), compiler, probe, cc, str(root/'pending.nl'), sanitizer]
namespace = {'__file__':str(helper)}
exec(compile(prefix, str(helper), 'exec'), namespace)
sys.argv = original_argv
with tempfile.TemporaryDirectory() as directory:
    tmp = Path(directory)
    for case in frozen['cases']:
        source = root/(case['name']+'.nl')
        assert hashlib.sha256(source.read_bytes()).hexdigest() == case['sha256']
        checked = subprocess.run([compiler,'--check',source],capture_output=True)
        assert checked.returncode == 0 and not checked.stdout and not checked.stderr
        view = subprocess.run([public,source],capture_output=True)
        assert view.returncode == 0 and not view.stderr, view.stderr
        evidence = subprocess.run([probe,source],capture_output=True,text=True)
        initial = [list(map(int,x.split()[1:])) for x in evidence.stdout.splitlines() if x.startswith('INIT ')]
        assert evidence.returncode == 0 and [x[-1] for x in initial] == [case['head_payload'],case['tail_payload']]
        for level in ['-O0','-O2']:
            namespace['optimization'] = level
            namespace['positive'](tmp,case['name']+level.replace('-',''),source.read_text())
        target=tmp/'preserve.c';target.write_bytes(b'valid-prior-output')
        r=subprocess.run([compiler,'--emit-c17-to',target,source],capture_output=True)
        raw=subprocess.run([compiler,source],capture_output=True)
        assert r.returncode == raw.returncode == 0 and target.read_bytes() == raw.stdout
        print(case['name'],'genuine public local-return + legacy cleanup observations PASS; O0/O2, allocation fail1/2/success, unchanged/passive PASS')
    for case in frozen['negatives']:
        source=root/(case['name']+'.nl');assert hashlib.sha256(source.read_bytes()).hexdigest()==case['sha256']
        expected=json.loads((root/'evidence/baseline.json').read_text())[source.name]['check']
        for args in [[compiler,'--check',source],[compiler,source]]:
            r=subprocess.run(args,capture_output=True);assert r.returncode==expected['exit']==3 and not r.stdout
            code=re.search(r'\[([^]]+)\]',expected['stderr'])[1].encode();assert code in r.stderr,(case,r.stderr)
        target.write_bytes(b'valid-prior-output')
        r=subprocess.run([compiler,'--emit-c17-to',target,source],capture_output=True)
        assert r.returncode==3 and target.read_bytes()==b'valid-prior-output'
        print(case['name'],'source refusal unchanged; zero C; named sink preserved',code.decode())
