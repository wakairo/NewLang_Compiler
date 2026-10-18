#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="${GITHUB_WORKSPACE:?}"
E="$ROOT/evidence/tests/r601-phase2"
B="$ROOT/baseline"
C="$ROOT/candidate"
OUT="${RUNNER_TEMP:?}/r601-output"
mkdir -p "$OUT"
echo "R601 evidence SHA: $(git -C "$ROOT/evidence" rev-parse HEAD)"
echo "R601 baseline SHA: $(git -C "$B" rev-parse HEAD)"
echo "R601 candidate SHA: $(git -C "$C" rev-parse HEAD)"
test "$(git -C "$B" rev-parse HEAD)" = 262f4bddb9055a6cf9067a639cbe44fa72b703a4
test "$(git -C "$C" rev-parse HEAD)" = 80f2ae3786a63235138408b35a2129a3b12afa33
test "$(git -C "$ROOT/evidence" rev-parse HEAD)" = "$GITHUB_SHA"
(cd "$ROOT/evidence" && sha256sum --check --strict tests/r601-phase2/SHA256SUMS)
bash -n "$E/run.sh"
python3 -m py_compile "$E/native.py"
gcc --version | head -1
clang --version | head -1
cmake --version | head -1
python3 --version

build_one () {
  local kind="$1" snap="$2" out="$3"
  local build="$OUT/build-$kind"
  echo "PREFLIGHT start fixed $kind $(git -C "$snap" rev-parse HEAD)"
  CC=gcc cmake -S "$snap" -B "$build" -DNEWLANG_FRONTEND_ONLY=ON -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Debug
  cmake --build "$build" --target newlangc --parallel 2
  local libraries=(
    "$build/libnewlang_checked_c_node.a"
    "$build/libnewlang_semantic.a"
    "$build/libnewlang_checked.a"
    "$build/libnewlang_parser.a"
    "$build/libnewlang_syntax.a"
    "$build/libnewlang_lexer.a"
    "$build/libnewlang_source.a"
    "$build/libnewlang_diagnostic.a"
  )
  local defs=()
  if [[ "$kind" == candidate ]]; then defs=(-DR601_CANDIDATE=1); fi
  gcc -std=c17 -O0 -Wall -Wextra -Wpedantic -Werror \
      "${defs[@]}" -I"$snap" -I"$snap/src" -I"$snap/include" \
      "$E/public_probe.c" -o "$out-public" \
      -Wl,--start-group "${libraries[@]}" -Wl,--end-group -lm
  "$out-public" --preflight "$E/add_a.nl"
  "$build/newlangc" --check "$E/add_a.nl" > "$OUT/$kind.preflight.stdout" 2> "$OUT/$kind.preflight.stderr"
  test ! -s "$OUT/$kind.preflight.stdout"
  test ! -s "$OUT/$kind.preflight.stderr"
  if [[ "$kind" == candidate ]]; then
    gcc -std=c17 -O0 -Wall -Wextra -Wpedantic -Werror \
        -I"$snap" -I"$snap/src" -I"$snap/include" \
        "$E/private_probe.c" -o "$out-private" \
        -Wl,--start-group "${libraries[@]}" -Wl,--end-group -lm
    gcc -std=c17 -O0 -Wall -Wextra -Wpedantic -Werror \
        -I"$snap" -I"$snap/src" -I"$snap/include" \
        "$E/gate_probe.c" -o "$out-gate" \
        -Wl,--start-group "${libraries[@]}" -Wl,--end-group -lm
  fi
  echo "PREFLIGHT PASS $kind: fixed checkout, hashes, strict C17 compile+link, actual registered one-test view and CHECK"
}
build_one baseline "$B" "$OUT/r601-base"
build_one candidate "$C" "$OUT/r601-cand"
echo "R601 HOSTED PREFLIGHT BOTH SHAS PASS; commence bounded tests"
"$OUT/r601-base-public" "$E/add_a.nl" "$E/add_b.nl" "$E/add_zero.nl"
"$OUT/r601-cand-public" "$E/add_a.nl" "$E/add_b.nl" "$E/add_zero.nl" \
    "$E/sub_a.nl" "$E/sub_b.nl" "$E/sub_zero.nl"
"$OUT/r601-cand-private" "$E/add_a.nl" "$E/add_b.nl" "$E/sub_a.nl" "$E/sub_b.nl"
"$OUT/r601-cand-gate" "$E/sub_a.nl"
python3 "$E/native.py" "$OUT/build-baseline/newlangc" "$OUT/build-candidate/newlangc" "$E" "$OUT/native"
echo "R601-PHASE2 ACTUAL SOURCE/PUBLIC/PRIVATE/BACKEND/NATIVE FINITE TESTS COMPLETED"
