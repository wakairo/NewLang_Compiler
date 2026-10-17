"""Frozen actual-source, public proof, staged C17 and passive native carrier audit."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

def run(args):
    return subprocess.run(args, capture_output=True, text=True, timeout=60)

def main():
    compiler, probe, gate, cc, directory, sanitizer = sys.argv[1:]
    folder=Path(directory)
    manifest=json.loads((folder/'frozen.json').read_text())
    calibration=json.loads((folder/'negative-calibration-freeze.json').read_text())
    for name,case in manifest['sources'].items():
        assert hashlib.sha256((folder/(name+'.nl')).read_bytes()).hexdigest()==case['sha256']
    assert hashlib.sha256((folder/calibration['source']).read_bytes()).hexdigest()==calibration['sha256']
    source=folder/'three_originals.nl'
    check=run([compiler,'--check',str(source)])
    owned=run([probe,str(source)])
    assert check.returncode==0 and not check.stdout and not check.stderr,check
    assert owned.returncode==0 and not owned.stderr and 'PUBLIC NODE_LINKS VALID' in owned.stdout,owned
    emitted=run([compiler,str(source)])
    assert emitted.returncode==0 and not emitted.stderr and emitted.stdout,emitted
    symbols={w[1]:int(w[2]) for line in owned.stdout.splitlines() if (w:=line.split())[0]=='B'}
    assert len(symbols)==11
    header=''.join(f'#define P572_{name} {symbol}\n' for name,symbol in symbols.items())+(folder/'native_observer.h').read_text()
    rows=[]
    with tempfile.TemporaryDirectory(prefix='p572-native-') as tmp:
        root=Path(tmp);c=root/'actual.c';c.write_text(emitted.stdout);h=root/'passive.h';h.write_text(header)
        flags=[] if sanitizer=='none' else [f'-fsanitize={sanitizer}','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        for opt in ['-O0','-O2']:
            for observed in [False,True]:
                exe=root/('native'+opt+str(observed))
                cmd=[cc,'-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt,*flags]
                if observed:cmd+=[f'-DNEWLANG_NODE_OBSERVER="{h}"']
                built=run([*cmd,str(c),'-o',str(exe)]);assert built.returncode==0,built.stderr
                ran=run([str(exe)]);assert ran.returncode==0 and not ran.stderr,ran
                assert ('p572 physical originals/oldCopy/reciprocal links PASS' in ran.stdout) if observed else not ran.stdout
                rows.append(dict(opt=opt,observed=observed,exit=ran.returncode,stdout=ran.stdout))
    negatives={'read_authority':('LOCAL-LOAN-PROFILE','parser'),
               'wrong_field_type':('NODE-LINK-FIELD-PROFILE','registration'),
               'scope_escape':('P8-EXIT-DEPENDENCY','registration'),
               'ptr_is_not_ref':('P3-WRITE-REF-REQUIRED','registration')}
    for name,(code,phase) in negatives.items():
        p=folder/(name+'.nl')
        for mode in [['--check'],[]]:
            result=run([compiler,*mode,str(p)])
            assert result.returncode==3 and not result.stdout and code in result.stderr,result
        refused=run([gate,'reject',str(p)])
        assert refused.returncode==0 and f'phase={phase}' in refused.stdout and 'artifact=NULL' in refused.stdout,refused
    boundaries=json.loads((folder/'boundary-freeze.json').read_text())
    for name,case in boundaries['cases'].items():
        p=folder/(name+'.nl')
        assert hashlib.sha256(p.read_bytes()).hexdigest()==case['sha256']
        checked=run([compiler,'--check',str(p)])
        rejected=run([compiler,str(p)])
        assert checked.returncode==0 and not checked.stdout and not checked.stderr,checked
        assert rejected.returncode==4 and not rejected.stdout and case['expected_diagnostic'] in rejected.stderr,rejected
    print(json.dumps(dict(compiler_host=cc,sanitizer=sanitizer,check=0,public_owned=owned.stdout,C17=0,C_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(),runs=rows,negatives=negatives),indent=2))
    print('P572 FINITE THREE ORIGINAL LEXICAL NODE LINK RUNG PASS; no heap owner/library/product claim')

if __name__=='__main__':main()
