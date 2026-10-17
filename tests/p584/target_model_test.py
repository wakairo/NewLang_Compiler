"""Generated C guard tests. Synthetic headers do NOT emulate a TI compiler/ABI.

All positive C originates in registered NewLang source. The scalar observer
only reads real returned carriers. Existing field/Array/native tests separately
check physical state; this test also requires all non-guard C bytes unchanged.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


GUARD = (
    '#include <limits.h>\n'
    '_Static_assert(CHAR_BIT == 8, "NewLang C17 requires 8-bit bytes");\n'
    '_Static_assert(UINT8_MAX == 255 && sizeof(uint8_t) == 1 && '
    '((uint8_t)-1) == 255, '
    '"NewLang C17 requires exact 8-bit uint8_t");\n'
)


def run(args, **kwargs):
    return subprocess.run(args, capture_output=True, text=True, timeout=40,
                          check=False, **kwargs)


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def main():
    (compiler, optin, cc, repo, sanitizer, scalar_probe, pair_probe,
     array1_probe, array2_probe, node_probe, five_probe) = sys.argv[1:]
    repo = Path(repo)
    directory = repo / 'tests/p584'
    frozen = json.loads((directory / 'frozen.json').read_text())
    baseline = {r['case']: r for r in json.loads(
        (directory / 'evidence/baseline.json').read_text())}
    for name, digest in frozen.items():
        path = repo / name if name.startswith('tests/') else directory / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == digest,
                f'frozen source changed: {name}')
    cases = [
        ('bounds', compiler, directory / 'bounds.nl', [scalar_probe]),
        ('pair', compiler, repo / 'tests/p521/combined_true.nl', [pair_probe]),
        ('array1', optin, repo / 'tests/p534/original.nl', [array1_probe]),
        ('array2', optin, repo / 'tests/p541/two_roots.nl', [array2_probe]),
        ('node', compiler, repo / 'tests/p471/asymmetric.nl',
         [node_probe, 'evidence']),
        ('five', compiler, repo / 'tests/fixtures/five_root_three_field.nl',
         [five_probe]),
    ]
    synthetic = {
        'char16_only': ('#undef CHAR_BIT\n#define CHAR_BIT 16\n',
                        'NewLang C17 requires 8-bit bytes'),
        'wide_alias_only': ('#define uint8_t uint16_t\n',
                            'NewLang C17 requires exact 8-bit uint8_t'),
        'signed_alias_only': ('#define uint8_t int8_t\n',
                              'NewLang C17 requires exact 8-bit uint8_t'),
        'wrong_max_only': ('#undef UINT8_MAX\n#define UINT8_MAX 65535\n',
                           'NewLang C17 requires exact 8-bit uint8_t'),
        'missing_max': ('#undef UINT8_MAX\n', 'UINT8_MAX'),
        'combined16': (
            '#undef CHAR_BIT\n#define CHAR_BIT 16\n'
            '#undef UINT8_MAX\n#define UINT8_MAX 65535\n'
            '#define uint8_t uint16_t\n', 'NewLang C17 requires 8-bit bytes'),
    }
    flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
    san = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}',
                                         '-fno-sanitize-recover=all']
    with tempfile.TemporaryDirectory(prefix='p584-target-') as tmp:
        root = Path(tmp)
        for name, driver, source, probe in cases:
            check = run([driver, '--check', str(source)])
            emitted = run([driver, str(source)])
            require(check.returncode == emitted.returncode == 0 and
                    not check.stdout and not check.stderr and not emitted.stderr,
                    f'{name}: source/check/C17 failed: {check} {emitted}')
            text = emitted.stdout
            before_digest = baseline[f'{name}.before.c.txt']['c17_sha256']
            without_guard = hashlib.sha256(text.replace(GUARD, '').encode()).hexdigest()
            require(text.count(GUARD) == 1 and without_guard == before_digest,
                    f'{name}: lost guard or changed non-guard guest code')
            owned = run([*probe, str(source)])
            require(owned.returncode == 0 and owned.stdout and not owned.stderr,
                    f'{name}: source-issued public observation failed: {owned}')
            path = root / f'{name}.c'
            path.write_text(text)
            # Five-root runtime also requires the existing host allocation
            # adapter; its full native test remains five_root_native.integration.
            for opt in ['-O0', '-O2']:
                output = root / f'{name}{opt}'
                link = ['-c'] if name == 'five' else []
                built = run([cc, *flags, *san, opt, *link, str(path),
                             '-o', str(output)])
                require(built.returncode == 0, f'{name}: strict C17: {built}')
                if name != 'five':
                    result = run([str(output)])
                    require(result.returncode == 0 and not result.stdout and
                            not result.stderr, f'{name}: native: {result}')
            if name == 'bounds':
                calls = list(re.finditer(
                    r'const uint8_t (nl_call_\d+) = nl_fn_\d+\([^\n]*\);', text))
                require(len(calls) == 2, 'missing actual u8 bound calls')
                observed = text.replace('#include <stdint.h>',
                                        '#include <stdint.h>\n#include <stdlib.h>')
                for match, value in zip(calls, [0, 255]):
                    observed = observed.replace(
                        match[0], match[0] + f'\n    if ({match[1]} != {value}) abort();')
                path.write_text(observed)
                for opt in ['-O0', '-O2']:
                    output = root / f'bounds_observed{opt}'
                    built = run([cc, *flags, *san, opt, str(path), '-o', str(output)])
                    require(built.returncode == 0, f'observer compile: {built}')
                    result = run([str(output)])
                    require(result.returncode == 0 and not result.stderr,
                            f'actual returned u8(0)/u8(255): {result}')
                path.write_text(text)
            refused = []
            for label, (changes, diagnostic) in synthetic.items():
                header = root / f'{label}.h'
                header.write_text('/* SYNTHETIC ONLY, NOT TI/target execution */\n'
                                  '#include <limits.h>\n#include <stdint.h>\n' + changes)
                output = root / 'negative.o'
                output.unlink(missing_ok=True)
                bad = run([cc, *flags, '-include', str(header), '-c', str(path),
                           '-o', str(output)])
                require(bad.returncode != 0 and diagnostic in bad.stderr and
                        not output.exists(), f'{name}/{label}: not fail-closed: {bad}')
                refused.append(label)
            print(json.dumps({'case': name, 'check': 0, 'public_owned': 'PASS',
                              'c17_sha256': hashlib.sha256(text.encode()).hexdigest(),
                              'unchanged_guest_C': True, 'strict_C17': ['O0', 'O2'],
                              'native': 'existing adapter test' if name == 'five'
                              else ['O0', 'O2'], 'synthetic_refused': refused}))
        for name, code, status in [
            ('range', 'V1-U8-LITERAL-RANGE', 3),
            ('type_error', 'P3-SINGLE-RESULT-REQUIRED', 3),
            ('backend_unsupported', 'V1-BACKEND-UNSUPPORTED', 4),
        ]:
            source = directory / f'{name}.nl'
            checked = run([compiler, '--check', str(source)])
            require(checked.returncode == (0 if status == 4 else 3) and
                    not checked.stdout, f'{name}: CHECK boundary changed')
            rejected = run([compiler, str(source)])
            require(rejected.returncode == status and not rejected.stdout and
                    code in rejected.stderr, f'{name}: partial output/boundary changed')
        if Path('/dev/full').exists():
            with Path('/dev/full').open('w') as sink:
                failed = subprocess.run([compiler, str(directory / 'bounds.nl')],
                                        stdout=sink, stderr=subprocess.PIPE, timeout=40)
            require(failed.returncode == 1 and b'V0-C-OUTPUT' in failed.stderr,
                    'C output error handling changed')
    print('C17 u8 target precondition PASS; synthetic negatives are NOT TI evidence')


if __name__ == '__main__':
    main()
