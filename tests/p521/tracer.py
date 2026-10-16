"""Temporary read-only instrumentation, fixed production file remains unchanged."""
from pathlib import Path
import sys
s=Path('src/main.c').read_text()
for function,end in [('copy_if_selection','/* Only one flat'),('copy_if_pair_frame','/* Driver-private proof:')]:
    start=s.index('static bool '+function+'(const NLCheckedFragment *f')
    finish=s.index(end,start)
    s=s[:start]+s[start:finish].replace('return false;','return (fprintf(stderr,"TRACE '+function+' line%d\\n",__LINE__),false);')+s[finish:]
Path(sys.argv[1]).write_text(s)
