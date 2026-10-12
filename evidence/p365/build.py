from pathlib import Path
import subprocess,sys,json,shlex,hashlib
r=Path(__file__).resolve().parents[2];b=Path(sys.argv[1]).resolve()
e=next(x for x in json.loads((b/'compile_commands.json').read_text()) if x['file'].endswith('/captured_closure.c'));a=shlex.split(e['command']);flags=a[1:a.index('-o')]+['-I'+str(r/'tests/support')]
link=[x for x in flags if x.startswith('-fsanitize=') or x.startswith('-fno-sanitize') or x=='-fno-omit-frame-pointer']
if '-fno-pie' in flags:link+=['-no-pie']
libs=[str(b/('libnewlang_'+x+'.a')) for x in ['semantic','checked_c_node','checked','parser','diagnostic','syntax','lexer','source']]
s=subprocess.check_output(['git','show','46a02e60cabce1b63cc3db4bc9035fe06f5f8009:evidence/p329/public_observer.c'],text=True)
assert hashlib.sha256(s.encode()).hexdigest()=='6cb44a88f43c7f844f243aabca8e11ced74536d7876a8fbed0604dd2656f3619'
(b/'parent.c').write_text(s.replace('#include "../../tests/support/test.h"','#include "test.h"'))
for source,name,wrap in [(b/'parent.c','baseline',True),(r/'evidence/p365/graph.c','graph',False)]:
    extra=['-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=realloc'] if wrap else []
    subprocess.run([a[0]]+flags+link+[str(source)]+libs+extra+['-o',str(b/name)],check=True)
