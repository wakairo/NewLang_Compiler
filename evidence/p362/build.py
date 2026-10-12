#!/usr/bin/env python3
from pathlib import Path
import json,shlex,subprocess,sys
root=Path(__file__).resolve().parents[2];b=Path(sys.argv[1]).resolve();b.mkdir(exist_ok=True)
e=next(x for x in json.loads((b/'compile_commands.json').read_text()) if x['file'].endswith('/captured_closure.c'))
a=shlex.split(e['command']);flags=a[1:a.index('-o')]+['-I'+str(root/'tests/support')]
link=[x for x in flags if x.startswith('-fsanitize=') or x.startswith('-fno-sanitize') or x=='-fno-omit-frame-pointer']
if '-fno-pie' in flags:link+=['-no-pie']
libs=[str(b/('libnewlang_'+x+'.a')) for x in ['semantic','checked_c_node','checked','parser','diagnostic','syntax','lexer','source']]
raw=subprocess.check_output(['git','show','46a02e60cabce1b63cc3db4bc9035fe06f5f8009:evidence/p329/public_observer.c'],text=True)
import hashlib
assert hashlib.sha256(raw.encode()).hexdigest()=='6cb44a88f43c7f844f243aabca8e11ced74536d7876a8fbed0604dd2656f3619'
s=raw.replace('#include "../../tests/support/test.h"','#include "test.h"');(b/'parent-observer.c').write_text(s)
probe=subprocess.check_output(['git','show','82578ff6eb23970e3ae934c98a38383ca01468f7:evidence/p338/public_probe.c'],text=True)
assert hashlib.sha256(probe.encode()).hexdigest()=='2fcf1a1622fb79273d8fbcc446e95330a05ec6115d5c259f6082b87a17fc2a55'
old='status == NL_CHECK_SEMANTIC_ERROR && d.diagnostic.diagnostic.code &&'
assert probe.count(old)==1 and probe.count('"ALLOCATED-DOMAIN-MISMATCH"')==1
# Public observer only: change its EXPECTED refusal from wrong-D semantic1 to
# the genuinely observed precision3. The compiler and inspected artifact do
# not change, nor does any acceptance/validator expectation.
probe=probe.replace(old,'status == NL_CHECK_ANALYSIS_PRECISION_LIMIT && d.diagnostic.diagnostic.code &&').replace('"ALLOCATED-DOMAIN-MISMATCH"','"CAPTURED-CLOSURE-PRECISION"')
(b/'precision-retry.c').write_text(probe)

for source,target,wrap in [(b/'parent-observer.c','calibration_observer',True),(root/'evidence/p362/owned_edges.c','owned_edges',False),(root/'tests/integration/five_root_source_gate_evidence.c','boundary',False),(root/'evidence/p338/public_probe.c','public_probe',False),(b/'precision-retry.c','precision_retry',False)]:
    extra=['-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=realloc'] if wrap else []
    subprocess.run([a[0]]+flags+link+[str(source)]+libs+extra+['-o',str(b/target)],check=True)
