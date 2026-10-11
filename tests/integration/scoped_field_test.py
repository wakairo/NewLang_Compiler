#!/usr/bin/env python3
"""P297 source tests: classify the first gate; acceptance needs owned evidence."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

compiler, observer, boundary = sys.argv[1:4]
fixtures = Path(__file__).resolve().parents[1] / 'fixtures'
s=(fixtures / 'experimental_scoped_field_call.nl').read_text()
call='set_link(write_A@next,Option<ptr<Node>>::Some(ptr_C));'
helper='    let old = replace(field_ref,new_value);\n    unit'
def change(text,a,b):
 assert text.count(a)==1,(a,text.count(a));return text.replace(a,b)
alpha=s
for a,b in [('Node','Element'),('set_link','update_edge'),('field_ref','destination'),('new_value','incoming')]: alpha=alpha.replace(a,b)
start=s.index('fn set_link('); end=s.index('fn main()');order=s[:start]+s[end:]+s[start:end]
old_occ='''match write_A@next {
    None => { unit },
    Some(previous) => {
        set_link(write_A@next,Option<ptr<Node>>::Some(ptr_C));
        let stale=previous;
        unit
    }
};'''
cases=[('single-field',s),('alpha',alpha),('definition-after-caller',order),
 ('same-value-fresh-occurrence',change(s,call,call.replace('ptr_C','ptr_B'))),
 ('Some-to-None',change(s,call,'set_link(write_A@next,Option<ptr<Node>>::None);')),
 ('wrong-Domain',change(s,'loan_read(life_A) { |stable_A_field|','loan_read(life_C) { |stable_A_field|')),
 ('read-ref-to-write',change(s,'ref_from_ptr(write, ptr_A, stable_A_field)','ref_from_ptr(read, ptr_A, stable_A_field)')),
 ('Copy-ptr-as-stability',change(s,'ref_from_ptr(write, ptr_A, stable_A_field)','ref_from_ptr(write, ptr_A, ptr_A)')),
 ('Copy-ptr-without-stability',change(s,'ref_from_ptr(write, ptr_A, stable_A_field)','ref_from_ptr(write, ptr_A)')),
 ('Copy-ptr-as-safe-formal',change(s,call,call.replace('write_A@next','ptr_A'))),
 ('old-payload-ref-live',change(s,call,old_occ)),
 ('Domain-move-under-loan',change(s,call,'let moved_domain=life_A;'+call)),
 ('callee-ref-escape',change(s,helper,'    let old = replace(field_ref,new_value);\n    field_ref')),
 ('caller-ref-escape',change(s,'                                                unit\n                                            };\n                                            let empty_dst', '                                                write_A@next\n                                            };\n                                            let empty_dst')),
 ('EndRoot-under-loan',change(s,call,'let forbidden=loan_exclusive_read(life_A){|ending|destroy(ptr_A,ending)};'+call)),
 ('orphan-Allocation',s.rsplit('deallocate(allocation_A, full_A);',1)[0]+s.rsplit('deallocate(allocation_A, full_A);',1)[1]),
 ('duplicate-Domain',change(s,'                                            loan_read(life_A) { |stable_A_field|','                                            let first=life_A;let second=life_A;\n                                            loan_read(life_A) { |stable_A_field|')),
 ('two-formal-refs-Unknown',change(s,'new_value:Option<ptr<Node>>)','new_value:Option<ptr<Node>>, alias:ref<write,Option<ptr<Node>>>)')),
 ('other-link-field',change(s,call,call.replace('@next','@prev'))),
 ('wrong-field-projection',change(s,call,call.replace('@next','@payload'))),
 ('ordinary-field-reloan',change(s,call,'loan_write(write_A@next){|alias|'+call+'unit};')),
 ('exclusive-field-loan',change(s,call,'loan_exclusive_write(write_A@next){|alias|'+call+'unit};')),
 ('additional-field-alias',change(s,call,'let alias=write_A@next;'+call)),
 ('second-replace-in-definition',change(s,helper,'    let old=replace(field_ref,new_value);\n    let again=replace(field_ref,new_value);\n    unit')),
 ('second-helper-call',change(s,call,call+call)),
 ('Allocation-as-Copy-payload',change(s,call,call.replace('ptr_C','allocation_C'))),
 ('original-full',(fixtures / 'experimental_five_root_detach_attach.nl').read_text())]

expected = {
    'single-field': (0, '', 'accepted-source/backend-unsupported'),
    'alpha': (0, '', 'accepted-source/backend-unsupported'),
    'definition-after-caller': (0, '', 'accepted-source/backend-unsupported'),
    'same-value-fresh-occurrence': (0, '', 'accepted-source/backend-unsupported'),
    'Some-to-None': (0, '', 'accepted-source/backend-unsupported'),
    'wrong-Domain': (1, 'ALLOCATED-DOMAIN-MISMATCH', 'semantic'),
    'read-ref-to-write': (1, 'P3-TYPE-MISMATCH', 'semantic'),
    'Copy-ptr-as-stability': (1, 'ALLOCATED-REF-TYPE', 'semantic'),
    'Copy-ptr-without-stability': (None, 'ALLOCATED-CALL-ARITY', 'parser'),
    'Copy-ptr-as-safe-formal': (1, 'P3-TYPE-MISMATCH', 'semantic'),
    'old-payload-ref-live': (3, 'P9-MATCH-PRECISION', 'precision'),
    'Domain-move-under-loan': (1, 'P3-REF-CONFLICT', 'semantic'),
    'callee-ref-escape': (1, 'P8-BODY-RESULT', 'semantic'),
    'caller-ref-escape': (1, 'P297-LOAN-ESCAPE', 'semantic'),
    'EndRoot-under-loan': (1, 'P3-REF-CONFLICT', 'semantic'),
    'orphan-Allocation': (1, 'P5-SCOPE-OBLIGATION', 'semantic'),
    'duplicate-Domain': (1, 'P3-USE-AFTER-CONSUME', 'semantic'),
    'two-formal-refs-Unknown': (3, 'P8-SIGNATURE-PRECISION', 'precision'),
    'other-link-field': (3, 'P297-CALL-PRECISION', 'precision'),
    'wrong-field-projection': (2, 'HEAP-LINK-FIELD-PROFILE', 'unsupported'),
    'ordinary-field-reloan': (2, 'FIELD-PROFILE', 'unsupported'),
    'exclusive-field-loan': (None, 'P5-EXPRESSION-UNSUPPORTED', 'parser-unsupported'),
    'additional-field-alias': (3, 'CAPTURED-CLOSURE-PRECISION', 'precision'),
    'second-replace-in-definition': (3, 'P297-BODY-PRECISION', 'precision'),
    'second-helper-call': (3, 'P297-CALL-PRECISION', 'precision'),
    'Allocation-as-Copy-payload': (1, 'P6-PAYLOAD-TYPE', 'semantic'),
    'original-full': (2, 'ALLOCATED-DOMAIN-LOAN', 'unsupported'),
}
results = []
with tempfile.TemporaryDirectory(prefix='p297-source-') as directory:
    root = Path(directory)
    for name, text in cases:
        path = root / (name + '.nl')
        path.write_text(text)
        runs = [subprocess.run([compiler, str(path)], capture_output=True, text=True) for _ in range(2)]
        run = runs[0]
        assert (run.returncode, run.stdout, run.stderr) == (runs[1].returncode, runs[1].stdout, runs[1].stderr), name
        probe = subprocess.run([boundary, str(path)], capture_output=True, text=True, check=True)
        state = json.loads(probe.stdout)
        status, code, classification = expected[name]
        assert state['registration_status'] == status and state['code'] == code, (name, state)
        assert not run.stdout and not state['owned_entry_artifact'], name
        assert not state['entry_check_attempted'] and not state['backend_attempted'], name
        owned = None
        oom = []
        if status == 0:
            assert run.returncode == 4 and '[V1-BACKEND-UNSUPPORTED]' in run.stderr, name
            evidence = subprocess.run([observer, str(path)] + (['oom'] if name == 'single-field' else []),
                                      capture_output=True, text=True, check=True)
            rows = [json.loads(line) for line in evidence.stdout.splitlines()]
            owned, oom = rows[0], rows[1:]
            assert owned['owned_source_artifact'] and owned['scoped_call_validator'], name
            assert owned['caller_changes'] == 6 and owned['callee_changes'] == 1, name
            assert owned['ProjectionId_index'] == 0 and owned['old_ptr_region'] == 3, name
            target = 0 if name == 'Some-to-None' else 3 if name == 'same-value-fresh-occurrence' else 4
            assert owned['new_ptr_region'] == target and owned['old_occurrence'] != owned['new_occurrence'], name
            assert owned['prev_current_preserved'] and owned['original_A_D_preserved_unique'], name
            assert owned['old_payload_current_cleared'] and len(set(owned['Allocation_values'])) == 5 and len(set(owned['Domain_values'])) == 5, name
            assert owned['root_fact_before'] != owned['root_fact_after'], name
            assert owned['scoped_ref_ended'] and owned['matched_frees'] == 5 and not owned['native_execution'], name
            if name == 'single-field':
                assert len(oom) == 3 and all(row['injected_failures'] == 7 and row['rollback_retry'] for row in oom)
        else:
            assert run.returncode == 3 and '[' + code + ']' in run.stderr, name
            assert state['snapshot_unchanged'], name
            assert state['parse_status'] == (3 if classification == 'parser' else 2 if classification == 'parser-unsupported' else 0), name
        sha = hashlib.sha256(text.encode()).hexdigest()
        if name == 'original-full':
            assert sha == '8763eb98f68d502b8a26b49d8a023ba38dcce45090253bf2c6695fc5391d3512'
            assert (state['start_byte'], state['end_byte']) == (2140, 2282)
            assert ':53:25-56:6:' in run.stderr
        if name == 'single-field':
            assert text.count('try_allocate_one<Node>()') == 5
        results.append(dict(name=name, sha256=sha, exit=run.returncode, diagnostic=run.stderr.splitlines()[0].replace(str(path), path.name),
                            source_span=text[state['start_byte']:state['end_byte']],
                            classification=classification, boundary=state, owned=owned, oom=oom))
    assert not list(root.glob('*.c')), 'no native output'
if len(sys.argv) == 5:
    Path(sys.argv[4]).write_text(json.dumps(results, ensure_ascii=False, indent=2) + '\n')
print('P297: 5 owned-source positives; 11 semantic refusals; 6 precision, 3 unsupported, 2 parser gates; 21 sampled OOM failures')
