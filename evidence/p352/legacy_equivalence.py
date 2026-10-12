#!/usr/bin/env python3
"""OFF/default and original ten-ON/new-OFF preprocessed TU equivalence."""
from pathlib import Path
import json,shlex,subprocess,sys,re,hashlib
root=Path(__file__).resolve().parents[2];frozen=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve()
entry=next(x for x in json.loads((frozen/'compile_commands.json').read_text()) if x['file'].endswith('/captured_closure.c'))
base_source=Path(entry['file']);cmd=shlex.split(entry['command']);flags=cmd[1:cmd.index('-o')]
rows=[]
for mode in ['default-OFF','old-ten-ON-new-OFF']:
    selected=[x for x in flags if not (mode=='default-OFF' and x.startswith('-DNEWLANG_EXPERIMENTAL_'))]
    before=subprocess.check_output([cmd[0]]+selected+['-E','-P',str(base_source)],text=True)
    after=subprocess.check_output([cmd[0]]+selected+['-E','-P',str(root/'src/captured_closure.c')],text=True)
    before=re.sub(r'\s+',' ',before);after=re.sub(r'\s+',' ',after)
    assert before==after,mode
    rows.append({'mode':mode,'preprocessed_TU_equal':True,'sha256':hashlib.sha256(before.encode()).hexdigest()})
out.write_text(json.dumps(rows,indent=2)+'\n')
print('P352_LEGACY DEFAULT_OFF and OLD_TEN_ON_NEW_OFF preprocessed captured_closure TU IDENTICAL')
