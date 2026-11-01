"""Read-only rejection tracing; compiler inputs and caller ownership unchanged."""
from pathlib import Path
import hashlib,json,subprocess,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir()
text=(r/'src/checked_c_node.c').read_text().replace('static bool reject(Emit *e)','static bool original_reject(Emit *e)',1)
needle='static bool put(Emit *e, const char *format, ...)'
text=text.replace(needle,'#define reject(e) (fprintf(stderr,"P1079 TRACE %s:%d\\n",__FILE__,__LINE__),original_reject(e))\n'+needle,1)
(out/'node.c').write_text(text)
(out/'probe.c').write_text('#include "'+str(r/'tests/p965/shared.h')+'"\nint main(int argc,char **argv){(void)node_backend;NLSemanticContext *base=NULL;TestNode n={0};if(argc!=2||!registry(argv[1],&base)||!checked(base,&n))return 1;char *c=NULL;size_t z=777;NLNodeCStatus s=nl_checked_c_node(n.entry,&c,&z);fprintf(stderr,"status=%d bytes=%zu\\n",s,z);free(c);node_checked_destroy(&n);nl_semantic_destroy(base);return 0;}\n')
libs=[Path('/tmp/p1079-parent')/('libnewlang_'+n+'.a') for n in ('semantic','parser','checked_c_node','lexer','checked','syntax','source','diagnostic')]
args=['gcc','-std=c17','-Wall','-Wextra','-Werror','-pedantic-errors','-g','-O0','-I'+str(r/'include'),'-I'+str(r/'src'),out/'node.c',out/'probe.c','-Wl,--start-group',*libs,'-Wl,--end-group','-o',out/'probe']
b=subprocess.run(list(map(str,args)),capture_output=True);(out/'build.stderr').write_bytes(b.stderr);assert b.returncode==0,b.stderr
p=subprocess.run([str(out/'probe'),str(r/'tests/p1079/sources-v3/if_true.nl')],capture_output=True);(out/'probe.stderr').write_bytes(p.stderr);assert p.returncode==0
print(p.stderr.decode())
