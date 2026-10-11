#!/usr/bin/env bash
set -euo pipefail
mkdir -p build/r311
cc -std=c17 -O0 -g -Wall -Wextra -Werror -Iinclude evidence/r311/public_observer.c \
  -Wl,--start-group build/libnewlang_semantic.a build/libnewlang_checked.a \
  build/libnewlang_parser.a build/libnewlang_syntax.a build/libnewlang_lexer.a \
  build/libnewlang_source.a build/libnewlang_diagnostic.a \
  -Wl,--end-group -o build/r311/observer
for mode in fresh-none fresh-some; do
  python3 evidence/r311/generate.py "$mode" "build/r311/$mode.nl"
  sha256sum "build/r311/$mode.nl"
  set +e
  build/newlangc "build/r311/$mode.nl" >"build/r311/$mode.c" 2>"build/r311/$mode.err"
  code=$?
  set -e
  echo "R311_CLI mode=$mode exit=$code bytes=$(wc -c < build/r311/$mode.c)"
  cat "build/r311/$mode.err"
  test "$code" -eq 4
  test ! -s "build/r311/$mode.c"
  grep -q V1-BACKEND-UNSUPPORTED "build/r311/$mode.err"
  build/r311/observer "build/r311/$mode.nl" "${mode#fresh-}"
done
python3 evidence/r311/negatives.py build/newlangc build/five_root_source_gate_evidence build/r311/fresh-none.nl build/r311/
