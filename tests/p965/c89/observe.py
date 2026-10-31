"""C89 separately frozen genuine historical observer variant."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

build, compiler, sanitizer = sys.argv[1:]
root = Path(__file__).resolve().parents[3]
flags = ['-std=c17', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
if sanitizer != 'none':
    flags += ['-fsanitize=' + sanitizer, '-fno-omit-frame-pointer',
              '-fno-sanitize-recover=all']
libraries = ['semantic', 'parser', 'checked_c_node', 'semantic', 'parser',
             'lexer', 'checked', 'syntax', 'source', 'diagnostic']
cases = [('tests/p813/sources/charge_false.nl', 'u8'),
         ('tests/p866/sources/trim_true.nl', 'unit'),
         ('tests/p866/sources/fan_false.nl', 'unit'),
         ('tests/p878/sources/relay_equal_binding.nl', 'unit')]
with tempfile.TemporaryDirectory(prefix='p965-c89-observer-') as directory:
    probe = Path(directory) / 'observer'
    command = [compiler, *flags, '-I' + str(root / 'include'),
               '-I' + str(root / 'tests/support'), str(root / 'tests/p965/c89/observer.c'),
               *[str(Path(build) / ('libnewlang_' + name + '.a')) for name in libraries],
               '-Wl,--wrap=nl_h_custody_reserve', '-Wl,--wrap=nl_h_custody_commit',
               '-Wl,--wrap=nl_checked_destroy', '-o', str(probe)]
    print('C89 observer compiler:', compiler, 'sanitizer:', sanitizer, flush=True)
    subprocess.run(command, check=True)
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    for source, mode in cases:
        print('C89 observer source:', source, 'mode:', mode, flush=True)
        subprocess.run([str(probe), str(root / source), mode], env=environment, check=True)
