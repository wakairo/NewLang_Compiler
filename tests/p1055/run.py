"""C144 bounded real-current/meaning/native/original-owner/OOM measurements."""
from pathlib import Path
import hashlib
import json
import os
import re
import resource
import signal
import subprocess
import sys
import tempfile

cli, cc, sanitizer, *selected = sys.argv[1:]
assert not selected or len(selected) == 1 and selected[0] in ['0','1','2','3','4']
selection = int(selected[0]) if selected else None
cli = Path(cli).resolve(); data = Path(__file__).resolve().parent; repo = data.parents[1]
oracle = json.loads((data/'oracle-v1.json').read_bytes())
old = json.loads((data/'old-C-identity-v1.json').read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
evidence = os.environ.get('P1055_EVIDENCE_DIR')
archive = Path(tempfile.mkdtemp(prefix='P1055-',dir=evidence)) if evidence else None
commands=[]


def invoke(label,args,**kw):
    planned_signal=kw.pop('planned_signal',None)
    args=list(map(str,args));r=subprocess.run(args,capture_output=True,timeout=180,**kw)
    commands.append(dict(label=label,command=args,exit=r.returncode,
                         stdout_SHA256=sha(r.stdout),stderr_SHA256=sha(r.stderr)))
    if archive:
        (archive/(label+'.stdout')).write_bytes(r.stdout);(archive/(label+'.stderr')).write_bytes(r.stderr)
        (archive/'commands-FIRST.json').write_text(json.dumps(commands,indent=2)+'\n')
    assert r.returncode>=0 or r.returncode==planned_signal,('unexpected signal STOP',label,r.returncode,r.stderr)
    return r


def ok(r):
    assert r.returncode==0 and not r.stderr,(r.returncode,r.stdout,r.stderr)


def rows(r):
    return [json.loads(s) for s in r.stdout.decode().splitlines() if s.startswith('{')]


def quota():
    signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE,(128,128))


def preserved(path,sentinel,before):
    after=path.stat()
    assert path.read_bytes()==sentinel and (before.st_dev,before.st_ino)==(after.st_dev,after.st_ino)
    assert not list(path.parent.glob('*.tmp*'))


flags=['-std=c17','-Wall','-Wextra','-Werror','-pedantic-errors']
if sanitizer!='none': flags+=['-fsanitize='+sanitizer,'-fno-omit-frame-pointer','-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='p1055-') as directory:
    temp=Path(directory)
    libraries=[cli.parent/('libnewlang_'+n+'.a') for n in ['semantic','parser','checked_c_node','lexer','checked','syntax','source','diagnostic']]
    assert all(f.is_file() for f in libraries)
    for name in ['observe','consumer','oom']:
        wraps=['-Wl,--wrap='+n for n in ['malloc','calloc','realloc']] if name=='oom' else []
        ok(invoke('build-'+name,[cc,*flags,'-g','-O0','-fno-pie','-no-pie','-I'+str(repo/'include'),
                  data/(name+'.c'),'-Wl,--start-group',*libraries,'-Wl,--end-group',*wraps,'-o',temp/name]))
    # Test all genuine semantic wrong-origin sources BEFORE any native execution.
    if selection in (None,0):
        for name,expected in oracle['negative'].items():
            source=data/'sources'/(name+'.nl')
            check=invoke(name+'-CHECK',[cli,'--check',source])
            assert check.returncode==expected['CHECK'] and not check.stdout
            if not check.returncode:
                public=invoke(name+'-current',[temp/'observe',source]);ok(public)
                current=[r for r in rows(public) if r['P1055']=='current']
                assert len(current)==2 and all(r['body']==r['parameters']==0 for r in current)
                print('P1055 NEGATIVE GENUINE CURRENT',name,public.stdout.decode().strip(),flush=True)
            else: assert expected['diagnostic'].encode() in check.stderr
            named=temp/'negative-existing.c';sentinel=b'P1055 preserve rejected output\n';named.write_bytes(sentinel);before=named.stat()
            for mode in ['stdout','fresh','existing']:
                target=temp/'negative-never.c' if mode=='fresh' else named
                args=[cli]+([] if mode=='stdout' else ['--emit-c17-to',target])+[source]
                r=invoke(name+'-'+mode,args)
                assert r.returncode==expected['C17'] and not r.stdout,('SAFETY false admission STOP',name,r.returncode,r.stdout,r.stderr)
                if mode=='fresh':assert not target.exists()
                preserved(named,sentinel,before)
            print('P1055 SOURCE REJECT',name,expected,'no_clobber=YES',flush=True)
    profiles=list(oracle['positive'].items())
    if selection is not None: profiles=[profiles[selection]]
    for name,expected in profiles:
        source=data/'sources'/(name+'.nl')
        ok(invoke(name+'-CHECK',[cli,'--check',source]))
        public=invoke(name+'-current',[temp/'observe',source]);ok(public);r=rows(public)
        currents=[v for v in r if v['P1055']=='current'];origins=[v for v in r if v['P1055']=='origin']
        events=[v for v in r if v['P1055']=='event']
        assert len(currents)==len(origins)==len(events)==2 and all(v['body']==v['parameters']==0 for v in currents)
        assert all(v['current']==0 for v in events)
        assert all(v['event_count']==1 and v['returned_event']==0 and v['old']==v['returned'] and v['old_forward_source'] for v in origins)
        assert [v['old'] for v in origins]==expected['old'] and [v['next'] for v in origins]==expected['next'] and [v['sibling'] for v in origins]==expected['sibling']
        assert len({v['function'] for v in currents})==2
        print(public.stdout.decode(),end='',flush=True)
        emitted=invoke(name+'-C17',[cli,source]);ok(emitted);assert emitted.stdout
        c=temp/(name+'.c');c.write_bytes(emitted.stdout)
        if archive:(archive/(name+'.c.data')).write_bytes(emitted.stdout)
        c_sha=sha(emitted.stdout)
        frames=re.findall(rb'NL_MIXED_COPY_BIND\((\d+),\d+,&nl_b_\d+_\d+\.f0',emitted.stdout);assert len(frames)==1
        bindings=[b'nl_b_'+frames[0]+b'_'+str(v['result_binding']).encode() for v in origins]
        assert all(re.search(rb'uint8_t\s+'+b+rb'\s*=',emitted.stdout) for b in bindings)
        for v in currents:assert emitted.stdout.count(b'nl_copy_state_'+str(v['function']).encode()+b'(')==2
        definitions=['-DP1055_T0='+str(expected['initial'][0]),'-DP1055_T1='+str(expected['initial'][1]),
                     '-DP1055_FIRST_INDEX='+str(expected['first_index']),'-DP1055_NEXT0='+str(expected['next'][0]),
                     '-DP1055_NEXT1='+str(expected['next'][1]),'-DP1055_H0='+str(expected['H'][0]),'-DP1055_H1='+str(expected['H'][1]),
                     '-DP1055_RESULT0='+bindings[0].decode(),'-DP1055_RESULT1='+bindings[1].decode()]
        observer='-DNEWLANG_HEAP_OBSERVER="'+str(data/'observer.h')+'"'
        named=temp/'named.c';sentinel=b'P1055 prior complete output\n';named.write_bytes(sentinel)
        ok(invoke(name+'-named',[cli,'--emit-c17-to',named,source]));assert named.read_bytes()==emitted.stdout
        named.write_bytes(sentinel);before=named.stat()
        failure=invoke(name+'-reached-output-error',[cli,'--emit-c17-to',named,source],preexec_fn=quota)
        assert failure.returncode==1 and not failure.stdout and b'OUTPUT' in failure.stderr
        preserved(named,sentinel,before)
        ok(invoke(name+'-exact-retry',[cli,'--emit-c17-to',named,source]));assert named.read_bytes()==emitted.stdout
        for opt in ['-O0','-O2']:
            raw=temp/'raw';ok(invoke(name+opt+'-raw-build',[cc,*flags,opt,c,'-o',raw]))
            raw_result=invoke(name+opt+'-raw',[raw]);ok(raw_result);assert not raw_result.stdout
            print('P1055 RAW',name,opt,sanitizer,'physical=INCONCLUSIVE(unit exit alone)',flush=True)
            observed=temp/'observed'
            ok(invoke(name+opt+'-physical-build',[cc,*flags,opt,c,repo/'tests/integration/two_heap_platform.c',
                      '-Wl,--wrap=malloc','-Wl,--wrap=free',observer,*definitions,'-o',observed]))
            for fail,successes in [('1',0),('2',1),('0',2)]:
                native=invoke(name+opt+'-guest'+str(successes),[observed],env=dict(os.environ,NEWLANG_TEST_FAIL_CALL=fail,
                              ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
                ok(native);text=native.stdout.decode().strip()
                physical='old='+','.join(map(str,expected['old']))+' new='+','.join(map(str,expected['next']))+' sibling='+','.join(map(str,expected['sibling']))+' calls=2'
                assert physical in text and 'actual_caller_returns=YES' in text and 'same_original_T=YES live_original_reads=YES' in text
                assert f'allocations={successes} frees={successes}' in text and f'live_tail={int(successes==2)} original_H=YES teardown=YES' in text
                print(json.dumps(dict(P1055='physical',source=name,cc=cc,opt=opt,sanitizer=sanitizer,guest_successes=successes,C_SHA256=c_sha,observations=text)),flush=True)
        if sanitizer=='none':
            function=str(currents[1]['function']).encode()
            mutant,n=re.subn(rb'(static uint8_t nl_copy_state_'+function+rb'\([^\n]+;)return old;(\})',rb'\1return next;\2',emitted.stdout)
            assert n==1;mc=temp/'wrong-second-return.c';mc.write_bytes(mutant);exe=temp/'wrong-return'
            ok(invoke(name+'-calibration-build',[cc,*flags,'-O0',mc,repo/'tests/integration/two_heap_platform.c',
                      '-Wl,--wrap=malloc','-Wl,--wrap=free',observer,*definitions,'-o',exe]))
            def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
            detected=invoke(name+'-planned-wrong-return',[exe],env=dict(os.environ,NEWLANG_TEST_FAIL_CALL='1'),
                            preexec_fn=no_core,planned_signal=-signal.SIGABRT)
            assert detected.returncode==-signal.SIGABRT and b'P1055_RESULT1' in detected.stderr
            print('P1055 CALIBRATION second actual return old->incoming detected; defined u8 mutant only',name,flush=True)
        fault=invoke(name+'-current-owner-negative',[temp/'consumer',source]);ok(fault)
        assert fault.stdout.count(b'P1055 ACTUAL REJECT')==14 and fault.stdout.count(b'P1055 H REJECT')==4
        assert b'A_destroyed_unrestored=YES B_current_exact_C_destroy=YES' in fault.stdout
        print(fault.stdout.decode(),end='',flush=True)
        oom=invoke(name+'-reached-OOM',[temp/'oom',source]);ok(oom);assert b'P1055 OOM SUMMARY reached=' in oom.stdout
        print(oom.stdout.decode(),end='',flush=True)
    if selection in (None,0):
        for relative,expected in old['sources'].items():
            source=repo/relative;assert sha(source.read_bytes())==expected['source_SHA256']
            r=invoke('old-C-'+relative.replace('/','_'),[cli,source]);ok(r);assert sha(r.stdout)==expected['C_SHA256'],relative
        print('P1055 original C byte identities PASS',len(old['sources']),flush=True)
        for script,marker in [('history_tests.py',b'P1055 HISTORY REJECT'),('history_v8_forward.py',b'P1045 HISTORY REJECT')]:
            r=invoke(script,[sys.executable,data/script]);ok(r);assert r.stdout.count(marker)==5
            print(r.stdout.decode(),end='',flush=True)
print('P1055 C144 selected profile',selection,'source/current/origin/physical/original owner/OOM/no-clobber PASS',flush=True)
