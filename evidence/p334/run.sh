#!/usr/bin/env bash
set -euo pipefail
build=${1:-build-p334}
compiler_host=${CC:-cc}
mkdir -p "$build/observer-source"
cmake -S . -B "$build" -DCMAKE_BUILD_TYPE=Debug \
 -DNEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON -DNEWLANG_EXPERIMENTAL_NESTED_CALLER=ON \
 -DNEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL=ON -DNEWLANG_EXPERIMENTAL_OWNER_AGGREGATES=ON \
 -DNEWLANG_EXPERIMENTAL_THREE_ARG_OWNER_CALL=ON -DNEWLANG_EXPERIMENTAL_SCOPED_FIELD_CALL=ON \
 -DNEWLANG_EXPERIMENTAL_SCOPED_PREV_FIELD_CALL=ON -DNEWLANG_EXPERIMENTAL_TWO_SCOPED_FIELD_CALLS=ON \
 -DNEWLANG_EXPERIMENTAL_CROSS_ROOT_A_NEXT_AFTER_TWO_CALLS=ON \
 -DNEWLANG_EXPERIMENTAL_CROSS_ROOT_C_PREV_AFTER_A_NEXT=ON
cmake --build "$build" --parallel 2 --target newlangc five_root_source_gate_evidence newlang_checked_c_node
# Read the immutable P329 public-only baseline observer, never modify PR331.
# It validates only the existing eight-snapshot calibration/retry artifact;
# it does NOT claim a ninth-Change owned positive.
git show 46a02e60cabce1b63cc3db4bc9035fe06f5f8009:evidence/p329/public_observer.c > "$build/observer-source/original.c"
echo "6cb44a88f43c7f844f243aabca8e11ced74536d7876a8fbed0604dd2656f3619  $build/observer-source/original.c" | sha256sum --check
python3 - "$build/observer-source" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=(p/'original.c').read_text()
assert s.count('#include "../../tests/support/test.h"')==1
(p/'public_observer.c').write_text(s.replace('#include "../../tests/support/test.h"','#include "test.h"'))
PY
"$compiler_host" -g -std=c17 -Wall -Wextra -Wpedantic -Werror -Iinclude -Itests/support \
 "$build/observer-source/public_observer.c" "$build/libnewlang_semantic.a" \
 "$build/libnewlang_checked_c_node.a" "$build/libnewlang_checked.a" "$build/libnewlang_parser.a" \
 "$build/libnewlang_diagnostic.a" "$build/libnewlang_syntax.a" "$build/libnewlang_lexer.a" \
 "$build/libnewlang_source.a" -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc \
 -o "$build/calibration_observer"
python3 evidence/p334/observe.py "$build/newlangc" "$build/calibration_observer" \
 "$build/five_root_source_gate_evidence" "$build/p334-observations.json"
sha256sum "$build/newlangc" > "$build/p334-compiler-sha256.txt"
