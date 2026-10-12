#!/usr/bin/env python3
"""Replace original private TU ONLY in a copied TEST archive; normal lib untouched."""
from pathlib import Path
import json,shlex,subprocess,sys,shutil,hashlib
root=Path(__file__).resolve().parents[2];build=Path(sys.argv[1]).resolve();out=build/'p350-shadow';out.mkdir(exist_ok=True)
entry=next(x for x in json.loads((build/'compile_commands.json').read_text()) if x['file'].endswith('/captured_closure.c'))
args=shlex.split(entry['command']);compiler=args[0];flags=args[1:args.index('-o')]+['-DNEWLANG_TEST_CHRONOLOGY_SHADOW=1','-I'+str(root/'src'),'-I'+str(root/'tests/support')]
for name,output in [('private_primitive.c','captured_closure.c.o'),('shadow.c','shadow.o'),('harness.c','harness.o')]:
    subprocess.run([compiler]+flags+['-c',str(root/'evidence/p350'/name),'-o',str(out/output)],check=True)
protected=[build/'libnewlang_semantic.a',build/'newlangc'];before={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
original=build/'libnewlang_semantic.a';shadow=out/'libnewlang_semantic_shadow.a';shutil.copyfile(original,shadow)
subprocess.run(['ar','rcs',str(shadow),str(out/'captured_closure.c.o')],check=True)
assert b'nl_test_p350_' not in subprocess.check_output(['nm','-g',str(original)])
assert b'nl_test_p350_primitive' in subprocess.check_output(['nm','-g',str(shadow)])
link_flags=[x for x in flags if x.startswith('-fsanitize=') or x.startswith('-fno-sanitize') or x=='-fno-omit-frame-pointer']
if '-fno-pie' in flags:link_flags+=['-no-pie']
libs=[shadow]+[build/('libnewlang_'+x+'.a') for x in ['checked','parser','diagnostic','syntax','lexer','source']]
subprocess.run([compiler,'-g']+link_flags+[str(out/'harness.o'),str(out/'shadow.o')]+[str(x) for x in libs]+['-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=realloc','-o',str(out/'harness')],check=True)

assert all(hashlib.sha256(p.read_bytes()).hexdigest()==before[p] for p in protected)
