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
        cases = ((None, 0), ('TRUTH_CHANGING', 91))
        for category, expected in cases:
            try:
                module.execute(root, cc, 'must_compile', bad, expected, 'none', mutant_kind=category)
            except SystemExit as error:
                assert 'strict C17:' in str(error)
            else:
                raise SystemExit('compile refusal incorrectly passed original/truth-changing policy')
        rows = module.execute(root, cc, 'same_truth', bad, 0, 'none', mutant_kind='SAME_TRUTH')
        assert len(rows) == 2
        for row in rows:
            assert row['category'] == 'SAME_TRUTH'
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
    print('P605 phase policy: original/changed compile required; same-truth compile refusal has native=NULL; wrong runtime rejected')


if __name__ == '__main__':
    main()
