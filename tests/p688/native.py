"""Unedited producer C, versioned header, independently compiled C consumers."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(args, **kw):
    return subprocess.run(list(map(str, args)), capture_output=True, **kw)


def require(ok, why):
    if not ok:
        raise AssertionError(why)


def exercise(compiler, probe, cc, sanitizer, folder):
    folder = Path(folder)
    include = folder.parents[1] / 'include'
    freeze = json.loads((folder / 'freeze.json').read_text())
    for name, digest in freeze['files'].items():
        require(hashlib.sha256((folder / name).read_bytes()).hexdigest() == digest,
                f'changed original: {name}')
    calibration = json.loads((folder / 'calibration_freeze.json').read_text())
    require(hashlib.sha256((folder / 'rich_calibrated.nl').read_bytes()).hexdigest()
            == calibration['sha256'], 'changed calibration')
    flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
             '-I', include]
    if sanitizer != 'none':
        flags += [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all',
                  '-fno-omit-frame-pointer']
    report = {'compiler': run([cc, '--version']).stdout.decode().splitlines()[0],
              'sanitizer': sanitizer, 'positive': [], 'negative': []}

    def denied(source, selector, status, diagnostic):
        r = run([compiler, '--experimental-c17-u8-export', selector, folder/source])
        require(r.returncode == status and not r.stdout and
                diagnostic.encode() in r.stderr, (source, selector, r))
        report['negative'].append({'source': source, 'selector': selector,
                                   'exit': status, 'diagnostic': diagnostic,
                                   'stdout_bytes': 0})

    with tempfile.TemporaryDirectory(prefix='p688-') as temp:
        tmp = Path(temp)
        producers, consumers = [], []
        for name, oracle in freeze['positive'].items():
            stem = Path(name).stem
            source = folder/name
            r = run([compiler, '--check', source])
            require(r.returncode == 0 and not r.stdout and not r.stderr, r)
            ordinary = run([compiler, source])
            require(ordinary.returncode == 0 and not ordinary.stderr and
                    ordinary.stdout == (folder/'evidence'/f'baseline_{stem}.c.txt').read_bytes(),
                    'ordinary C output changed')
            owned = run([probe, source, oracle['selector']+'()', oracle['value']])
            require(owned.returncode == 0 and not owned.stderr, owned)
            evidence = json.loads(owned.stdout)
            exported = run([compiler, '--experimental-c17-u8-export',
                            oracle['selector'], source])
            require(exported.returncode == 0 and not exported.stderr, exported)
            text = exported.stdout.decode()
            require('int main(' not in text and
                    re.search(r'uint8_t newlang_c17_u8_export_v0\(void\)\s*'
                              r'\{\s*return nl_fn_'+str(evidence['function'])+r'\(\);', text),
                    'wrapper lost actual selected runtime call')
            produced = tmp/f'{stem}.c'
            produced.write_bytes(exported.stdout)
            for opt in ('-O0', '-O2'):
                po, co, exe = tmp/f'{stem}{opt}.o', tmp/f'consumer{stem}{opt}.o', tmp/f'{stem}{opt}'
                for src, obj in ((produced, po), (folder/oracle['consumer'], co)):
                    r = run([cc, *flags, opt, '-c', src, '-o', obj])
                    require(r.returncode == 0 and not r.stderr, r)
                r = run([cc, *flags, opt, po, co, '-o', exe])
                require(r.returncode == 0 and not r.stderr, r)
                r = run([exe])
                require(r.returncode == 0 and not r.stdout and not r.stderr, r)
                report['positive'].append({'source': name, 'value': oracle['value'],
                    'optimization': opt, 'separate_objects': True, 'consumer_exit': 0,
                    'owned': evidence, 'producer_sha256': hashlib.sha256(exported.stdout).hexdigest()})
                # Wrong C oracle must really fail, rather than always exit0.
                wrong = tmp/'wrong_oracle.c'
                wrong.write_text(re.sub(r'\b'+str(oracle['value'])+r'\b', '99',
                                        (folder/oracle['consumer']).read_text()))
                wrong_obj = tmp/'wrong_oracle.o'
                r = run([cc, *flags, opt, '-c', wrong, '-o', wrong_obj])
                require(r.returncode == 0, r)
                r = run([cc, *flags, opt, po, wrong_obj, '-o', exe])
                require(r.returncode == 0 and run([exe]).returncode == 1, r)
                # Normal unmodified program still has ordinary exit0.
                normal = tmp/'ordinary.c'
                normal.write_bytes(ordinary.stdout)
                r = run([cc, *flags, opt, normal, '-o', exe])
                require(r.returncode == 0 and run([exe]).returncode == 0, r)
            producers.append(po)
            consumers.append(co)
            # Internal function remains inaccessible from a distinct TU.
            private = tmp/'private.c'
            private.write_text('#include "newlang/experimental_c17_u8_export_v0.h"\n'
                f'extern uint8_t nl_fn_{evidence["function"]}(void);\n'
                f'int main(void){{return nl_fn_{evidence["function"]}();}}\n')
            private_obj = tmp/'private.o'
            require(run([cc, *flags, '-c', private, '-o', private_obj]).returncode == 0,
                    'private consumer compile')
            require(run([cc, *flags, po, private_obj, '-o', tmp/'private']).returncode != 0,
                    'internal static linked externally')
        require(run([cc, *flags, *producers, consumers[0], '-o', tmp/'collision']).returncode != 0,
                'duplicate external producer symbol accepted')
        bad = tmp/'bad_prototype.c'
        bad.write_text('#include "newlang/experimental_c17_u8_export_v0.h"\n'
                       'int newlang_c17_u8_export_v0(void);\n')
        require(run([cc, *flags, '-c', bad, '-o', tmp/'bad.o']).returncode != 0,
                'wrong prototype with header accepted')
        target = tmp/'wrong_target.c'
        target.write_text('#include <limits.h>\n#undef CHAR_BIT\n#define CHAR_BIT 16\n'
                          '#include "newlang/experimental_c17_u8_export_v0.h"\n')
        require(run([cc, *flags, '-c', target, '-o', tmp/'target.o']).returncode != 0,
                'synthetic non-8-bit-byte target admitted')
        report['link_controls'] = ['private static refused twice', 'duplicate producer refused',
                                   'wrong prototype with header refused',
                                   'synthetic CHAR_BIT16 refused; not actual C28x']
    for source, selector, status, code in [
        ('threshold.nl', 'unknown', 4, 'C17-U8-EXPORT-SELECTOR'),
        ('threshold.nl', 'main', 4, 'C17-U8-EXPORT-SELECTOR'),
        ('threshold.nl', '', 4, 'C17-U8-EXPORT-SELECTOR'),
        ('threshold.nl', 'low_threshold();main', 4, 'C17-U8-EXPORT-SELECTOR'),
        ('formal.nl', 'param', 4, 'C17-U8-EXPORT-SELECTOR'),
        ('unit_result.nl', 'no_result', 4, 'C17-U8-EXPORT-SELECTOR'),
        ('rich_calibrated.nl', 'richer', 4, 'C17-U8-EXPORT-UNSUPPORTED'),
        ('rich.nl', 'richer', 3, 'P13-IF-OPEN'),
        ('bad_return.nl', 'invalid', 3, 'P8-BODY-RESULT'),
        ('duplicate.nl', 'same', 3, 'P11-DUPLICATE-FUNCTION'),
        ('main_u8.nl', 'main', 4, 'C17-U8-EXPORT-SELECTOR'),
    ]:
        denied(source, selector, status, code)
    r = run([compiler, '--check', folder/'main_u8.nl'])
    require(r.returncode == 0 and not r.stdout, r)
    r = run([compiler, folder/'main_u8.nl'])
    require(r.returncode == 4 and not r.stdout and b'V1-BACKEND-UNSUPPORTED' in r.stderr, r)
    with open('/dev/full', 'wb', buffering=0) as full:
        r = subprocess.run([str(compiler), '--experimental-c17-u8-export',
                            'low_threshold', str(folder/'threshold.nl')],
                           stdout=full, stderr=subprocess.PIPE)
    require(r.returncode == 1 and b'V0-C-OUTPUT' in r.stderr, r)
    report['io'] = '/dev/full output failure CLI1; arbitrary I/O is not transactional'
    return report


if __name__ == '__main__':
    print(json.dumps(exercise(*sys.argv[1:]), indent=2))
