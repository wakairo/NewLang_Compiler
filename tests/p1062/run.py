"""C150 bounded real-current/meaning/native/original-owner/OOM measurements."""
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
assert not selected or len(selected) == 1 and selected[0] in [str(i) for i in range(6)]
selection = int(selected[0]) if selected else None
cli = Path(cli).resolve(); data = Path(__file__).resolve().parent; repo = data.parents[1]
oracle = json.loads((data/'oracle-v1.json').read_bytes())
harness = json.loads((data/'harness-oracle-v1.json').read_bytes())
old = json.loads((data/'old-C-identity-v1.json').read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
evidence = os.environ.get('P1062_EVIDENCE_DIR')
if evidence: Path(evidence).mkdir(parents=True,exist_ok=True)
archive = Path(tempfile.mkdtemp(prefix='P1062-',dir=evidence)) if evidence else None
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
with tempfile.TemporaryDirectory(prefix='p1062-') as directory:
    temp=Path(directory)
    libraries=[cli.parent/('libnewlang_'+n+'.a') for n in ['semantic','parser','checked_c_node','lexer','checked','syntax','source','diagnostic']]
    assert all(f.is_file() for f in libraries)
    for name in ['observe','consumer','oom']:
        wraps=['-Wl,--wrap='+n for n in ['malloc','calloc','realloc']] if name=='oom' else []
        ok(invoke('build-'+name,[cc,*flags,'-g','-O0','-fno-pie','-no-pie','-I'+str(repo/'include'),
                  (data/'observe.c' if name=='observe' else data/(name+'.c')),'-Wl,--start-group',*libraries,'-Wl,--end-group',*wraps,'-o',temp/name]))
    baseline={r['source']:r for r in json.loads((data/'evidence/parent-FIRST/results-FIRST.json').read_bytes())}
    # Test all genuine semantic wrong-origin sources BEFORE any native execution.
    if selection in (None,0):
        for name,expected in oracle['negative'].items():
            source=data/'sources'/(name+'.nl')
            check=invoke(name+'-CHECK',[cli,'--check',source])
            assert check.returncode==baseline[name]['CHECK'] and check.returncode in expected['CHECK_allowed'] and not check.stdout
            if not check.returncode:
                public=invoke(name+'-current',[temp/'observe',source,str(expected['calls'])]);ok(public)
                current=[r for r in rows(public) if r['P1062']=='current']
                assert len(current)==expected['calls'] and all(r['body']==r['parameters']==0 for r in current)
                print('P1062 NEGATIVE GENUINE CURRENT',name,public.stdout.decode().strip(),flush=True)
            else: assert baseline[name]['diagnostic'].split('error(semantic)[')[1].split(']')[0].encode() in check.stderr
            named=temp/'negative-existing.c';sentinel=b'P1062 preserve rejected output\n';named.write_bytes(sentinel);before=named.stat()
            for mode in ['stdout','fresh','existing']:
                target=temp/'negative-never.c' if mode=='fresh' else named
                args=[cli]+([] if mode=='stdout' else ['--emit-c17-to',target])+[source]
                r=invoke(name+'-'+mode,args)
                assert r.returncode==(check.returncode if expected['C17']=='same-as-CHECK' else expected['C17']) and not r.stdout,('SAFETY false admission STOP',name,r.returncode,r.stdout,r.stderr)
                if mode=='fresh':assert not target.exists()
                preserved(named,sentinel,before)
            print('P1062 SOURCE REJECT',name,expected,'no_clobber=YES',flush=True)
    profiles=list(oracle['positive'].items())
    if selection is not None: profiles=[profiles[selection]]
    for name,expected in profiles:
        source=data/'sources'/(name+'.nl')
        ok(invoke(name+'-CHECK',[cli,'--check',source]))
        public=invoke(name+'-current',[temp/'observe',source,str(expected['genuine_Copy_calls'])]);ok(public);r=rows(public)
        currents=[v for v in r if v['P1062']=='current'];origins=[v for v in r if v['P1062']=='origin']
        events=[v for v in r if v['P1062']=='event']
        assert len(currents)==len(origins)==len(events)==2 and all(v['body']==v['parameters']==0 for v in currents)
        assert all(v['current']==0 for v in events)
        assert all(v['event_count']==1 and v['returned_event']==0 and v['old']==v['returned'] and v['old_forward_source'] for v in origins)
        assert [v['old'] for v in origins]==expected['helper_old'] and [v['next'] for v in origins]==expected['helper_next'] and [v['sibling'] for v in origins]==expected['helper_sibling']
        assert len({v['function'] for v in currents})==2 and len({v['root'] for v in origins})==2
        direct=[v for v in r if v['P1062']=='direct'];assert len(direct)==1
        direct=direct[0];assert direct['root']==origins[1]['root'] and direct['index']==expected['direct_index'] and direct['old']==expected['direct_old'] and direct['next']==expected['direct_next']
        h=[v for v in r if v['P1062'].startswith('H_')]
        assert len(h)==3 and h[0]['current'] and h[1]['entry_GET'] and h[1]['valid'] and h[1]['return_proved'] and h[2]['entry_GET'] and h[2]['post_proved']
        print(public.stdout.decode(),end='',flush=True)
        emitted=invoke(name+'-C17',[cli,source]);ok(emitted);assert emitted.stdout
        c=temp/(name+'.c');c.write_bytes(emitted.stdout)
        if archive:(archive/(name+'.c.data')).write_bytes(emitted.stdout)
        c_sha=sha(emitted.stdout)
        frames=re.findall(rb'NL_MIXED_COPY_BIND\((\d+),\d+,&nl_b_\d+_\d+\.f0',emitted.stdout)
        assert len(frames)==2 and frames[0]==frames[1]
        symbols=[v['result_binding'] for v in origins]+[direct['binding']]
        bindings=[b'nl_b_'+frames[0]+b'_'+str(v).encode() for v in symbols]
        assert all(re.search(rb'uint8_t\s+'+b+rb'\s*=',emitted.stdout) for b in bindings)
        for v in currents:assert emitted.stdout.count(b'nl_copy_state_'+str(v['function']).encode()+b'(')==2
        values={
            'T00':expected['root_initial'][0][0],'T01':expected['root_initial'][0][1],
            'T10':expected['root_initial'][1][0],'T11':expected['root_initial'][1][1],
            'F00':expected['final'][0][0],'F01':expected['final'][0][1],
            'F10':expected['final'][1][0],'F11':expected['final'][1][1],
            'OLD0':expected['helper_old'][0],'OLD1':expected['helper_old'][1],
            'NEXT0':expected['helper_next'][0],'NEXT1':expected['helper_next'][1],
            'SIB0':expected['helper_sibling'][0],'SIB1':expected['helper_sibling'][1],
            'I0':expected['helper_indices'][0],'I1':expected['helper_indices'][1],
            'DIRECT_INDEX':expected['direct_index'],'DIRECT_OLD':expected['direct_old'],
            'DIRECT_NEXT':expected['direct_next'],'H0':expected['H'][0],'H1':expected['H'][1],
            'RET0':bindings[0].decode(),'RET1':bindings[1].decode(),'RET2':bindings[2].decode(),
            'LOCAL_PACKETS':harness['producer_local_packets'][name],
            'HEAD_READS':expected['expected_H_reads']['head_none'],
            'CALLER_READS':expected['expected_H_reads']['caller_tail'],
            'RECEIVER_READS':expected['expected_H_reads']['receiver_tail']}
        definitions=['-DP1062_'+k+'='+str(v) for k,v in values.items()]
        observer='-DNEWLANG_HEAP_OBSERVER="'+str(data/'observer.h')+'"'
        named=temp/'named.c';sentinel=b'P1062 prior complete output\n';named.write_bytes(sentinel)
        ok(invoke(name+'-named',[cli,'--emit-c17-to',named,source]));assert named.read_bytes()==emitted.stdout
        named.write_bytes(sentinel);before=named.stat()
        failure=invoke(name+'-reached-output-error',[cli,'--emit-c17-to',named,source],preexec_fn=quota)
        assert failure.returncode==1 and not failure.stdout and b'OUTPUT' in failure.stderr
        preserved(named,sentinel,before)
        ok(invoke(name+'-exact-retry',[cli,'--emit-c17-to',named,source]));assert named.read_bytes()==emitted.stdout
        for opt in ['-O0','-O2']:
            raw=temp/'raw';ok(invoke(name+opt+'-raw-build',[cc,*flags,opt,c,'-o',raw]))
            raw_result=invoke(name+opt+'-raw',[raw]);ok(raw_result);assert not raw_result.stdout
            print('P1062 RAW',name,opt,sanitizer,'physical=INCONCLUSIVE(unit exit alone)',flush=True)
            observed=temp/'observed'
            ok(invoke(name+opt+'-physical-build',[cc,*flags,opt,c,repo/'tests/integration/two_heap_platform.c',
                      '-Wl,--wrap=malloc','-Wl,--wrap=free',observer,*definitions,'-o',observed]))
            for fail,successes in [('1',0),('2',1),('0',2)]:
                native=invoke(name+opt+'-guest'+str(successes),[observed],env=dict(os.environ,NEWLANG_TEST_FAIL_CALL=fail,
                              ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
                ok(native);text=native.stdout.decode().strip()
                physical='old='+','.join(map(str,expected['helper_old']))+' direct_old='+str(expected['direct_old'])+' final='+';'.join(','.join(map(str,v)) for v in expected['final'])
                assert physical in text and 'all_H_closed_before_T=YES actual_caller_returns=YES' in text
                assert 'T_originals=2 typed_live_fields=YES no_alias=YES calls=2 direct_writes=1' in text
                assert f'allocations={successes} frees={successes} trials={2 if successes else 1} live_tail={int(successes==2)} original_H=YES teardown=YES' in text
                print(json.dumps(dict(P1062='physical',source=name,cc=cc,opt=opt,sanitizer=sanitizer,guest_successes=successes,C_SHA256=c_sha,observations=text)),flush=True)
        if sanitizer=='none':
            function=str(currents[1]['function']).encode()
            mutant,n=re.subn(rb'(static uint8_t nl_copy_state_'+function+rb'\([^\n]+;)return old;(\})',rb'\1return next;\2',emitted.stdout)
            assert n==1;mc=temp/'wrong-second-return.c';mc.write_bytes(mutant);exe=temp/'wrong-return'
            ok(invoke(name+'-calibration-build',[cc,*flags,'-O0',mc,repo/'tests/integration/two_heap_platform.c',
                      '-Wl,--wrap=malloc','-Wl,--wrap=free',observer,*definitions,'-o',exe]))
            def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
            detected=invoke(name+'-planned-wrong-return',[exe],env=dict(os.environ,NEWLANG_TEST_FAIL_CALL='1'),
                            preexec_fn=no_core,planned_signal=-signal.SIGABRT)
            assert detected.returncode==-signal.SIGABRT and b'P1062_RET1' in detected.stderr
            print('P1062 CALIBRATION second actual return old->incoming detected; defined u8 mutant only',name,flush=True)
        fault=invoke(name+'-current-owner-negative',[temp/'consumer',source]);ok(fault)
        assert fault.stdout.count(b'P1062 ACTUAL REJECT')==14 and fault.stdout.count(b'P1062 H REJECT')==4
        assert b'A_destroyed_unrestored=YES B_current_exact_C_destroy=YES' in fault.stdout
        assert fault.stdout.count(b'P1062 WORLD REJECT')==7 and b'foreign_arm_A_destroyed_unrestored=YES B_exact_C_destroy=YES' in fault.stdout
        print(fault.stdout.decode(),end='',flush=True)
        oom=invoke(name+'-reached-OOM',[temp/'oom',source]);ok(oom);assert b'P1062 OOM SUMMARY reached=' in oom.stdout
        print(oom.stdout.decode(),end='',flush=True)
    if selection in (None,0):
        for relative,expected in old['sources'].items():
            source=repo/relative;assert sha(source.read_bytes())==expected['source_SHA256']
            r=invoke('old-C-'+relative.replace('/','_'),[cli,source]);ok(r);assert sha(r.stdout)==expected['C_SHA256'],relative
        print('P1062 original C byte identities PASS',len(old['sources']),flush=True)
        r=invoke('history-v10',[sys.executable,data/'history_tests.py']);ok(r)
        assert r.stdout.count(b'P1062 HISTORY REJECT')==5
        print(r.stdout.decode(),end='',flush=True)
print('P1062 C150 selected profile',selection,'source/current/origin/physical/original owner/OOM/no-clobber PASS',flush=True)
