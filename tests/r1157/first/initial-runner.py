"""R1157 independent finite source/owned/C17 observations; Actions execution only."""
import base64
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import shlex
import subprocess
import sys
import tarfile
import traceback

CLI, REPO, BUILD, CC, SAN = sys.argv[1:]
REPO, BUILD = Path(REPO), Path(BUILD)
R1157_DIR = Path(__file__).resolve().parent
OUT = BUILD / 'r1157-raw'
OUT.mkdir(exist_ok=True)
ORACLE = json.loads((R1157_DIR / 'v1/oracle.json').read_text())
RESULT = {'version': 'R1157-machine-V2', 'cases': {}, 'negatives': {}, 'commands': [],
          'checkout': os.environ.get('GITHUB_SHA'), 'run': os.environ.get('GITHUB_RUN_ID'),
          'attempt': os.environ.get('GITHUB_RUN_ATTEMPT'), 'sanitizer': SAN,
          'first': True, 'independence': 'V1 oracle frozen before interface connection'}


def run(tag, command, allowed=(0,), timeout=45):
    prefix = OUT / tag
    prefix.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run([str(x) for x in command], capture_output=True, timeout=timeout)
    prefix.with_suffix('.stdout').write_bytes(proc.stdout)
    prefix.with_suffix('.stderr').write_bytes(proc.stderr)
    item = {'tag': tag, 'command': [str(x) for x in command], 'status': proc.returncode}
    RESULT['commands'].append(item)
    (OUT / 'commands.json').write_text(json.dumps(RESULT['commands'], indent=2) + '\n')
    if allowed is not None and proc.returncode not in allowed:
        raise RuntimeError(f'{tag}: exit {proc.returncode}; {proc.stderr[:600]!r}')
    return proc


PROBE = r'''#include "newlang/parser.h"
#include "newlang/if_field.h"
#include <stdio.h>
#include <string.h>

static void places(const char *path, const char *phase, const NLSemanticContext *w,
                   const NLIfFieldView *v)
{
    const size_t ids[3] = {v->root, v->child, v->sibling};
    for (size_t k = 0; k < 3; ++k) {
        NLSemanticPlaceView p;
        if (!nl_semantic_place_view(w, ids[k], &p)) continue;
        printf("{\"kind\":\"place\",\"path\":\"%s\",\"phase\":\"%s\",\"role\":%zu,\"place\":%zu,\"inc\":%zu,\"fact\":%zu,\"value\":%zu,\"parent\":%zu,\"index\":%zu,\"live\":%d}\n",
               path, phase, k, ids[k], p.incarnation, p.current_fact,
               p.current_value, p.parent_aggregate, p.parent_field_index, p.live);
    }
}
static int walk(const NLCheckedFragment *f, const char *path, unsigned depth)
{
    if (depth > 8) return 1;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (!n) continue;
        NLSourceView name = {0};
        const NLSource *source = nl_checked_source(f);
        if (!nl_source_view(source, n->name, &name)) name.length = 0;
        if (name.length > 60) name.length = 0;
        printf("{\"kind\":\"node\",\"path\":\"%s\",\"id\":%zu,\"nodekind\":%d,\"name\":\"%.*s\",\"symbol\":%zu,\"type\":%zu,\"initializer\":%zu,\"copied\":%d,\"field\":%d,\"root\":%zu,\"child\":%zu,\"index\":%zu,\"old\":%zu,\"new\":%zu,\"inductive\":%d,\"backedges\":%zu,\"breaks\":%zu}\n",
               path, i, (int)n->kind, (int)name.length, name.bytes ? (const char *)name.bytes : "",
               n->symbol, n->type, n->initializer, n->value_use == NL_VALUE_COPIED,
               n->field.present, n->field.parent, n->field.child, n->field.index,
               n->field.old_value, n->field.new_value, n->header_inductive,
               n->continue_edges, n->break_edges);
        if (n->kind == NL_CHECKED_IF) {
            NLIfFieldView v;
            if (nl_checked_if_field_view(f, i, &v)) {
                NLCheckStatus valid = nl_checked_if_field_validate(f, i, NULL);
                printf("{\"kind\":\"ifview\",\"path\":\"%s\",\"id\":%zu,\"validation\":%d,\"root\":%zu,\"child\":%zu,\"sibling\":%zu,\"index\":%zu,\"inc\":%zu,\"child_inc\":%zu,\"base\":%zu,\"changes0\":%zu,\"changes1\":%zu}\n",
                       path, i, (int)valid, v.root, v.child, v.sibling, v.index,
                       v.incarnation, v.child_incarnation, v.base,
                       v.arms[0].change_count, v.arms[1].change_count);
                if (valid != NL_CHECK_OK) return 2;
                places(path, "entry", v.entry, &v);
                places(path, "post", v.post, &v);
                for (size_t a = 0; a < 2; ++a) {
                    for (size_t e = 0; e < v.arms[a].event_count; ++e) {
                        NLIfFieldEvent event;
                        if (!nl_checked_if_field_event(f, i, a, e, &event)) return 3;
                        NLCheckStatus ok = nl_checked_if_field_event_validate(f, i, a, e, NULL);
                        printf("{\"kind\":\"ifevent\",\"arm\":%zu,\"event\":%zu,\"change\":%d,\"validation\":%d,\"root\":%zu,\"child\":%zu,\"index\":%zu,\"old\":%zu,\"incoming\":%zu}\n",
                               a, e, event.change, (int)ok, event.field.parent,
                               event.field.child, event.field.index, event.field.old_value, event.incoming);
                        if (ok != NL_CHECK_OK) return 4;
                    }
                }
            }
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *arm = nl_checked_if_arm(f, i, a);
                if (arm) { char next[160]; snprintf(next, sizeof next, "%s/i%zu.%zu", path, i, a);
                    int status = walk(arm, next, depth + 1); if (status) return status; }
            }
        }
        if (n->kind == NL_CHECKED_LOOP) {
            const NLCheckedFragment *body = nl_checked_loop_body(f, i);
            if (body) { char next[160]; snprintf(next, sizeof next, "%s/l%zu", path, i);
                int status = walk(body, next, depth + 1); if (status) return status; }
        }
        if (n->kind == NL_CHECKED_REGISTERED_CALL && n->body_backed) {
            const NLCheckedFragment *body = nl_checked_call_body(f, i);
            if (body) { char next[160]; snprintf(next, sizeof next, "%s/c%zu", path, i);
                int status = walk(body, next, depth + 1); if (status) return status; }
        }
    }
    return 0;
}
int main(int argc, char **argv)
{
    NLSource *source = NULL, *call_source = NULL;
    NLParser *parser = NULL, *call_parser = NULL;
    NLSyntaxTree *tree = NULL, *call_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic parse = {0}; NLFunctionUnitDiagnostic unit = {0};
    NLCheckDiagnostic diagnostic = {0};
    int result = 10;
    if (argc != 2 || nl_source_load(argv[1], &source) != NL_SOURCE_OK) goto cleanup;
    if (nl_parser_create(source, &parser) != NL_PARSE_OK) goto cleanup;
    NLParseStatus ps = nl_parser_parse_function_unit(parser, &tree, &parse);
    printf("{\"kind\":\"stage\",\"stage\":\"parse\",\"status\":%d}\n", (int)ps);
    if (ps != NL_PARSE_OK) { nl_parse_diagnostic_render(stderr, source, &parse); goto cleanup; }
    if (nl_semantic_create(&context) != NL_CHECK_OK) goto cleanup;
    const NLSyntaxTree *inputs[] = {tree};
    NLCheckStatus cs = nl_semantic_register_function_unit(context, inputs, 1, &unit);
    printf("{\"kind\":\"stage\",\"stage\":\"register\",\"status\":%d}\n", (int)cs);
    if (cs != NL_CHECK_OK) { nl_check_diagnostic_render(stderr, source, &unit.diagnostic); goto cleanup; }
    const char call[] = "specimen()";
    if (nl_source_create(call, strlen(call), "<R1157-own-call>", &call_source) != NL_SOURCE_OK) goto cleanup;
    if (nl_parser_create(call_source, &call_parser) != NL_PARSE_OK) goto cleanup;
    if (nl_parser_parse_source_fragment(call_parser, &call_tree, &parse) != NL_PARSE_OK) goto cleanup;
    cs = nl_semantic_check_source_fragment(context, call_tree, &checked, &diagnostic);
    printf("{\"kind\":\"stage\",\"stage\":\"actual_call\",\"status\":%d}\n", (int)cs);
    if (cs != NL_CHECK_OK) { nl_check_diagnostic_render(stderr, call_source, &diagnostic); goto cleanup; }
    result = walk(checked, "actual", 0);
cleanup:
    nl_checked_destroy(checked); nl_semantic_destroy(context);
    nl_syntax_tree_destroy(call_tree); nl_syntax_tree_destroy(tree);
    nl_parser_destroy(call_parser); nl_parser_destroy(parser);
    nl_source_destroy(call_source); nl_source_destroy(source);
    return result;
}
'''

OBSERVER = r'''
#include <stdio.h>
static size_t r1157_operand(unsigned c, unsigned side, size_t v)
{ fprintf(stderr, "R1157 OP %u %u %zu\n", c, side, v); return v; }
static _Bool r1157_condition(unsigned c, _Bool v, const void *root,
                             const void *f0, const void *f1,
                             unsigned v0, unsigned v1)
{ fprintf(stderr, "R1157 COND %u %u %p %p %p %u %u\n", c, (unsigned)v,
          root, f0, f1, v0, v1); return v; }
'''


def close_paren(text, start, opener='(', closer=')'):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == opener:
            depth += 1
        elif text[i] == closer:
            depth -= 1
            if depth == 0:
                return i
    raise RuntimeError('observer unmatched delimiter')


def comparison(expression, cid):
    value = expression.strip()
    while value.startswith('(') and close_paren(value, 0) == len(value) - 1:
        value = value[1:-1].strip()
    found = list(re.finditer(r'==|(?<![<>=])<(?![=<>])', value))
    if len(found) != 1:
        raise RuntimeError(f'observer needs actual comparison operands: {expression}')
    item = found[0]
    left, right = value[:item.start()].strip(), value[item.end():].strip()
    return f'r1157_operand({cid}, 0, ({left})) {item.group()} r1157_operand({cid}, 1, ({right}))'


def observe(c_source, nodes, case):
    mapping = {}
    for n in nodes:
        if n.get('kind') == 'node' and n.get('symbol') and n.get('initializer') and n.get('name') in ('capsule', 'before', 'settled', 'echoed', 'displaced', 'after', 'still'):
            mapping.setdefault(n['name'], f"nl_local_{n['symbol']}")
    if set(mapping) != {'capsule', 'before', 'settled', 'echoed', 'displaced', 'after', 'still'}:
        raise RuntimeError(f'observer source-issued binding map incomplete: {mapping}')
    root = mapping['capsule']
    edits = []
    condition_count = 0
    for match in re.finditer(r'\bif\s*\(', c_source):
        start = c_source.index('(', match.start())
        end = close_paren(c_source, start)
        brace = re.match(r'\s*\{', c_source[end + 1:])
        if not brace:
            raise RuntimeError('observer requires explicit emitted IF braces')
        cid = condition_count
        condition_count += 1
        expr = comparison(c_source[start + 1:end], cid)
        replacement = (f'r1157_condition({cid}, ({expr}), &{root}, &{root}.f0, '
                       f'&{root}.f1, {root}.f0, {root}.f1)')
        edits.append((start + 1, end, replacement))
        then_open = end + 1 + brace.end() - 1
        then_close = close_paren(c_source, then_open, '{', '}')
        else_match = re.match(r'\s*else\s*\{', c_source[then_close + 1:])
        if not else_match:
            raise RuntimeError('observer requires both emitted arms')
        else_open = then_close + else_match.end()
        edits.append((then_open + 1, then_open + 1, f'\nfprintf(stderr, "R1157 ARM {cid} then\\n");'))
        edits.append((else_open + 1, else_open + 1, f'\nfprintf(stderr, "R1157 ARM {cid} else\\n");'))
    if condition_count != 2:
        raise RuntimeError(f'observer IF schema: expected 2, saw {condition_count}')
    for name, symbol in mapping.items():
        if name == 'capsule':
            continue
        pat = re.compile(r'(?m)^[ \t]*(?:const\s+)?(?:uint8_t|size_t|_Bool)\s+' + re.escape(symbol) + r'\s*=[^;]*;')
        hits = list(pat.finditer(c_source))
        if len(hits) != 1:
            raise RuntimeError(f'observer declaration schema {name}: {len(hits)}')
        end = hits[0].end()
        edits.append((end, end, f'\nfprintf(stderr, "R1157 VAL {name} %zu\\n", (size_t){symbol});'))
    # Actual emitted store LHS is observed; neither target nor value is supplied.
    stores = list(re.finditer(r'(?m)^[ \t]*(?P<lhs>' + re.escape(root) + r'\.f[01]|\*nl_local_\d+)\s*=\s*(?P<rhs>[^;]+);', c_source))
    if len(stores) != 2:
        raise RuntimeError(f'observer store schema: {len(stores)}')
    for item in stores:
        lhs = item.group('lhs')
        pointer = f'&{lhs}' if not lhs.startswith('*') else lhs[1:]
        before = f'fprintf(stderr, "R1157 STORE before %p %u\\n", (void *)({pointer}), (unsigned)({lhs}));\n'
        after = f'\nfprintf(stderr, "R1157 STORE after %p %u\\n", (void *)({pointer}), (unsigned)({lhs}));'
        edits.append((item.start(), item.start(), before))
        edits.append((item.end(), item.end(), after))
    for start, end, replacement in sorted(edits, reverse=True):
        c_source = c_source[:start] + replacement + c_source[end:]
    # Includes stay ahead of helper declarations so size_t is the genuine C type.
    includes = list(re.finditer(r'(?m)^#include[^\n]*\n', c_source))
    if not includes:
        raise RuntimeError('observer generated includes absent')
    pos = includes[-1].end()
    return c_source[:pos] + OBSERVER + c_source[pos:]


def assess(text, case):
    lines = text.splitlines()
    conditions = [x.split() for x in lines if x.startswith('R1157 COND ')]
    guards = [x for x in conditions if x[2] == '0']
    choice = [x for x in conditions if x[2] == '1']
    assert [int(x[3]) for x in guards] == [int(x) for x in case['guard']], guards
    assert len(choice) == 1 and int(choice[0][3]) == int(case['truth']), choice
    addresses = {tuple(x[4:7]) for x in conditions}
    assert len(addresses) == 1, addresses
    root, f0, f1 = next(iter(addresses))
    assert f0 != f1 and root == f0, addresses
    assert all([int(x[7]), int(x[8])] == [case['ember'], case['reserve']] for x in conditions)
    ops = [x.split() for x in lines if x.startswith('R1157 OP ')]
    assert [int(x[4]) for x in ops if x[2:4] == ['0', '0']] == case['values'], ops
    assert [int(x[4]) for x in ops if x[2:4] == ['1', '0']] == [case['left']], ops
    assert [int(x[4]) for x in ops if x[2:4] == ['1', '1']] == [case['right']], ops
    arms = [x.split()[3] for x in lines if x.startswith('R1157 ARM 1 ')]
    assert arms == [case['selected_arm']], arms
    stores = [x.split() for x in lines if x.startswith('R1157 STORE ')]
    assert len(stores) == 2 and stores[0][2] == 'before' and stores[1][2] == 'after', stores
    target = [f0, f1][case['index']]
    assert stores[0][3] == stores[1][3] == target, stores
    assert [int(x[4]) for x in stores] == [case['old'], case['new']], stores
    vals = {x.split()[2]: int(x.split()[3]) for x in lines if x.startswith('R1157 VAL ')}
    wanted = {k: case[k] for k in ('before', 'displaced', 'after', 'still')}
    wanted.update(settled=case['loop_result'], echoed=case['copy_result'])
    assert vals == wanted, (vals, wanted)
    return {'conditions': len(choice), 'loop_headers': len(guards), 'stores': 1,
            'selected': arms[0], 'root': root, 'f0': f0, 'f1': f1, 'values': vals}


def public_assess(nodes, case):
    views = [x for x in nodes if x.get('kind') == 'ifview']
    if len(views) != 1:
        raise RuntimeError(f'public owned IF-field view count {len(views)}')
    view = views[0]
    assert view['validation'] == 0 and view['index'] == case['index']
    assert view['changes0'] == view['changes1'] == 1
    places = [x for x in nodes if x.get('kind') == 'place' and x['path'] == view['path']]
    for role in range(3):
        a = next(x for x in places if x['role'] == role and x['phase'] == 'entry')
        b = next(x for x in places if x['role'] == role and x['phase'] == 'post')
        assert a['live'] and b['live'] and a['place'] == b['place'] and a['inc'] == b['inc']
        if role == 2:
            assert a['fact'] == b['fact'] and a['value'] == b['value']
        else:
            assert a['fact'] != b['fact']
    return view


def main():
    shutil.copytree(R1157_DIR / 'v1', OUT / 'frozen-v1', dirs_exist_ok=True)
    shutil.copytree(R1157_DIR / 'v2', OUT / 'frozen-v2', dirs_exist_ok=True)
    for item in json.loads((R1157_DIR / 'v1/manifest.json').read_text())['files']:
        assert hashlib.sha256((R1157_DIR / 'v1' / item['path']).read_bytes()).hexdigest() == item['sha256']
    run('checkout-head', ['git', 'rev-parse', 'HEAD'])
    run('checkout-parents', ['git', 'show', '-s', '--format=%H %P %T', 'HEAD'])
    run('compiler-version', [CLI, '--version'])
    # Negatives never receive a fabricated later-layer success.
    for negative in ORACLE['negatives']:
        name = negative['id']
        p = run(f'negative/{name}', [CLI, '--check', R1157_DIR / 'v2/negative' / f'{name}.nl'], allowed=None)
        diagnostic = p.stderr.decode(errors='replace')
        hit = re.search(r'error\((parser|semantic|cli)\)\[([^]]+)\]', diagnostic)
        RESULT['negatives'][name] = {'status': p.returncode, 'actual_layer': hit.group(1) if hit else 'unclassified',
            'diagnostic': hit.group(2) if hit else None, 'owned': 'NOT_REACHED', 'C17': 'NOT_REACHED', 'native': 'NOT_REACHED'}
        if p.returncode not in (2, 3, 4) or not hit:
            raise RuntimeError(f'STOP negative {name}: expected structured rejection')
    checks = []
    for case in ORACLE['positives']:
        name = case['id']
        p = run(f'{name}/check', [CLI, '--check', R1157_DIR / 'v2/sources' / f'{name}.nl'], allowed=None)
        RESULT['cases'][name] = {'check_status': p.returncode, 'owned': 'NOT_REACHED', 'C17': 'NOT_REACHED', 'native': 'NOT_REACHED'}
        checks.append((name, p.returncode, p.stderr.decode(errors='replace')))
    if any(rc or err for _, rc, err in checks):
        RESULT['first_failure'] = {'layer': 'source_CHECK', 'checks': checks}
        raise RuntimeError('affected STOP: positive CHECK differs from frozen acceptance; later layers NOT_REACHED')
    probe_c, probe = OUT / 'public-probe.c', OUT / 'public-probe'
    probe_c.write_text(PROBE)
    flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I', REPO / 'include']
    hostflags = [f'-fsanitize={SAN}', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'] if SAN != 'none' else []
    libraries = [BUILD / f'libnewlang_{name}.a' for name in ('semantic','checked','parser','syntax','lexer','source','diagnostic')]
    run('public-probe-build', [CC, *flags, *shlex.split(os.environ.get('CFLAGS', '')), *hostflags, probe_c, '-Wl,--start-group', *libraries, '-Wl,--end-group', '-o', probe])
    gcc = shutil.which('gcc-13') or shutil.which('gcc')
    clang = shutil.which('clang-23') or shutil.which('clang')
    if not gcc or not clang:
        raise RuntimeError('required guest GCC/Clang unavailable')
    for label, compiler in [('gcc',gcc),('clang',clang)]:
        run(f'{label}-version', [compiler, '--version'])
    configs = [('gccO0',gcc,['-O0']),('gccO2',gcc,['-O2']),('clangO0',clang,['-O0']),('clangO2',clang,['-O2']),
               ('guestASan',clang,['-O1','-fsanitize=address','-fno-omit-frame-pointer','-fno-sanitize-recover=all']),
               ('guestUBSan',clang,['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'])]
    for case in ORACLE['positives']:
        name = case['id']; directory = OUT / name; directory.mkdir(exist_ok=True)
        original = R1157_DIR / 'v1/sources' / f'{name}.nl'
        adapted = R1157_DIR / 'v2/sources' / f'{name}.nl'
        p = run(f'{name}/public', [probe, original])
        nodes = [json.loads(x) for x in p.stdout.decode().splitlines()]
        RESULT['cases'][name]['owned'] = public_assess(nodes, case)
        raw = run(f'{name}/emit', [CLI, adapted]).stdout
        rawpath = directory / 'original.c'; rawpath.write_bytes(raw)
        named = directory / 'named.c'
        run(f'{name}/named-emit', [CLI, '--emit-c17-to', named, adapted])
        assert named.read_bytes() == raw, 'ordinary stdout/named emission differ'
        export = run(f'{name}/export-emit', [CLI, '--experimental-c17-u8-export', 'specimen', original]).stdout
        exportpath = directory / 'export.c'; exportpath.write_bytes(export)
        driver = directory / 'export-driver.c'
        driver.write_text('#include "newlang/experimental_c17_u8_export_v0.h"\n#include <stdio.h>\nint main(void) { printf("RESULT %u\\n", (unsigned)newlang_c17_u8_export_v0()); return 0; }\n')
        observed = directory / 'observed.c'
        observed.write_text(observe(raw.decode(), nodes, case))
        RESULT['cases'][name]['C17'] = {'original_sha256': hashlib.sha256(raw).hexdigest(), 'named_identical': True,
            'export_sha256': hashlib.sha256(export).hexdigest(), 'observer_sha256': hashlib.sha256(observed.read_bytes()).hexdigest()}
        measurements = []
        for config, compiler, options in configs:
            for mode, source in [('original',rawpath),('observed',observed),('export',exportpath)]:
                binary = directory / f'{config}-{mode}'
                sources = [source,driver] if mode == 'export' else [source]
                run(f'{name}/{config}-{mode}-build', [compiler,*flags,*(['-Wno-error=gcc-install-dir-libstdcxx'] if Path(compiler).name == 'clang-23' else []),*options,*sources,'-o',binary])
                p = run(f'{name}/{config}-{mode}-run', [binary])
                if mode == 'observed':
                    measurements.append({'config':config,'passive':assess(p.stderr.decode(),case)})
                else:
                    assert not p.stderr, f'{name} {config} {mode} stderr {p.stderr!r}'
                    if mode == 'export':
                        assert p.stdout == f"RESULT {case['return']}\n".encode(), p.stdout
            RESULT['cases'][name]['native'] = measurements
    RESULT['outcome'] = 'FINITE_PASS'


def archive():
    (OUT / 'results.json').write_text(json.dumps(RESULT, indent=2) + '\n')
    capsule = io.BytesIO()
    with tarfile.open(fileobj=capsule,mode='w:gz') as tar:
        for path in sorted(OUT.rglob('*')):
            if path.is_file() and not os.access(path, os.X_OK):
                tar.add(path, arcname=str(path.relative_to(OUT)))
    data = capsule.getvalue()
    digest = hashlib.sha256(data).hexdigest()
    archivepath = BUILD / 'r1157-raw.tar.gz'; archivepath.write_bytes(data)
    encoded = base64.b64encode(data).decode()
    # CTest captures child stdout on success; duplicate only our evidence capsule
    # to its parent's existing job-log stream. No credentials or foreign files.
    stream = None
    try:
        stream = open(f'/proc/{os.getppid()}/fd/1','w')
    except OSError:
        stream = sys.stdout
    print(f'R1157_RAW_BEGIN sha256={digest} bytes={len(data)}', file=stream, flush=True)
    for offset in range(0,len(encoded),6000):
        print('R1157_RAW '+encoded[offset:offset+6000],file=stream,flush=True)
    print('R1157_RAW_END',file=stream,flush=True)
    print('R1157_RESULT '+json.dumps({'outcome':RESULT.get('outcome'),'first_failure':RESULT.get('first_failure'),'sha256':digest}),file=stream,flush=True)
    if stream is not sys.stdout:
        stream.close()


status=0
try:
    main()
except Exception as exc:
    status=1
    RESULT['outcome']='FIRST_RED'
    RESULT.setdefault('first_failure',{'layer':'unclassified; inspect raw command/evidence before repair','error':str(exc)})
    (OUT/'traceback.txt').write_text(traceback.format_exc())
    print(traceback.format_exc(),flush=True)
finally:
    archive()
sys.exit(status)
