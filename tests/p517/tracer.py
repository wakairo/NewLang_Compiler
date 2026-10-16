"""Read-only diagnostics from exactly frozen candidate; writes temporary C only."""
from pathlib import Path
import subprocess,sys
s=subprocess.check_output(['git','show','53b6883664f5f585ddd095ab357bc80e1e4018af:src/main.c'],text=True)
for function,end in [('copy_if_selection','/* Only one flat'),('copy_if_pair_frame','/* Driver-private proof:')]:
    start=s.index('static bool '+function+'(const NLCheckedFragment *f')
    finish=s.index(end,start)
    s=s[:start]+s[start:finish].replace('return false;','return (fprintf(stderr,"TRACE '+function+' line%d\\n",__LINE__),false);')+s[finish:]
Path(sys.argv[1]).write_text(s)
