# Reproduce the unchanged compiler inventory

Use Linux/glibc, GCC, Python>=3.12 and CMake>=3.28,<4, on this evidence branch
whose Compiler tree is IDENTICAL to1c6d9cab7349a7e501b04c9157f25f60df242e02.
No LLVM is required. These are local commands, not a new CI workflow. Avoid
rewriting the committed original results by placing replay output separately.
`newlangc_p534` here is the EXISTING experimental executable, never an ordinary
CLI success claim. Do not run these old target commands against a later compiler
that has removed the target without separately recording that authority change.

```sh
cmake -S . -B /tmp/p546-rebuild -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p546-rebuild --parallel 2
ctest --test-dir /tmp/p546-rebuild --output-on-failure
mkdir -p /tmp/p546-replay/inputs
cp evidence/p546/inventory.py evidence/p546/resources.py /tmp/p546-replay/
export P546_BUILD_DIR=/tmp/p546-rebuild
python3 /tmp/p546-replay/inventory.py
```

Actual registered-source public proof (original AST/source destroyed by the
existing test loader before actual `main()` check):

```sh
/tmp/p546-rebuild/p534_public_probe /tmp/p546-replay/inputs/roots1.nl
/tmp/p546-rebuild/p541_public_probe /tmp/p546-replay/inputs/roots2.nl
/tmp/p546-rebuild/p534_semantic_gate oom /tmp/p546-replay/inputs/roots16.nl
```

Measurement-only GNU link wrappers; they neither change Production nor supply
semantic/guest values. `.c.txt` keeps RAW executed evidence out of the normal
C-source formatting inventory. This probe uses genuine registered source
artifacts and public observation/validator/emitter APIs. The internal test
support loader itself is the unchanged repo helper, never `seed_root`.

```sh
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -x c \
  evidence/p546/launcher.c.txt -o /tmp/p546-replay/launcher
cc -std=c17 -Iinclude -Isrc -Dmain=newlang_driver_main \
  -c src/main.c -o /tmp/p546-replay/main.o
cc -std=c17 -x c evidence/p546/heap_wrapper.c.txt -x none \
  /tmp/p546-replay/main.o \
  /tmp/p546-rebuild/libnewlang_parser_p534.a \
  /tmp/p546-rebuild/libnewlang_checked_c_node.a \
  /tmp/p546-rebuild/libnewlang_semantic.a \
  /tmp/p546-rebuild/libnewlang_checked.a \
  /tmp/p546-rebuild/libnewlang_parser.a \
  /tmp/p546-rebuild/libnewlang_syntax.a \
  /tmp/p546-rebuild/libnewlang_lexer.a \
  /tmp/p546-rebuild/libnewlang_diagnostic.a \
  /tmp/p546-rebuild/libnewlang_source.a \
  -Wl,--wrap=malloc,--wrap=realloc,--wrap=calloc,--wrap=free \
  -o /tmp/p546-replay/heap-cli
python3 /tmp/p546-replay/resources.py
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -Iinclude -x c \
  evidence/p546/admission_probe.c.txt -x none \
  /tmp/p546-rebuild/libnewlang_parser_p534.a \
  /tmp/p546-rebuild/libnewlang_semantic.a \
  /tmp/p546-rebuild/libnewlang_checked.a \
  /tmp/p546-rebuild/libnewlang_parser.a \
  /tmp/p546-rebuild/libnewlang_syntax.a \
  /tmp/p546-rebuild/libnewlang_lexer.a \
  /tmp/p546-rebuild/libnewlang_diagnostic.a \
  /tmp/p546-rebuild/libnewlang_source.a -o /tmp/p546-replay/admission-probe
/tmp/p546-replay/admission-probe /tmp/p546-replay/inputs/array_copy.nl
```

Expected last line: records1,validated0,finalplaces10/values10/bindings2,
public_c17_status2,output_null1,length77. This is a bounded certification refusal,
not successful Array Copy emission or proof of a forged authority acceptance.

Each `rootsK-plain.c.txt` is exact unmodified source-derived C. Its separately
saved observed variant only adds passive physical observation inside the actual
source-function while original local arrays are live. To replay their physical
observations, use either host compiler, -O0 and -O2, strict C17:

```sh
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 -x c \
  evidence/p546/roots2-observed.c.txt -o /tmp/p546-replay/observed
/tmp/p546-replay/observed
```

No code here performs NewLang source element access; native observer array
indices inspect the emitted carrier only. Exact source and C hashes and original
run-specific addresses are in the retained manifest/logs. CPU and RSS vary by
host; RLIMIT_AS32MiB and malloc_usable_size are Linux/glibc-only measurement aids,
not a compiler language/resource policy or a production memory guarantee.
