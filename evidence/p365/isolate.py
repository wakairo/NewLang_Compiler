from pathlib import Path
import json,shlex,subprocess,hashlib,shutil,sys
r=Path(__file__).resolve().parents[2];b=Path(sys.argv[1]).resolve();d=b/'isolated';d.mkdir(exist_ok=True)
s=(r/'src/captured_closure.c').read_text()
a='    size_t histogram[NL_CAPTURED_MAX_RELEASES + 1] = {0};\n    return validate(f, match, histogram, 0, true);'
assert s.count(a)==1
s=s.replace('NLCheckStatus nl_checked_captured_closure_validate(', '#ifdef NEWLANG_TEST_P365_OWNED8_CUSTODY\n#include "cursor.inc"\n#endif\nNLCheckStatus nl_checked_captured_closure_validate(')
s=s.replace(a,'''    size_t histogram[NL_CAPTURED_MAX_RELEASES + 1] = {0};
    NLCheckStatus status=validate(f, match, histogram, 0, true);
#ifdef NEWLANG_TEST_P365_OWNED8_CUSTODY
    if(status==NL_CHECK_OK)status=p365_walk(f,match);
#endif
    return status;''')
(d/'captured_closure.c').write_text(s)
e=next(x for x in json.loads((b/'compile_commands.json').read_text()) if x['file'].endswith('/captured_closure.c'));a=shlex.split(e['command']);flags=a[1:a.index('-o')]+['-I'+str(r/'src'),'-I'+str(r/'evidence/p365'),'-DNEWLANG_TEST_P365_OWNED8_CUSTODY=1']
subprocess.run([a[0]]+flags+['-c',str(d/'captured_closure.c'),'-o',str(d/'captured_closure.c.o')],check=True)
archive=d/'libnewlang_semantic.a';shutil.copyfile(b/'libnewlang_semantic.a',archive);subprocess.run(['ar','r',str(archive),str(d/'captured_closure.c.o')],check=True)
link=[x for x in flags if x.startswith('-fsanitize=') or x.startswith('-fno-sanitize') or x=='-fno-omit-frame-pointer']
if '-fno-pie' in flags:link+=['-no-pie']
libs=[str(archive)]+[str(b/('libnewlang_'+x+'.a')) for x in ['checked_c_node','checked','parser','diagnostic','syntax','lexer','source']]
subprocess.run([a[0]]+flags+link+['-I'+str(r/'tests/support'),str(b/'parent.c')]+libs+['-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=realloc','-o',str(d/'observer')],check=True)
