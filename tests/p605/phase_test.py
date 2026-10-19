"""Test harness phase policy only, not NewLang or guest-value evidence.

Deliberate #error input exercises the compilation-refusal branch independently
of the host GCC version. Real original-guest native evidence remains P603/P596.
"""
import importlib.util
from pathlib import Path
import sys
import tempfile


def main():
    cc, helper = sys.argv[1:]
    spec = importlib.util.spec_from_file_location('p603_phase_policy', helper)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    bad = '#error P605_SYNTHETIC_COMPILER_REFUSAL\n'
    with tempfile.TemporaryDirectory(prefix='p605-phase-') as tmp:
        root = Path(tmp)
        try:
            module.execute(root, cc, 'must_compile', bad, 0, 'none')
        except SystemExit as error:
            assert 'strict C17:' in str(error)
        else:
            raise SystemExit('compile refusal incorrectly passed original policy')
        for category, expected in (('SAME_TRUTH', 0), ('TRUTH_CHANGING', 91)):
            rows = module.execute(root, cc, category, bad, expected, 'none', mutant_kind=category)
            assert len(rows) == 2
            for row in rows:
                assert row['category'] == category
                assert row['outcome'] == 'MUTANT_COMPILE_REFUSED'
                assert row['compile_exit'] != 0 and row['native_exit'] is None
                assert 'P605_SYNTHETIC_COMPILER_REFUSAL' in row['diagnostic']
        try:
            module.execute(root, cc, 'wrong_runtime', 'int main(void){return 0;}\n',
                           91, 'none', mutant_kind='TRUTH_CHANGING')
        except SystemExit as error:
            assert 'native wrong_runtime/' in str(error)
        else:
            raise SystemExit('observer runtime success mislabeled a truth-changing refusal')
        for missing in ('<=', '>', 'copy'):
            witnesses = {family: ['runtime91'] for family in ('<=', '>', 'copy')}
            witnesses[missing] = []
            try:
                module.require_runtime_falsifiers(witnesses)
            except SystemExit as error:
                assert 'missing non-boundary' in str(error)
            else:
                raise SystemExit('missing runtime family incorrectly passed')
        module.require_runtime_falsifiers({family: ['runtime91'] for family in ('<=', '>', 'copy')})
    print('P608 phase policy: original compile required; either synthetic refusal has native=NULL; wrong runtime and missing family rejected')


if __name__ == '__main__':
    main()
