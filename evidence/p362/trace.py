#!/usr/bin/env python3
"""Trace a copied TU/archive; do not alter the normal compiler or accept route."""
from pathlib import Path
import hashlib,json,shlex,shutil,subprocess,sys
root=Path(__file__).resolve().parents[2];b=Path(sys.argv[1]).resolve();d=b/'trace';d.mkdir(exist_ok=True)
normal=b/'libnewlang_semantic.a';digest=hashlib.sha256(normal.read_bytes()).hexdigest()
s=(root/'src/captured_closure.c').read_text()
a='    r->reason = reason;\n    r->node = node;'
assert s.count(a)==1
s=s.replace(a,a+'\n    fprintf(stderr, "P362_TRACE reason=%s node=%zu\\n", reason, node);')
a='        if (n->kind == NL_CHECKED_REGISTERED_CALL)\n            return replay_stop(r, "UNMODELLED_CALLEE", j);'
assert s.count(a)==1
s=s.replace(a,'''        if (n->kind == NL_CHECKED_REGISTERED_CALL) {
            fprintf(stderr,
                    "P362_TRACE unsealed_attempt_direct_count=%zu "
                    "registered_call=%zu span=%zu:%zu\\n",
                    f->field_change_count, j, n->span.start_byte,
                    n->span.end_byte);
            return replay_stop(r, "UNMODELLED_CALLEE", j);
        }''')
s='#ifndef NEWLANG_TEST_P362_TRACE\n#error "test trace only"\n#endif\n#include <stdio.h>\n'+s
source=d/'captured_closure.c';source.write_text(s)
formatter=shutil.which('clang-format-23')
if formatter:subprocess.run([formatter,'-style=file:'+str(root/'.clang-format'),'-i',str(source)],check=True)
e=next(x for x in json.loads((b/'compile_commands.json').read_text()) if x['file'].endswith('/captured_closure.c'))
a=shlex.split(e['command']);flags=a[1:a.index('-o')]+['-I'+str(root/'src'),'-DNEWLANG_TEST_P362_TRACE=1']
link=[x for x in flags if x.startswith('-fsanitize=') or x.startswith('-fno-sanitize') or x=='-fno-omit-frame-pointer']
if '-fno-pie' in flags:link+=['-no-pie']
subprocess.run([a[0]]+flags+['-c',str(source),'-o',str(d/'captured_closure.c.o')],check=True)
archive=d/'libtrace.a';shutil.copyfile(normal,archive)
subprocess.run(['ar','r',str(archive),str(d/'captured_closure.c.o')],check=True)
libs=[str(archive)]+[str(b/('libnewlang_'+x+'.a')) for x in ['checked_c_node','checked','parser','diagnostic','syntax','lexer','source']]
subprocess.run([a[0]]+flags+link+[str(root/'tests/integration/five_root_source_gate_evidence.c')]+libs+['-o',str(d/'boundary')],check=True)
r=subprocess.run([str(d/'boundary'),str(root/'evidence/p362/first_adoption.nl')],capture_output=True,text=True,check=True)
boundary=json.loads(r.stdout);assert boundary['registration_status']==3 and not boundary['owned_entry_artifact'] and boundary['snapshot_unchanged']
assert 'UNMODELLED_CALLEE node=149' in r.stderr and 'unsealed_attempt_direct_count=9 registered_call=149 span=12007:12053' in r.stderr
assert hashlib.sha256(normal.read_bytes()).hexdigest()==digest
result={'guard':'NEWLANG_TEST_P362_TRACE','normal_archive_sha256':digest,'normal_archive_unchanged':True,'trace':r.stderr,'boundary':boundary,'trace_tu_sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
(b/'p362-trace.json').write_text(json.dumps(result,indent=2)+'\n')
print(r.stderr,end='')
