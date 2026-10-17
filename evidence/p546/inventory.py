"""Read-only P546 baseline inventory; writes only separate evidence files."""
import hashlib,json,os,subprocess,time,resource
from pathlib import Path
r=Path(__file__).parent
b=Path(os.environ.get('P546_BUILD_DIR','/workspace/p546-baseline-build'))
cases={}
for k in [1,2,16,64,128,240,241]:
 text='fn main()->unit {\n'+''.join(f'    let original_{i} = Array<Option<u8>,4>{{ Option<u8>::None, Option<u8>::None, Option<u8>::None, Option<u8>::None }};\n' for i in range(k))+'    unit\n}\n'
 cases[f'roots{k}']=dict(source=text,expect='closed direct constructor profile' if k<=240 else 'existing parser budget refusal')
cases.update({
 'bare_none':dict(source='fn main()->unit {let n=Option<u8>::None;unit}\n',expect='surface calibration; no certification claim'),
 'option_return':dict(source='fn copied()->Option<u8>{Option<u8>::None}\nfn main()->unit{copied();unit}\n',expect='surface calibration; outside direct Array body'),
 'array_copy':dict(source='fn main()->unit {let a=Array<Option<u8>,4>{Option<u8>::None,Option<u8>::None,Option<u8>::None,Option<u8>::None};let b=a;unit}\n',expect='outside requested constructor-only source; measure before judging'),
 'other_count':dict(source='fn main()->unit {let a=Array<Option<u8>,3>{Option<u8>::None,Option<u8>::None,Option<u8>::None};unit}\n',expect='unsupported count parser refusal'),
 'other_type':dict(source='fn main()->unit {let a=Array<u8,4>{u8(1),u8(2),u8(3),u8(4)};unit}\n',expect='unsupported type parser refusal')})
manifest={'fixed_compiler':'1c6d9cab7349a7e501b04c9157f25f60df242e02','canonical':'Draft17.32','compiler_edits':False,'cases':{}}
for name,case in cases.items():
 data=case['source'].encode();(r/'inputs'/f'{name}.nl').write_bytes(data)
 manifest['cases'][name]=dict(sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),expect=case['expect'])
(r/'frozen-inventory.json').write_text(json.dumps(manifest,indent=2)+'\n')
with (r/'baseline-inventory.jsonl').open('w') as log:
 for name in cases:
  modes=['check','c17']
  for tool in ['newlangc','newlangc_p534']:
   for mode in modes:
    p=r/'inputs'/f'{name}.nl'
    assert hashlib.sha256(p.read_bytes()).hexdigest()==manifest['cases'][name]['sha256']
    start=time.monotonic()
    x=subprocess.run([str(b/tool)]+(['--check'] if mode=='check' else [])+[str(p)],capture_output=True,text=True)
    out=r/f'{name}-{tool}-{mode}.out';out.write_text(x.stdout)
    row=dict(case=name,tool=tool,mode=mode,status=x.returncode,elapsed=time.monotonic()-start,diagnostic=x.stderr,output_bytes=len(x.stdout),output_sha256=hashlib.sha256(x.stdout.encode()).hexdigest())
    log.write(json.dumps(row)+'\n');log.flush()
