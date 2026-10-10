"""Observations only. Supply exact fixed-main CLI, output folder and retained P274 inputs."""
import json,subprocess,pathlib,hashlib
import sys
repo=pathlib.Path(__file__).resolve().parents[2]
compiler=str(pathlib.Path(sys.argv[1]).resolve())
root=pathlib.Path(sys.argv[2]).resolve();root.mkdir(parents=True,exist_ok=True)
s=(repo/'tests/fixtures/five_root_three_field.nl').read_text()
a=s.rpartition('deallocate(allocation_B, full_B);')
w=a[0]+'deallocate(allocation_C, full_B);'+a[2]
start=w.rindex('                                            let empty_B =')
end=w.index('                                            let empty_A =',start)
b=w[start:end];w=w[:start]+w[end:]
pos=w.rindex('                                            let empty_C =');w=w[:pos]+b+w[pos:]
cases={'canonical':s,'direct-wrong-original-allocation':w,'two-H-LiveTail':(repo/'tests/fixtures/live_tail_return.nl').read_text(),'two-H-custody':(repo/'tests/fixtures/live_tail_custody.nl').read_text()}
for n in ['correct-original','wrong-original-allocation','mixed-harmless-repaired']:
 cases[n]=(pathlib.Path(sys.argv[3])/(n+'.nl')).read_text()
tail=cases['two-H-LiveTail']
needle='                    let LiveTail {'
manual=tail.replace(needle, '                    let guessed = LiveTail { owned_ptr:ptr_t, owned_allocation:allocation_t, owned_domain:life_t };\n'+needle, 1)
cases['two-H-LiveTail-direct-construction']=manual
v=[]
for name,text in cases.items():
 p=root/(name+'.nl');p.write_text(text)
 r=subprocess.run([compiler,p.name],cwd=root,capture_output=True,text=True)
 v.append({'input':name,'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'exit':r.returncode,'stdout_bytes':len(r.stdout),'diagnostic':r.stderr})
assert v[0]['exit']==0 and v[1]['exit']==3 and 'P4-ALLOCATION-MISMATCH' in v[1]['diagnostic'],v
(root/'observations.json').write_text(json.dumps(v,indent=2)+'\n')
for x in v:print(x)
