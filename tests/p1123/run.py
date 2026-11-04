from pathlib import Path
import hashlib,json,re,subprocess,sys,tempfile
p=Path(__file__).resolve().parent;f=json.loads((p/'freeze.json').read_bytes())
compiler,probe,raw_arg,gcc,clang,sanitizer=sys.argv[1:]
assert sanitizer in ['none','address','undefined']
for name,digest in f['files'].items():
    assert hashlib.sha256((p/name).read_bytes()).hexdigest()==digest,name
oracle=json.loads((p/'oracle.json').read_bytes())
Path(raw_arg).mkdir(parents=True,exist_ok=True)
raw=Path(tempfile.mkdtemp(prefix='p1123-',dir=raw_arg));rows=[]
def run(cmd,name):
    r=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=120)
    n=len(rows)+1;(raw/(str(n)+'-'+name+'.out')).write_bytes(r.stdout)
    (raw/(str(n)+'-'+name+'.err')).write_bytes(r.stderr)
    rows.append(dict(name=name,command=list(map(str,cmd)),status=r.returncode,
                     stdout_sha256=hashlib.sha256(r.stdout).hexdigest(),stderr_sha256=hashlib.sha256(r.stderr).hexdigest()))
    (raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    print(name,r.returncode,flush=True);return r
for case,e in oracle['cases'].items():
    source=p/'sources'/(case+'.nl')
    r=run([compiler,'--check',source],case+'-check');assert r.returncode==0 and not r.stdout and not r.stderr
    r=run([probe,source],case+'-owned');assert r.returncode==0 and b'B survives PASS' in r.stdout and not r.stderr
    r=run([compiler,source],case+'-emit');assert r.returncode==0 and not r.stderr;c=r.stdout.decode()
    assert 'for (;;) {' in c and 'continue;' in c
    # Add read-only observations beside the real helper and real inline stores.
    pattern=r'(static uint8_t nl_fn_\d+\(uint8_t \*(nl_local_\d+), uint8_t (nl_local_\d+)\)\n\{\n)(.*?)(\n\})'
    def helper(m):
        body,n=re.subn(r'(^\s*\*(nl_local_\d+) = [^;]+;)$',lambda x:x[1]+'\n    p1123_changed('+x[2]+');',m[4],flags=re.M)
        assert n==1
        return m[1]+'    p1123_helper('+m[2]+', '+m[3]+');\n'+body+m[5]
    observed,n=re.subn(pattern,helper,c,flags=re.S);assert n==1
    # Inline main stores occur before the helper definition.
    # Match stores in main only by excluding the helper implementation range.
    helpers=list(re.finditer(pattern,observed,re.S));assert len(helpers)==1
    a,b=helpers[0].span()
    prefix,n=re.subn(r'(^\s*\*(nl_local_\d+) = [^;]+;)$',lambda x:x[1]+'\n        p1123_outer_store('+x[2]+');',observed[:a],flags=re.M)
    assert n==1+e['macros']['PREFIX']
    observed=prefix+observed[a:]
    for variant,text in [('raw',c),('observed',observed)]:
        source=raw/(case+'-'+variant+'.c');source.write_text(text)
        for cc in dict.fromkeys([gcc,clang]):
            for opt in ['-O0','-O2']:
                label=case+'-'+Path(cc).name+opt+'-'+variant;exe=raw/label
                flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt]
                if sanitizer!='none':
                    flags+=['-fsanitize='+sanitizer,'-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
                if variant=='observed':
                    flags+=['-DNEWLANG_LOOP_OBSERVER="'+str(p/'observe.h')+'"']
                    flags+=['-DP1123_'+k+'='+str(v) for k,v in e['macros'].items()]
                r=run([cc,*flags,source,'-o',exe],label+'-build');assert r.returncode==0,r.stderr
                r=run([exe],label+'-native');assert r.returncode==0 and not r.stderr,(case,r.stderr)
                if variant=='observed':assert b'actual backedge/fresh phase/same original/old/postcurrent/sibling/ambient/caller PASS' in r.stdout
                exe.unlink()
for case,e in oracle['negative'].items():
    source=p/'sources'/(case+'.nl')
    r=run([compiler,'--check',source],case+'-check')
    if 'check' in e:assert r.returncode==e['check']
    else:assert r.returncode in [2,3]
    named=raw/(case+'.c');sentinel=b'P1123 original named bytes\x00\xff\n'
    named.write_bytes(sentinel);before=named.stat()
    q=run([compiler,'--emit-c17-to',named,source],case+'-named')
    assert q.returncode==e.get('emit',r.returncode) and not q.stdout
    assert named.read_bytes()==sentinel
    after=named.stat();assert (before.st_ino,before.st_mtime_ns)==(after.st_ino,after.st_mtime_ns)
    assert not list(raw.glob('.'+named.name+'*.tmp*'))
    if 'diagnostic' in e:assert e['diagnostic'].encode() in (r.stderr+q.stderr)
for case in ['03-zero-less-control','04-zero-equality-control']:
    r=run([compiler,p/'sources'/(case+'.nl')],case+'-unchanged-c17')
    assert r.returncode==0 and not r.stderr
    assert r.stdout==(p/'expected-c17'/(case+'.c17')).read_bytes()
print('C220 native finite cohort complete; no general loop soundness claim')
