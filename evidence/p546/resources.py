from pathlib import Path
import subprocess,json,os
r=Path(__file__).parent
with (r/'baseline-resources.jsonl').open('w') as f:
 for tool in [str(Path(os.environ.get('P546_BUILD_DIR','/workspace/p546-baseline-build'))/'newlangc_p534'),str(r/'heap-cli')]:
  for k in [1,2,16,64,128,240,241]:
   for memory in [0,32]:
    for mode in ['check','c17']:
     x=subprocess.run([str(r/'launcher'),str(memory),tool]+(['--check'] if mode=='check' else [])+[str(r/'inputs'/f'roots{k}.nl')],capture_output=True,text=True)
     row=dict(k=k,tool=tool,limit_mib=memory,mode=mode,status=x.returncode,output_bytes=len(x.stdout),diagnostic=x.stderr)
     f.write(json.dumps(row)+'\n');f.flush()
