#!/usr/bin/env python3
"""Rerun immutable R359 controls; never change that branch or source evidence."""
from pathlib import Path
import hashlib,json,shlex,subprocess,sys
root=Path(__file__).resolve().parents[2];b=Path(sys.argv[1]).resolve();d=b/'r359';d.mkdir(exist_ok=True)
pin='276ca88d34ef1d54ee0ccda4c9a4f47956ddb26f';prefix='evidence/r-finite-nine-blind/'
positive=['repeated-field.nl','distributed-fields.nl']
negative=['wrong-stability.nl','read-not-write.nl','non-link-projection.nl','ptr-base-projection.nl','duplicate-release.nl','missing-release.nl','wrong-ending-domain.nl']
hashes={}
for name in positive+negative+['observer.c']:
    data=subprocess.check_output(['git','show',pin+':'+prefix+name]);(d/name).write_bytes(data);hashes[name]=hashlib.sha256(data).hexdigest()
e=next(x for x in json.loads((b/'compile_commands.json').read_text()) if x['file'].endswith('/captured_closure.c'))
a=shlex.split(e['command']);flags=a[1:a.index('-o')]
# The immutable R observer predates formatting. Compile it unchanged, excluding
# only this style warning; it is not copied into the tracked C-format scope.
flags=[x for x in flags if x!='-Wclang-format-violations']
link=[x for x in flags if x.startswith('-fsanitize=') or x.startswith('-fno-sanitize') or x=='-fno-omit-frame-pointer']
if '-fno-pie' in flags:link+=['-no-pie']
libs=[str(b/('libnewlang_'+x+'.a')) for x in ['semantic','checked_c_node','checked','parser','diagnostic','syntax','lexer','source']]
subprocess.run([a[0]]+flags+link+[str(d/'observer.c')]+libs+['-o',str(d/'observer')],check=True)
rows=[]
for name in positive+negative:
    r=subprocess.run([str(d/'observer'),'POS' if name in positive else 'NEG',str(d/name)],capture_output=True,text=True)
    assert r.returncode==0,(name,r.stdout,r.stderr);assert not r.stderr
    if name in positive:assert 'owned=yes' in r.stdout and 'closure_bad=0' in r.stdout
    else:
        status='SEMANTIC_UNSUPPORTED' if name in ['non-link-projection.nl','ptr-base-projection.nl'] else 'SEMANTIC_ERROR'
        assert 'owned=no' in r.stdout and 'REGISTER status='+status in r.stdout
    rows.append({'name':name,'exit':r.returncode,'stdout':r.stdout})
(b/'p362-r359.json').write_text(json.dumps({'evidence_commit':pin,'hashes':hashes,'rows':rows},indent=2)+'\n')
print('P362_R359 controls: two actual owned9/public positives + five semantic + two unsupported negatives PASS')
