"""Compare an already saved R1190 actual decode with immutable FIRST numbers.

The observer and decoder receive neither this file nor the oracle. No observed
value can change the source, expectation, or pre-execution manifest.
"""
import hashlib
import json
from pathlib import Path
import sys

ORACLE_SHA256 = '15b50df7981dc98f076fb164d36114878356520fae88cc53580583e126cca7b4'


def compare(observed, case, columns):
    expected = [dict(zip(columns, row)) for row in case['trace']]
    active = [row for row in expected if row['outer_guard']]
    diffs, incomplete = [], []
    def eq(key, actual, wanted):
        if actual != wanted:
            diffs.append({'key': key, 'actual': actual, 'expected': wanted})
    mapping = {
        'phase_headers': ('phase', expected), 'outer_guard_results': ('outer_guard', expected),
        'selected_at_headers': ('selected_before', expected), 'sibling_at_headers': ('sibling', expected),
        'helper_arguments': ('helper_actual', active), 'fresh_formal_values': ('fresh_formal', active),
        'helper_branch_results': ('helper_condition', active), 'helper_normal_results': ('helper_normal', active),
        'replace_old_results': ('old_result', active), 'actual_incoming': ('helper_normal', active),
        'selected_after_each_store': ('selected_after', active), 'sibling_after_each_store': ('sibling', active),
        'continue_arguments': ('continue_out', active)}
    for key, (column, rows) in mapping.items():
        eq(key, observed[key], [r[column] for r in rows])
    for key in ('helper_call_count', 'store_count', 'continue_count'):
        eq(key, observed[key], len(active))
    eq('break_count', observed['break_count'], 1)
    eq('initial_selected', observed['initial_selected'], case['initial_record'][case['selected_field']])
    eq('initial_sibling', observed['initial_sibling'], case['initial_record'][case['sibling_field']])
    values = {'actual_C_copy_values':'helper_actual','formal_C_copy_values':'fresh_formal',
              'before_selected_reads':'selected_before','current_reads':'selected_after','sibling_reads':'sibling',
              'caller_result_copies':'helper_normal','old_result_copies':'old_copy','entry_copies_in_iterations':'entry_copy'}
    for key, column in values.items():
        eq(key+'.interval_count', len(observed[key]), len(active))
        for i, (actual, wanted) in enumerate(zip(observed[key], active)):
            for decl, value in actual.items():
                eq(f'{key}.{i}.{decl}',value,wanted[column])
    for key, count in [('actual_C_copy_values',case['links_initializer_actual_formal'][1]),
                       ('formal_C_copy_values',case['links_initializer_actual_formal'][2])]:
        for i, row in enumerate(observed[key]):
            if len(row) < count:
                incomplete.append(f'{key}.{i}: fewer directly captured C Copy declarations than the frozen logical links')
    seed=observed['seed']; wanted=case['seed']['origin']
    eq('seed.actual_AST_origin',seed['origin'],{'kind':'literal','value':wanted})
    for decl, values in seed['C_copy_values'].items():
        if not values: incomplete.append('seed Copy declaration not directly observed: '+decl)
        eq('seed.'+decl,values,[wanted]*len(values))
    if case['seed']['copies'] and len(seed['C_copy_values']) < len(case['seed']['copies'])+1:
        incomplete.append('initializer Copy origin/additional links not fully present as captured C declarations')
    terminal=case['terminal']; post=observed['post']
    eq('post.break_value',post['break_value'],terminal['break_value'])
    eq('post.actual_break_phase',post['actual_break_phase'],terminal['break_value'])
    for actual, key in [('break_copies','break_copy'),('post_selected','post_selected'),
                        ('post_sibling','post_sibling'),('entry_after','entry_after')]:
        for decl, row in post[actual].items():
            if not row: incomplete.append('outside declaration not observed: '+decl)
            eq('post.'+actual+'.'+decl,row,[terminal[key]]*len(row))
    if 'seed_after' in terminal and not post['outside_seed']:
        incomplete.append('outside retained seed Copy not observed')
    for decl, row in post['outside_seed'].items():
        if not row: incomplete.append('outside seed declaration not observed: '+decl)
        eq('post.outside_seed.'+decl,row,[terminal.get('seed_after',wanted)]*len(row))
    return diffs,incomplete


def main():
    if len(sys.argv)!=5:
        raise SystemExit('compare.py FIRST_ORACLE CASE ACTUAL_DECODE OUTPUT_JSON')
    oracle_path,case_id,decode_path,output_path=sys.argv[1:]
    report={'schema':'R1190_FIRST_COMPARISON_V1','case':case_id,'status':'INCOMPLETE'};rc=1
    try:
        captured=Path(decode_path).read_bytes(); decoded=json.loads(captured)
        report['saved_decode_sha256']=hashlib.sha256(captured).hexdigest()
        if decoded['status']!='ACTUAL_VALUES_DECODED':
            raise ValueError('actual decode did not complete before oracle input')
        frozen=Path(oracle_path).read_bytes()
        if hashlib.sha256(frozen).hexdigest()!=ORACLE_SHA256:
            raise ValueError('FIRST oracle byte identity changed')
        oracle=json.loads(frozen);case=next(c for c in oracle['cases'] if c['id']==case_id)
        differences,incomplete=compare(decoded['observed'],case,oracle['columns'])
        report.update(differences=differences,coverage_gaps=incomplete,oracle_sha256=ORACLE_SHA256)
        if differences: report['status']='FROZEN_ORACLE_MISMATCH';rc=42
        elif incomplete: report['status']='VALUES_MATCH_WITH_OBSERVATION_GAPS';rc=1
        else: report['status']='DECODED_FINITE_VALUES_MATCH';rc=0
        report['limits']=decoded['observed']['limits']
    except Exception as error:
        report['error']=str(error)
    Path(output_path).write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
    return rc

if __name__=='__main__':
    raise SystemExit(main())
