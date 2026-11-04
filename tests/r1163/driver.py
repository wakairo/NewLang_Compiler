"""R1163 Actions-only semantic/native runner with lossless log archives."""
import base64
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time
import traceback
import zipfile

import observer


class Stop(RuntimeError):
    def __init__(self, layer, message):
        super().__init__(message)
        self.layer = layer


def require(ok, layer, message):
    if not ok:
        raise Stop(layer, message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def broadcast(line):
    print(line, flush=True)
    # CTest normally captures successful stdout. Copy evidence to the CTest
    # process's existing Actions log descriptor, without a new remote service.
    try:
        with open('/proc/'+str(os.getppid())+'/fd/1', 'w') as stream:
            stream.write(line+'\n')
            stream.flush()
    except OSError:
        pass


class Run:
    def __init__(self, out):
        self.out = out
        self.commands = []

    def command(self, name, args, timeout=90):
        started = time.time()
        try:
            p = subprocess.run([str(a) for a in args], stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, timeout=timeout)
        except subprocess.TimeoutExpired as e:
            (self.out/(name+'.stdout')).write_bytes(e.stdout or b'')
            (self.out/(name+'.stderr')).write_bytes(e.stderr or b'')
            raise Stop('TIMEOUT', name)
        (self.out/(name+'.stdout')).write_bytes(p.stdout)
        (self.out/(name+'.stderr')).write_bytes(p.stderr)
        row = {'name':name,'argv':[str(a) for a in args], 'exit':p.returncode,
               'stdout_sha256':digest(p.stdout),'stderr_sha256':digest(p.stderr),
               'seconds':round(time.time()-started,6)}
        self.commands.append(row)
        (self.out/'commands.json').write_text(json.dumps(self.commands,indent=2)+'\n')
        return p

    def clean(self, name, args, layer, empty_stdout=False):
        p = self.command(name,args)
        require(p.returncode == 0, layer, name+' exit='+str(p.returncode))
        require(not p.stderr, layer, name+' unexpected stderr')
        if empty_stdout:
            require(not p.stdout, layer, name+' unexpected stdout')
        return p


def archive(out, summary):
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file() and p.suffix != '.exe':
                z.write(p,str(p.relative_to(out)))
    raw = buffer.getvalue()
    data = base64.b64encode(raw).decode()
    chunks = [data[i:i+2500] for i in range(0,len(data),2500)]
    ident = os.environ.get('GITHUB_JOB','unknown')+'-'+os.environ.get('GITHUB_RUN_ID','local')+'-'+digest(raw)[:12]
    broadcast('R1163 RESULT '+json.dumps(summary,sort_keys=True))
    broadcast('R1163 ARCHIVE_BEGIN '+json.dumps({'id':ident,'sha256':digest(raw),
        'bytes':len(raw),'chunks':len(chunks),'encoding':'base64-zip'},sort_keys=True))
    for i,part in enumerate(chunks):
        broadcast('R1163 ARCHIVE_CHUNK '+ident+' '+str(i)+' '+part)
    broadcast('R1163 ARCHIVE_END '+ident)


def public_check(p, case):
    rows = [json.loads(line) for line in p.stdout.decode().splitlines()]
    original = [r for r in rows if r['event']=='original']
    require(len(original)==1,'OWNED_PUBLIC','one validated original-T RETURN view')
    r = original[0]
    require(r['normal_arm']==1-case['return_arm'] and r['index']==case['index'],
            'OWNED_PUBLIC','normal source arm/index')
    require(r['old']==case['initial'][case['index']] and r['current']==case['new']
            and r['sibling_value']==case['initial'][1-case['index']],
            'OWNED_PUBLIC','replayed original child/sibling values')
    require(len([r for r in rows if r['event']=='lineage'])==1,
            'OWNED_PUBLIC','one loop-result -> ordinary Copy -> comparison lineage')
    require(any(r['event']=='public_complete' for r in rows),'OWNED_PUBLIC','complete public receipt')
    return rows


def main():
    compiler, probe, host_cc, sanitizer, source_dir, out_dir = sys.argv[1:]
    source_dir,out = Path(source_dir),Path(out_dir)
    out.mkdir(parents=True,exist_ok=False)
    run = Run(out)
    summary = {'status':'RUNNING','positive':[],'negative':[],
        'compiler_sha':'9030055314600b804fafde64adebb03c4d6315a1',
        'host_compiler':host_cc,'host_sanitizer':sanitizer,
        'guest_sanitizer':sanitizer,'github':{k:os.environ.get(k) for k in
        ('GITHUB_SHA','GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT','GITHUB_JOB','GITHUB_EVENT_NAME')},
        'environment':{'python':sys.version,'platform':platform.platform()}}
    try:
        require(os.environ.get('GITHUB_ACTIONS')=='true','ENVIRONMENT','Actions-only execution')
        oracle = json.loads((source_dir/'oracle.json').read_text())
        shutil.copyfile(source_dir/'oracle.json',out/'oracle.json')
        summary['oracle_sha256']=digest((source_dir/'oracle.json').read_bytes())
        require(summary['oracle_sha256']=='cfd4cd373e3b1b1e3c358b546cdd5691687bbc87ccc7de8e1eb31f83bc71fc63',
                'INPUT_FREEZE','V3 oracle hash')
        for case in oracle['positive']+oracle['negative']:
            data = (source_dir/case['file']).read_bytes()
            require(digest(data)==case['sha256'],'INPUT_FREEZE',case['id'])
            (out/(case['id']+'.nl')).write_bytes(data)
        observer.self_test()
        gcc = host_cc if 'gcc' in Path(host_cc).name else shutil.which('gcc-13') or shutil.which('gcc')
        clang = host_cc if 'clang' in Path(host_cc).name else shutil.which('clang-23') or shutil.which('clang')
        require(gcc and clang,'MACHINERY_TOOL','GCC and Clang required')
        versions={}
        for name,cc in [('gcc',gcc),('clang',clang),('host',host_cc)]:
            p=run.command(name+'-version',[cc,'--version'])
            require(p.returncode==0,'MACHINERY_TOOL',name+' version')
            versions[name]=p.stdout.decode(errors='replace')
        summary['versions']=versions
        for case in oracle['negative']:
            ident=case['id']; src=source_dir/case['file']
            check=run.command(ident+'-check',[compiler,'--check',src])
            require(check.returncode==3 and not check.stdout and bool(check.stderr),
                    'NEGATIVE_CHECK',ident+' must reject before C emission')
            emit=run.command(ident+'-emit',[compiler,src])
            require(emit.returncode==3 and not emit.stdout and bool(emit.stderr),
                    'NEGATIVE_EMIT',ident+' must emit no C')
            named=out/(ident+'-forbidden.c')
            p=run.command(ident+'-named',[compiler,'--emit-c17-to',named,src])
            require(p.returncode==3 and not p.stdout and not named.exists(),
                    'NEGATIVE_NAMED',ident+' must create no output')
            diagnostic=check.stderr.decode(errors='replace')
            layer='PARSE' if 'parse' in diagnostic.lower() else 'SEMANTIC_OR_REGISTRATION'
            summary['negative'].append({'id':ident,'status':'REJECTED','first_observed_layer':layer,
                'diagnostic':diagnostic,'owned':'NOT_REACHED','native':'NOT_RUN'})
        for case in oracle['positive']:
            ident=case['id'];src=source_dir/case['file']
            broadcast('R1163 CASE_BEGIN '+ident)
            run.clean(ident+'-check',[compiler,'--check',src],'SOURCE_CHECK',True)
            pub=run.clean(ident+'-public',[probe,src],'OWNED_PUBLIC')
            public_rows=public_check(pub,case)
            emit=run.clean(ident+'-emit',[compiler,src],'C17_EMIT')
            cfile=out/(ident+'.c');cfile.write_bytes(emit.stdout)
            named=out/(ident+'-named.c')
            run.clean(ident+'-named',[compiler,'--emit-c17-to',named,src],'C17_NAMED',True)
            require(named.exists() and named.read_bytes()==emit.stdout,'C17_IDENTICAL',ident)
            ast=run.command(ident+'-ast',[clang,'-std=c17','-Xclang','-ast-dump=json','-fsyntax-only',cfile])
            require(ast.returncode==0,'MACHINERY_AST',ident+' Clang AST parse')
            tree=json.loads(ast.stdout)
            try:
                observed, metadata=observer.instrument(emit.stdout.decode(),tree)
            except observer.ShapeError as e:
                raise Stop('MACHINERY_AST',ident+': '+str(e)) from e
            observed_file=out/(ident+'-observed.c');observed_file.write_text(observed)
            (out/(ident+'-observer-map.json')).write_text(json.dumps(metadata,indent=2)+'\n')
            lanes=[]
            for family,cc in [('gcc',gcc),('clang',clang)]:
                for opt in ('O0','O2'):
                    flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-'+opt]
                    if family=='clang' and '23.1.3' in versions['clang']:
                        flags+=['-Wno-error=gcc-install-dir-libstdcxx']
                    if sanitizer!='none':
                        flags+=['-fsanitize='+sanitizer,'-fno-omit-frame-pointer','-fno-sanitize-recover=all']
                    for form,inp in [('unmodified',cfile),('passive',observed_file)]:
                        label=ident+'-'+family+'-'+opt+'-'+form
                        exe=out/(label+'.exe')
                        build=run.command(label+'-build',[cc,*flags,inp,'-o',exe])
                        require(build.returncode==0,'NATIVE_BUILD' if form=='unmodified' else 'MACHINERY_OBSERVER_BUILD',label)
                        p=run.command(label+'-run',[exe])
                        require(p.returncode==0,'GUEST_SANITIZER_OR_NATIVE' if sanitizer!='none' else 'NATIVE_EXECUTION',label)
                        require(not p.stderr,'GUEST_SANITIZER_OR_STDERR',label)
                        if form=='unmodified':
                            require(not p.stdout,'UNMODIFIED_OUTPUT',label)
                            receipt={'exit':0,'stdout':'','stderr':''}
                        else:
                            try:
                                receipt=observer.validate(observer.parse_trace(p.stdout.decode()),metadata,case)
                            except observer.ShapeError as e:
                                raise Stop('NATIVE_OBSERVATION',label+': '+str(e)) from e
                        lanes.append({'family':family,'optimization':opt,'form':form,
                                      'guest_sanitizer':sanitizer,'receipt':receipt})
            summary['positive'].append({'id':ident,'status':'PASS','public':public_rows,
                'emitted_sha256':digest(emit.stdout),'passive_sha256':digest(observed.encode()),'lanes':lanes})
            broadcast('R1163 CASE_PASS '+ident)
        require(len(summary['positive'])==6 and len(summary['negative'])==7,'COMPLETENESS','finite matrix')
        summary['status']='PASS'
    except Exception as e:
        summary['status']='STOP'
        summary['layer']=e.layer if isinstance(e,Stop) else 'MACHINERY_EXCEPTION'
        summary['error']=str(e)
        (out/'traceback.txt').write_text(traceback.format_exc())
    finally:
        archive(out,summary)
    return 0 if summary['status']=='PASS' else 1


if __name__=='__main__':
    sys.exit(main())
