#!/usr/bin/env bash
set -euo pipefail
build=${1:-build-p350}
if [[ ${P350_SANITIZER:-none} == none ]]; then
  bash evidence/p338/build.sh "$build"
else
  # The immutable P338 manual observer link isn't sanitizer-aware. Run it
  # intact in a normal control build, then instrument only the new G1 binary
  # and its existing libraries using the EXISTING sanitizer option.
  bash evidence/p338/build.sh "${build}-frozen-control"
  mapfile -t options < <(python3 - "${build}-frozen-control/CMakeCache.txt" <<'PY'
import re,sys
for key in re.findall(r'^(NEWLANG_EXPERIMENTAL_\w+):BOOL=ON$',open(sys.argv[1]).read(),re.M):print('-D'+key+'=ON')
PY
)
  test "${#options[@]}" = 10
  cmake -S . -B "$build" -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER="$P350_SANITIZER" "${options[@]}"
  cmake --build "$build" --parallel 2 --target newlangc newlang_checked_c_node
fi
python3 evidence/p350/build.py "$build"
python3 evidence/p350/observe.py "$build"
