"""Hash/JSON/packaging only: preserve existing Actions bytes; execute no compiler."""
import base64
import gzip
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
import zipfile

WORK = Path(__file__).resolve().parent
AUDIT = WORK.parent/'c236_ci_audit'
OUT = WORK/'archive_payload/evidence/r1163-c236'
FROZEN = WORK/'tests/r1163'
HEAD = 'df73c2913d18045a7c97a90f6bf5088644868782'
ORACLE = 'cfd4cd373e3b1b1e3c358b546cdd5691687bbc87ccc7de8e1eb31f83bc71fc63'
JOBS = [
    (126948281687,41937122452,'FAST1','735677fbc462c70fbe1af844a4542720b2263361'),
    (126949041752,41937346694,'FAST2','52b5b577e33b7be45419a797f6994565f2923b28'),
    (126958710768,41940191325,'FAST3',HEAD),
    (126949834380,41937577335,'FIRST-PARTIAL','52b5b577e33b7be45419a797f6994565f2923b28'),
    (126949834394,41937577311,'FIRST-GCC','52b5b577e33b7be45419a797f6994565f2923b28'),
    (126949834650,41937577311,'FIRST-GCC-Release','52b5b577e33b7be45419a797f6994565f2923b28'),
    (126949834610,41937577311,'FIRST-Clang','52b5b577e33b7be45419a797f6994565f2923b28'),
    (126949834527,41937577311,'FIRST-ASan','52b5b577e33b7be45419a797f6994565f2923b28'),
    (126949834503,41937577311,'FIRST-UBSan','52b5b577e33b7be45419a797f6994565f2923b28'),
    (126958733286,41940198010,'M2-PARTIAL',HEAD),
    (126958733018,41940197950,'M2-GCC',HEAD),
    (126958733478,41940197950,'M2-GCC-Release',HEAD),
    (126958733522,41940197950,'M2-Clang',HEAD),
    (126958733238,41940197950,'M2-ASan',HEAD),
    (126958733403,41940197950,'M2-UBSan',HEAD),
]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def save_json(path, data):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(data,indent=2,ensure_ascii=False)+'\n')


def capsules(log):
    headers, chunks, ends, summaries = {}, {}, set(), []
    for line in log.decode('utf-8-sig').splitlines():
        marker='R1163 ARCHIVE_BEGIN '
        if marker in line:
            header=json.loads(line.split(marker,1)[1]);ident=header['id']
            assert ident not in headers or headers[ident]==header
            headers[ident]=header
        m=re.search(r'R1163 ARCHIVE_CHUNK (\S+) (\d+) ([A-Za-z0-9+/=]+)',line)
        if m:
            ident,num,data=m.groups();num=int(num);slot=chunks.setdefault(ident,{})
            assert num not in slot or slot[num]==data
            slot[num]=data
        m=re.search(r'R1163 ARCHIVE_END (\S+)',line)
        if m:ends.add(m.group(1))
        marker='R1163 RESULT '
        if marker in line:summaries.append(json.loads(line.split(marker,1)[1]))
    result=[]
    for ident, header in headers.items():
        assert ident in ends and set(chunks[ident])==set(range(header['chunks']))
        raw=base64.b64decode(''.join(chunks[ident][i] for i in range(header['chunks'])),validate=True)
        assert len(raw)==header['bytes'] and sha(raw)==header['sha256']
        result.append((header,raw,summaries))
    return result


def actual_lineage(tree, meta, case, observer):
    """Post-run structural audit of recorded actual C AST, not new execution."""
    walk, core, kids, ref = observer.walk, observer.core, observer.kids, observer.refname
    functions=[n for n in kids(tree) if n.get('kind')=='FunctionDecl' and n.get('name')==meta['function']
               and any(c.get('kind')=='CompoundStmt' for c in kids(n))]
    assert len(functions)==1
    nodes=list(walk(functions[0]))
    variables={n['name']:n for n in nodes if n.get('kind')=='VarDecl'}
    loops=[n for n in nodes if n.get('kind') in ('ForStmt','WhileStmt','DoStmt')]
    assert len(loops)==1
    loop=loops[0];inside={id(n) for n in walk(loop)}
    equality=[n for n in nodes if n.get('kind')=='BinaryOperator' and n.get('opcode')=='==']
    assert len(equality)==1
    equality=equality[0]
    copy_name=ref(kids(equality)[0]);copy_node=variables[copy_name]
    assert core(kids(equality)[1]).get('kind')=='IntegerLiteral'
    assert int(core(kids(equality)[1])['value'])==case['compare']
    result_name=ref(kids(copy_node)[-1])
    assignments=[n for n in nodes if n.get('kind')=='BinaryOperator' and n.get('opcode')=='='
                 and core(kids(n)[0]).get('kind')=='DeclRefExpr']
    result_defs=[n for n in assignments if ref(kids(n)[0])==result_name]
    assert len(result_defs)==1 and id(result_defs[0]) in inside
    phase_name=ref(kids(result_defs[0])[1]);phase_node=variables[phase_name]
    assert id(phase_node) in inside
    carry_name=ref(kids(phase_node)[-1]);carry_node=variables[carry_name]
    assert id(carry_node) not in inside
    initial=core(kids(carry_node)[-1])
    assert initial.get('kind')=='IntegerLiteral' and int(initial['value'])==case['start']
    progress=[n for n in assignments if ref(kids(n)[0])==carry_name]
    assert len(progress)==1 and id(progress[0]) in inside
    arithmetic=core(kids(progress[0])[1])
    assert arithmetic.get('kind')=='BinaryOperator' and arithmetic.get('opcode')==case['op']
    assert ref(kids(arithmetic)[0])==phase_name
    assert core(kids(arithmetic)[1]).get('kind')=='IntegerLiteral'
    assert int(core(kids(arithmetic)[1])['value'])==1
    guards=[n for n in walk(loop) if n.get('kind')=='BinaryOperator' and n.get('opcode')=='<']
    assert len(guards)==1 and ref(kids(guards[0])[0])==phase_name
    assert int(core(kids(guards[0])[1])['value'])==case['bound']+(case['op']=='-')
    assert len([n for n in walk(loop) if n.get('kind')=='BreakStmt'])==1
    assert len([n for n in walk(loop) if n.get('kind')=='ContinueStmt'])==1
    root=variables[meta['root']]
    alias=root['type']['typeAliasDeclId']
    records=[n for n in variables.values() if n.get('type',{}).get('typeAliasDeclId')==alias]
    assert len(records)==1 and records[0] is root
    init=core(kids(root)[-1]);assert init.get('kind')=='InitListExpr'
    initial_fields=[int(core(n)['value']) for n in kids(init)]
    assert initial_fields==case['initial']
    offsets={name:observer.offset(node) for name,node in
             [(carry_name,carry_node),(phase_name,phase_node),(result_name,result_defs[0]),(copy_name,copy_node)]}
    assert observer.offset(root)<observer.offset(carry_node)<observer.offset(loop)
    assert observer.offset(loop,True)<observer.offset(copy_node)<observer.offset(equality)
    return {'carry':carry_name,'phase':phase_name,'break_result':result_name,'copy':copy_name,
            'root':meta['root'],'root_record_locals':1,'initial_fields':initial_fields,
            'definition_offsets':offsets,'progress_assignment_offset':observer.offset(progress[0]),
            'classification':'post-run actual C/AST structural and raw-trace audit only'}


def trace_lineage(trace, meta, case, lineage):
    def values(kind,name,offset):
        return [r['value'] for r in trace if r['kind']==kind and
                meta['events'][r['tag']].get('value')==name and
                meta['events'][r['tag']]['offset']==offset]
    offsets=lineage['definition_offsets']
    assert values('SCALAR',lineage['carry'],offsets[lineage['carry']])==[case['start']]
    assert values('SCALAR',lineage['phase'],offsets[lineage['phase']])==case['trace']
    assert values('ASSIGN',lineage['carry'],lineage['progress_assignment_offset'])==case['trace'][1:]
    assert values('ASSIGN',lineage['break_result'],offsets[lineage['break_result']])==[case['result']]
    assert values('SCALAR',lineage['copy'],offsets[lineage['copy']])==[case['result']]


def audit_capsule(raw, declared_summaries, expected_run, label):
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        assert z.testzip() is None
        names=z.namelist();assert len(names)==len(set(names))
        assert all(not Path(n).is_absolute() and '..' not in Path(n).parts for n in names)
        files={n:z.read(n) for n in names}
    members=[{'path':n,'bytes':len(data),'sha256':sha(data)} for n,data in sorted(files.items())]
    summary=json.loads(files['summary.json'])
    assert declared_summaries and all(s==summary for s in declared_summaries)
    assert summary['github']['GITHUB_RUN_ID']==str(expected_run)
    assert summary['github']['GITHUB_RUN_ATTEMPT']=='1'
    assert summary['oracle_sha256']==ORACLE==sha(files['oracle.json'])
    oracle=json.loads(files['oracle.json'])
    for case in oracle['positive']+oracle['negative']:
        assert sha(files[case['id']+'.nl'])==case['sha256']
        assert files[case['id']+'.nl']==(FROZEN/case['file']).read_bytes()
    commands=json.loads(files['commands.json'])
    for cmd in commands:
        for stream in ('stdout','stderr'):
            assert sha(files[cmd['name']+'.'+stream])==cmd[stream+'_sha256']
    native=[c for c in commands if c['name'].endswith('-run')]
    builds=[c for c in commands if c['name'].endswith('-build')]
    negative_receipts=[]
    for case in oracle['negative']:
        ident=case['id']
        for kind in ('check','emit','named'):
            name=ident+'-'+kind;cmd=next(c for c in commands if c['name']==name)
            assert cmd['exit']==3 and not files[name+'.stdout'] and files[name+'.stderr']
        assert ident+'-forbidden.c' not in files
        diagnostic=files[ident+'-check.stderr'].decode()
        code=re.search(r'error\(([^)]+)\)\[([^\]]+)\]',diagnostic)
        negative_receipts.append({'id':ident,'actual_exit_by_route':{'check':3,'emit':3,'named':3},
                                 'category':code.group(1) if code else None,
                                 'diagnostic_code':code.group(2) if code else None,
                                 'owned':'NOT_REACHED','native':'NOT_RUN'})
    if summary['guest_sanitizer']!='none':
        for c in builds:
            assert all(flag in c['argv'] for flag in ['-fsanitize='+summary['guest_sanitizer'],
                       '-fno-omit-frame-pointer','-fno-sanitize-recover=all'])
    case_rows=[]
    if summary['status']=='PASS':
        assert label.startswith('M2-')
        assert len(summary['positive'])==6 and len(summary['negative'])==7
        assert len(native)==48 and len(builds)==48
        assert all(c['exit']==0 for c in native+builds)
        observer_spec=importlib.util.spec_from_file_location('r1163_archive_observer',FROZEN/'observer.py')
        observer=importlib.util.module_from_spec(observer_spec);observer_spec.loader.exec_module(observer)
        for case in oracle['positive']:
            ident=case['id'];c=files[ident+'.c'];observed=files[ident+'-observed.c'].decode()
            assert c==files[ident+'-named.c']==files[ident+'-emit.stdout']
            meta=json.loads(files[ident+'-observer-map.json'])
            lineage=actual_lineage(json.loads(files[ident+'-ast.stdout']),meta,case,observer)
            rebuilt=c.decode()
            for edit in sorted(meta['insertions'],key=lambda e:(e['offset'],e['priority']),reverse=True):
                assert 0<=edit['offset']<=len(c)
                rebuilt=rebuilt[:edit['offset']]+edit['text']+rebuilt[edit['offset']:]
            assert meta['prefix']+rebuilt+meta['suffix']==observed
            result=next(p for p in summary['positive'] if p['id']==ident)
            assert result['emitted_sha256']==sha(c)
            assert result['passive_sha256']==sha(files[ident+'-observed.c'])
            assert len(result['lanes'])==8
            for lane in result['lanes']:
                build_name=ident+'-'+lane['family']+'-'+lane['optimization']+'-'+lane['form']+'-build'
                build=next(c for c in commands if c['name']==build_name)
                assert all(f in build['argv'] for f in ['-std=c17','-Wall','-Wextra','-Wpedantic',
                                                      '-Werror','-'+lane['optimization']])
                name=ident+'-'+lane['family']+'-'+lane['optimization']+'-'+lane['form']+'-run'
                cmd=next(c for c in commands if c['name']==name)
                assert cmd['exit']==0 and not files[name+'.stderr']
                if lane['form']=='unmodified':assert not files[name+'.stdout']
                else:
                    trace=observer.parse_trace(files[name+'.stdout'].decode())
                    receipt=observer.validate(trace,meta,case)
                    trace_lineage(trace,meta,case,lineage)
                    assert receipt==lane['receipt']
            case_rows.append({'id':ident,'status':result['status'],'source_sha256':case['sha256'],
                              'emitted_sha256':sha(c),'passive_sha256':sha(files[ident+'-observed.c']),
                              'lanes':8,'c_condition_negated':meta['c_condition_negated'],
                              'source_arm_mapping':meta['c_branch_to_source_arm'],
                              'old_copy_lineage':meta['old_copy_lineage'],
                              'actual_loop_value_lineage':lineage})
    return members, summary, commands, {'status':summary['status'],'layer':summary.get('layer'),
        'error':summary.get('error'),'github':summary['github'],
        'host_compiler':summary['host_compiler'],'host_sanitizer':summary['host_sanitizer'],
        'guest_sanitizer':summary['guest_sanitizer'],'versions':summary['versions'],
        'oracle_sha256':summary['oracle_sha256'],'native_builds':len(builds),'native_runs':len(native),
        'passive_runs':sum('-passive-run' in c['name'] for c in native),
        'unmodified_runs':sum('-unmodified-run' in c['name'] for c in native),
        'positive':case_rows,'negative':summary['negative'],'negative_route_audit':negative_receipts,
        'guest_sanitizer_build_flags_verified':summary['guest_sanitizer'] if builds else 'NOT_RUN',
        'command_count':len(commands),'command_bytes_verified':True,'all_frozen_inputs_verified':True,
        'positive_summary_count':len(summary['positive']),
        'reached_positive_commands':[{'name':c['name'],'exit':c['exit']} for c in commands
                                     if c['name'].startswith(('a_','b_','c_','d_','e_','f_'))]}


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    ledger=json.loads((WORK/'terminal-ledger.json').read_text())
    assert len(ledger['jobs'])==15 and all(j['status']=='completed' for j in ledger['jobs'])
    assert {j['id'] for j in ledger['jobs']}=={r[0] for r in JOBS}
    assert all(r['run_attempt']==1 for r in ledger['runs'])
    save_json(OUT/'run-ledger.json',ledger)
    rows=[]
    for job,run,label,head in JOBS:
        status=next(j for j in ledger['jobs'] if j['id']==job)
        runrow=next(r for r in ledger['runs'] if r['id']==run)
        assert runrow['head_sha']==head
        source=AUDIT/('job_'+str(job));target=OUT/'jobs'/str(job);target.mkdir(parents=True,exist_ok=True)
        log=(source/'job.log').read_bytes()
        compressed=gzip.compress(log,compresslevel=9,mtime=0)
        assert gzip.decompress(compressed)==log
        (target/'decoded.log.gz').write_bytes(compressed)
        row={'job_id':job,'run_id':run,'attempt':1,'label':label,'head':head,
             'conclusion':status['conclusion'],'job_url':f'https://github.com/wakairo/NewLang_Compiler/actions/runs/{run}/job/{job}',
             'decoded_log':{'path':f'jobs/{job}/decoded.log.gz','uncompressed_bytes':len(log),
                            'uncompressed_sha256':sha(log),'gzip_bytes':len(compressed),'gzip_sha256':sha(compressed)},
             'github_job_log_zip':'NOT_FETCHED; this is the complete connector-decoded original log',
             'formatter_artifact_zip':'NOT_FETCHED; do not infer from the decoded log',
             'capsules':[]}
        text=log.decode('utf-8-sig')
        row['ctest_summaries']=re.findall(r'(\d+)% tests passed, (\d+) tests failed out of (\d+)',text)
        row['ctest_failed_tests']=sorted(set(re.findall(r'^.*?\s\d+ - ([A-Za-z0-9_.-]+) \(Failed\)\s*$',text,re.M)))
        recovered=capsules(log)
        if label.startswith('FAST'):
            assert not recovered
            row['R_semantic_native']='NOT_RUN: FAST format/build/registration preparation only'
        else:
            assert len(recovered)==1
            for header,raw,declared in recovered:
                members,summary,commands,audit=audit_capsule(raw,declared,run,label)
                if (source/'capsule.zip').exists():assert raw==(source/'capsule.zip').read_bytes()
                (target/'capsule.zip').write_bytes(raw)
                save_json(target/'capsule-members.json',members)
                save_json(target/'audit.json',audit)
                row['capsules'].append({'path':f'jobs/{job}/capsule.zip','header':header,
                    'member_count':len(members),'audit':audit})
        rows.append(row)
    save_json(OUT/'jobs.json',rows)
    final=[r for r in rows if r['label'].startswith('M2-')]
    for case in json.loads((FROZEN/'oracle.json').read_text())['positive']:
        generated={p['emitted_sha256'] for row in final for cap in row['capsules']
                   for p in cap['audit']['positive'] if p['id']==case['id']}
        assert len(generated)==1, 'unexplained generated-C identity difference across final jobs'
    audit={'final_head':HEAD,'FAST_actual_jobs':3,'normal_actual_jobs':12,'manual_jobs':0,
           'all_final_actions_success':all(r['conclusion']=='success' for r in final),
           'all_final_R_finite_pass':all(r['capsules'][0]['audit']['status']=='PASS' for r in final),
           'final_native_runs':sum(r['capsules'][0]['audit']['native_runs'] for r in final),
           'first_native_runs':sum(r['capsules'][0]['audit']['native_runs'] for r in rows if r['label'].startswith('FIRST-')),
           'generated_C_identical_across_final_jobs':True,
           'local_execution':'hash/JSON/zip/gzip/text observation only; compiler/native/formatter/CMake/CTest zero',
           'old_failures_preserved':True,'V1_V2':'NOT_RUN','V3_oracle_sha256':ORACLE}
    save_json(OUT/'archive-audit.json',audit)
    print(json.dumps(audit,indent=2))


if __name__=='__main__':
    main()
