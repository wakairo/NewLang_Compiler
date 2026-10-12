#!/usr/bin/env bash
# R359 isolated blind first pass. No existing test/fixture/report is read.
set -u
set -o pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$(dirname "$HERE")/../.."
echo "R359 HEAD $(git rev-parse HEAD)"
echo "R359 frozen candidate: 82578ff6eb23970e3ae934c98a38383ca01468f7"
if ! git merge-base --is-ancestor 82578ff6eb23970e3ae934c98a38383ca01468f7 HEAD; then
  echo "BARRIER: candidate ancestry mismatch"; exit 90
fi
sha256sum "$HERE/"*.nl "$HERE/observer.c" "$HERE/run.sh" | tee "$HERE/hashes.txt"
python3 scripts/bootstrap.py || { echo "BARRIER: bootstrap"; exit 91; }
. .deps/activate.sh || { echo "BARRIER: toolchain activation"; exit 92; }
CC=gcc cmake --fresh -S . -B build-r359 -DCMAKE_BUILD_TYPE=Debug \
  -DNEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=OFF \
  -DNEWLANG_EXPERIMENTAL_NESTED_CALLER=OFF \
  -DNEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL=OFF \
  -DNEWLANG_EXPERIMENTAL_OWNER_AGGREGATES=OFF \
  -DNEWLANG_EXPERIMENTAL_THREE_ARG_OWNER_CALL=OFF \
  -DNEWLANG_EXPERIMENTAL_SCOPED_FIELD_CALL=OFF \
  -DNEWLANG_EXPERIMENTAL_SCOPED_PREV_FIELD_CALL=OFF \
  -DNEWLANG_EXPERIMENTAL_TWO_SCOPED_FIELD_CALLS=OFF \
  -DNEWLANG_EXPERIMENTAL_CROSS_ROOT_A_NEXT_AFTER_TWO_CALLS=OFF \
  -DNEWLANG_EXPERIMENTAL_CROSS_ROOT_C_PREV_AFTER_A_NEXT=OFF \
  -DNEWLANG_EXPERIMENTAL_FINITE_CHANGE_REPLAY=ON \
    || { echo "BARRIER: configuration"; exit 93; }
cmake --build build-r359 --parallel 2 --target newlang_semantic newlang_parser \
  || { echo "BARRIER: compile frozen semantic/parser target"; exit 94; }
cc -std=c17 -g -O0 -Wall -Wextra -Werror -Iinclude \
  "$HERE/observer.c" -Wl,--start-group \
  build-r359/libnewlang_semantic.a \
  build-r359/libnewlang_checked.a \
  build-r359/libnewlang_parser.a \
  build-r359/libnewlang_syntax.a \
  build-r359/libnewlang_lexer.a \
  build-r359/libnewlang_diagnostic.a \
  build-r359/libnewlang_source.a \
  -Wl,--end-group -o build-r359/r359_observer \
  || { echo "BARRIER: independent public C17 observer compile/link"; exit 95; }
echo "BINARY sha256 $(sha256sum build-r359/r359_observer)"
failure=0
run_case() {
  local mode="$1" file="$2" code
  echo "=== R359 $mode $file BEGIN ==="
  build-r359/r359_observer "$mode" "$HERE/$file"
  code=$?
  echo "=== R359 $mode $file exit=$code END ==="
  if [ "$code" -eq 51 ] || [ "$code" -eq 50 ]; then
    echo "FINDINGS HARD STOP: unexpected accepted mutated source or malformed owned return ($file)"
    exit 51
  fi
  if [ "$code" -ne 0 ]; then
    failure=1
  fi
}
run_case POS repeated-field.nl
run_case POS distributed-fields.nl
for file in wrong-stability.nl read-not-write.nl non-link-projection.nl \
  ptr-base-projection.nl duplicate-release.nl missing-release.nl \
  wrong-ending-domain.nl; do
  run_case NEG "$file"
done
echo "R359 all cases attempted: failed-case flag=$failure"
exit "$failure"
