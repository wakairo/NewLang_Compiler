"""R-owned passive C instrumentation derived from each actual Clang C AST.

Never substitutes an initializer, condition, operand, branch or store. The only
reads inserted are initialized scalar values and live original-object addresses.
The wrapper reads counters only after the guest function has returned.
"""
import json
import re


class ShapeError(RuntimeError):
    pass


def need(ok, message):
    if not ok:
        raise ShapeError(message)


def kids(n):
    return n.get('inner', [])


def walk(n):
    yield n
    for child in kids(n):
        yield from walk(child)


def offset(n, end=False):
    r = n.get('range', {}).get('end' if end else 'begin', {})
    r = r.get('expansionLoc', r)
    need('offset' in r, 'missing source offset: ' + n.get('kind', '?'))
    return r['offset'] + (r.get('tokLen', 0) if end else 0)


def core(n):
    while n.get('kind') in ('ImplicitCastExpr', 'ParenExpr', 'CStyleCastExpr'):
        need(len(kids(n)) == 1, 'cast arity')
        n = kids(n)[0]
    return n


def atom(n):
    n = core(n)
    return n.get('kind') in ('DeclRefExpr', 'IntegerLiteral', 'CXXBoolLiteralExpr')


def refname(n):
    n = core(n)
    need(n.get('kind') == 'DeclRefExpr', 'expected a live scalar carrier')
    return n['referencedDecl']['name']


def scalar(n):
    t = n.get('type', {})
    q = t.get('desugaredQualType', t.get('qualType', ''))
    return q.replace('const ', '') in ('unsigned char', 'unsigned long',
        'unsigned long long', 'unsigned int', 'int', '_Bool', 'bool',
        'uint8_t', 'size_t')


def instrument(source, tree):
    parents = {}
    for n in walk(tree):
        for child in kids(n):
            parents[id(child)] = n
    functions = [n for n in kids(tree) if n.get('kind') == 'FunctionDecl'
                 and any(c.get('kind') == 'CompoundStmt' for c in kids(n))]
    candidates = [n for n in functions if any(x.get('kind') in
                  ('ForStmt', 'WhileStmt', 'DoStmt') for x in walk(n))]
    need(len(candidates) == 1, 'expected one actual emitted loop function')
    function = candidates[0]
    body = next(c for c in kids(function) if c.get('kind') == 'CompoundStmt')
    nodes = list(walk(body))
    loops = [n for n in nodes if n.get('kind') in ('ForStmt', 'WhileStmt', 'DoStmt')]
    need(len(loops) == 1, 'expected one source loop')
    loop = loops[0]
    loop_end = offset(loop, True)
    variables = {n['name']: n for n in nodes if n.get('kind') == 'VarDecl'}
    edits = []
    metadata = {'function': function['name'], 'variables': {}, 'events': []}

    def text(n):
        return source[offset(n):offset(n, True)]

    def after(n):
        p = n
        while id(p) in parents and parents[id(p)].get('kind') not in (
                'CompoundStmt', 'LabelStmt'):
            p = parents[id(p)]
        e = offset(p, True)
        while e < len(source) and source[e].isspace():
            e += 1
        if e < len(source) and source[e] == ';':
            e += 1
        return e

    def before(n):
        p = n
        while id(p) in parents and parents[id(p)].get('kind') not in (
                'CompoundStmt', 'LabelStmt'):
            p = parents[id(p)]
        return offset(p)

    def insert(pos, code, priority=0):
        edits.append((pos, priority, '\n' + code + '\n'))

    def record(event, fields, n, pos=None, priority=0):
        tag = len(metadata['events'])
        metadata['events'].append({'tag': tag, 'kind': event,
            'offset': offset(n), **{k:v[0] for k,v in fields.items()}})
        fmt = 'R1163 ' + event + ' tag=' + str(tag)
        args = []
        for k, (label, expression, pointer) in fields.items():
            fmt += ' ' + k + '=' + ('%p' if pointer else '%llu')
            args.append('(void *)(' + expression + ')' if pointer else
                        '(unsigned long long)(' + expression + ')')
        code = 'printf(' + json.dumps(fmt + '\n')
        if args:
            code += ', ' + ', '.join(args)
        code += ');'
        insert(after(n) if pos is None else pos, code, priority)
        return tag

    def containing_statement(n):
        p = n
        while id(p) in parents and parents[id(p)].get('kind') != 'CompoundStmt':
            p = parents[id(p)]
        return p

    def result_carrier(n):
        p = n
        while id(p) in parents:
            p = parents[id(p)]
            if p.get('kind') == 'VarDecl':
                return p['name']
            if p.get('kind') == 'BinaryOperator' and p.get('opcode') == '=':
                return refname(kids(p)[0])
            if p.get('kind') in ('IfStmt', 'CompoundStmt'):
                break
        raise ShapeError('comparison does not produce a named actual bool')

    equalities = [n for n in nodes if n.get('kind') == 'BinaryOperator'
                  and n.get('opcode') == '==' and offset(n) > loop_end]
    need(len(equalities) == 1, 'one root equality required')
    equality = equalities[0]
    need(all(atom(x) for x in kids(equality)), 'root comparison passive atoms')
    boolean = result_carrier(equality)
    metadata['comparison_bool'] = boolean
    record('COMPARE', {'lhs': ('lhs', text(kids(equality)[0]), False),
                      'rhs': ('rhs', text(kids(equality)[1]), False),
                      'bool': (boolean, boolean, False)}, equality)
    insert(after(equality), '++r1163_condition_count;', -1)
    root_ifs = [n for n in nodes if n.get('kind') == 'IfStmt'
                and offset(n) > loop_end and any(x.get('kind') == 'DeclRefExpr'
                and x.get('referencedDecl', {}).get('name') == boolean
                for x in walk(kids(n)[0]))]
    need(len(root_ifs) == 1, 'root if tied to actual bool')
    root_if = root_ifs[0]
    branches = [x for x in kids(root_if) if x.get('kind') == 'CompoundStmt']
    need(len(branches) == 2, 'two explicit root branches')
    metadata['root_if_end'] = offset(root_if, True)
    for i, branch in enumerate(branches):
        insert(offset(branch)+1, f'++r1163_branch_count[{i}];\n'
               f'printf("R1163 BRANCH arm={i}\\n");')

    guards = [n for n in walk(loop) if n.get('kind') == 'BinaryOperator'
              and n.get('opcode') == '<']
    need(len(guards) == 1, 'one loop less-than guard')
    guard = guards[0]
    need(all(atom(x) for x in kids(guard)), 'loop guard passive atoms')
    guard_bool = result_carrier(guard)
    record('GUARD', {'phase': ('phase', text(kids(guard)[0]), False),
                     'bound': ('bound', text(kids(guard)[1]), False),
                     'bool': (guard_bool, guard_bool, False)}, guard)
    for n in walk(loop):
        if n.get('kind') == 'ContinueStmt':
            insert(offset(n), '++r1163_continue_count;')
        if n.get('kind') == 'BreakStmt':
            insert(offset(n), '++r1163_break_count;')
        if n.get('kind') == 'BinaryOperator' and n.get('opcode') in ('+', '-'):
            need(all(atom(x) for x in kids(n)), 'progress arithmetic passive atoms')
            result = result_carrier(n)
            record('ARITH', {'lhs': ('lhs', text(kids(n)[0]), False),
                             'rhs': ('rhs', text(kids(n)[1]), False),
                             'result': (result, result, False)}, n)

    stores = [n for n in nodes if n.get('kind') == 'BinaryOperator'
              and n.get('opcode') == '=' and core(kids(n)[0]).get('kind')
              in ('UnaryOperator', 'MemberExpr') and offset(n) > loop_end]
    need(len(stores) == 1, 'one actual normal store')
    store = stores[0]
    destination = core(kids(store)[0])
    if destination.get('kind') == 'UnaryOperator':
        need(destination.get('opcode') == '*', 'only pointer dereference store')
        pointer = refname(kids(destination)[0])
        need(pointer in variables and kids(variables[pointer]), 'actual write-ref initializer')
        address = core(kids(variables[pointer])[-1])
        need(address.get('kind') == 'UnaryOperator' and address.get('opcode') == '&',
             'write-ref is actual field address')
        member = core(kids(address)[0])
    else:
        member = destination
    need(member.get('kind') == 'MemberExpr', 'normal destination is a fixed field')
    root_name = refname(kids(member)[0])
    need(root_name in variables, 'original record local exists')
    root = variables[root_name]
    need(kids(root) and offset(root) < offset(loop), 'original root initialized before loop')
    record_types = [n for n in kids(tree) if n.get('kind') == 'RecordDecl'
                    and n.get('name') and len([f for f in kids(n)
                    if f.get('kind') == 'FieldDecl']) == 2]
    root_type = root.get('type', {}).get('desugaredQualType', root.get('type', {}).get('qualType', ''))
    records = [n for n in record_types if n['name'] in root_type.split()]
    need(len(records) == 1, 'actual two-field record type')
    names = [f['name'] for f in kids(records[0]) if f.get('kind') == 'FieldDecl']
    need(member['name'] in names, 'actual member identity')
    metadata['root'] = root_name
    metadata['fields'] = names
    metadata['store_index'] = names.index(member['name'])
    record('ROOT', {'root': (root_name, '&'+root_name, True),
                    'f0': (names[0], '&'+root_name+'.'+names[0], True),
                    'f1': (names[1], '&'+root_name+'.'+names[1], True),
                    'v0': (names[0], root_name+'.'+names[0], False),
                    'v1': (names[1], root_name+'.'+names[1], False)}, root)
    target = text(destination)
    record('STORE_BEFORE', {'address': (target, '&('+target+')', True),
                            'old': (target, target, False)}, store,
                            pos=before(store))
    record('STORE_AFTER', {'address': (target, '&('+target+')', True),
                           'current': (target, target, False)}, store)
    insert(after(store), '++r1163_store_count;', -1)

    # Every initialized scalar and ordinary scalar assignment is observed;
    # metadata retains its actual C expression and source position for auditing.
    for n in nodes:
        if n.get('kind') == 'VarDecl' and scalar(n) and kids(n):
            name = n['name']
            init = kids(n)[-1]
            metadata['variables'][name] = {'offset': offset(n), 'init': text(init)}
            record('SCALAR', {'value': (name, name, False)}, n)
        if n.get('kind') == 'BinaryOperator' and n.get('opcode') == '=':
            left = core(kids(n)[0])
            if left.get('kind') == 'DeclRefExpr' and scalar(left):
                name = refname(left)
                record('ASSIGN', {'value': (name, name, False)}, n)
        if n.get('kind') == 'VarDecl' and scalar(n) and kids(n) and offset(n) > offset(root_if, True):
            init = core(kids(n)[-1])
            if init.get('kind') == 'MemberExpr':
                base = refname(kids(init)[0])
                need(base == root_name, 'postread retains original root')
                need(init['name'] in names, 'postread member identity')
                expression = text(init)
                record('POSTREAD', {'address': (expression, '&('+expression+')', True),
                                    'value': (n['name'], n['name'], False),
                                    'index': ('index', str(names.index(init['name'])), False)}, n)
                insert(after(n), '++r1163_postread_count;', -1)
        if n.get('kind') == 'ReturnStmt' and offset(root_if) < offset(n) < offset(root_if, True):
            insert(offset(n), '++r1163_return_count;')

    # Instrumented standalone entry wrapper never reads an ended object.
    mains = [n for n in functions if n.get('name') == 'main']
    need(len(mains) == 1, 'one emitted main')
    main = mains[0]
    loc = main.get('loc', {})
    need('offset' in loc and source[loc['offset']:loc['offset']+4] == 'main', 'main name range')
    name_offset = loc['offset']
    edits.append((name_offset, 100, 'r1163_guest_'))
    prefix = '''#include <stdio.h>
static unsigned r1163_condition_count, r1163_branch_count[2];
static unsigned r1163_continue_count, r1163_break_count, r1163_store_count;
static unsigned r1163_postread_count, r1163_return_count;
'''
    suffix = '''
int main(void) {
  int result = r1163_guest_main();
  printf("R1163 COUNTERS condition=%u then=%u else=%u continue=%u break=%u store=%u postread=%u returns=%u\\n", r1163_condition_count, r1163_branch_count[0], r1163_branch_count[1], r1163_continue_count, r1163_break_count, r1163_store_count, r1163_postread_count, r1163_return_count);
  return result;
}
'''
    for pos, _, code in sorted(edits, key=lambda e:(e[0], e[1]), reverse=True):
        source = source[:pos] + code + source[pos:]
    return prefix+source+suffix, metadata


def parse_trace(output):
    result=[]
    for line in output.splitlines():
        if not line.startswith('R1163 '):
            raise ShapeError('unexpected guest output: '+line)
        parts=line.split();row={'kind':parts[1]}
        for item in parts[2:]:
            key,value=item.split('=',1)
            row[key]=value if value.startswith('0x') or value=='(nil)' else int(value)
        result.append(row)
    return result


def validate(trace, meta, case):
    def rows(kind):return [r for r in trace if r['kind']==kind]
    need(len(rows('ROOT'))==1,'one dynamic original root')
    root=rows('ROOT')[0]
    need([root['v0'],root['v1']]==case['initial'],'initial original values')
    need(root['f0']!=root['f1'],'distinct fixed child addresses')
    need([r['phase'] for r in rows('GUARD')]==case['trace'],'actual phase trace')
    bound = case['bound'] if case['op']=='+' else case['bound']+1
    for guard in rows('GUARD'):
        need(guard['bound']==bound and guard['bool']==int(guard['phase']<bound),
             'actual guard bound and ordinary Bool')
    need(len(rows('ARITH'))==case['backedges'],'actual arithmetic count')
    for i,r in enumerate(rows('ARITH')):
        need(r['lhs']==case['trace'][i] and r['rhs']==1 and
             r['result']==case['trace'][i+1],'actual arithmetic operands/result')
    need(len(rows('COMPARE'))==1,'condition exactly once')
    compare=rows('COMPARE')[0]
    need(compare['lhs']==case['result'] and compare['rhs']==case['compare'] and
         compare['bool']==int(case['condition']),'actual root operands/bool')
    need([r['arm'] for r in rows('BRANCH')]==[case['selected_arm']],'only chosen arm')
    need(len(rows('COUNTERS'))==1,'post-return counters only')
    counts=rows('COUNTERS')[0]
    need(counts['condition']==1 and counts['then']+counts['else']==1,'condition/branch counters')
    need(counts['continue']==case['backedges'] and counts['break']==1,'loop edge counts')
    need(meta['store_index']==case['index'],'original declaration-index mapping')
    need(counts['returns']==int(case['early']),'function RETURN arm execution count')
    if case['early']:
        need(not rows('STORE_BEFORE') and not rows('STORE_AFTER') and not rows('POSTREAD'),
             'early RETURN must not execute store/postread')
        need(counts['store']==0 and counts['postread']==0,'early counters')
        # No scalar declaration/assignment after the root IF executes on early exit.
        for r in trace:
            if r['kind'] in ('SCALAR','ASSIGN'):
                event=meta['events'][r['tag']]
                need(event['offset']<meta['root_if_end'],'no IF-result/postbinding use after early RETURN')
    else:
        need(len(rows('STORE_BEFORE'))==len(rows('STORE_AFTER'))==1,'one normal replacement')
        old=rows('STORE_BEFORE')[0];new=rows('STORE_AFTER')[0]
        address=root['f'+str(case['index'])]
        need(old['address']==new['address']==address,'same original child address')
        need(old['old']==case['expected_old'] and new['current']==case['expected_current'],
             'actual old and current values')
        reads=rows('POSTREAD')
        need(len(reads)==2,'current and sibling postreads')
        expected={case['index']:case['expected_current'],1-case['index']:case['expected_sibling']}
        for r in reads:
            need(r['address']==root['f'+str(r['index'])] and r['value']==expected[r['index']],
                 'same original postread address/value')
        need(counts['store']==1 and counts['postread']==2,'normal event counts')
        # At least one post-IF ordinary u8 Copy keeps the old scalar. Its actual
        # initialized carrier is read; no expected value enters the guest.
        post=[r for r in rows('SCALAR') if meta['events'][r['tag']]['offset']>meta['root_if_end']]
        need(any(r['value']==case['expected_old'] for r in post),'old Copy survives normal join')
    return {'phase_trace':[r['phase'] for r in rows('GUARD')], 'comparison':compare,
            'counters':counts, 'root':root, 'store_before':rows('STORE_BEFORE'),
            'store_after':rows('STORE_AFTER'), 'postreads':rows('POSTREAD')}


def self_test():
    # Pure-text own mock: no compiler, P output or runtime is involved.
    n={'kind':'ImplicitCastExpr','inner':[{'kind':'ParenExpr','inner':[
        {'kind':'DeclRefExpr','referencedDecl':{'name':'own_mock'}}]}]}
    assert atom(n) and refname(n)=='own_mock'
    assert not atom({'kind':'CallExpr','inner':[]})
    assert scalar({'type':{'desugaredQualType':'unsigned char'}})
    assert not scalar({'type':{'desugaredQualType':'unsigned char *'}})
    assert parse_trace('R1163 COMPARE lhs=9 rhs=10 bool=0\n')[0]['bool']==0
    try:need(False,'deliberate own mock')
    except ShapeError:pass
    else:raise AssertionError('fail closed')


if __name__=='__main__':
    self_test()
    print('R1163 passive AST machinery pure-text own-mock PASS')
