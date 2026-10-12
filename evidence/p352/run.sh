#!/usr/bin/env bash
set -euo pipefail
build=${1:-build-p352}
build=$(python3 -c 'from pathlib import Path;import sys;print(Path(sys.argv[1]).resolve())' "$build")
base_source="${build}-original-source"
base_build="${build}-frozen"
# Actual frozen ancestor, not a candidate compiler with its new flag switched off.
if [[ ! -d "$base_source" ]]; then
  git worktree add --detach "$base_source" cc1707186e1338d71fdfba26bf90134d54f5de3e
fi
test "$(git -C "$base_source" rev-parse HEAD)" = cc1707186e1338d71fdfba26bf90134d54f5de3e
(cd "$base_source" && bash evidence/p350/run.sh "$base_build")
cmake -S . -B "$build" -DCMAKE_BUILD_TYPE="${P352_BUILD_TYPE:-Debug}" \
 -DNEWLANG_SANITIZER="${P352_SANITIZER:-none}" \
 -DNEWLANG_EXPERIMENTAL_FINITE_CHANGE_REPLAY=ON
cmake --build "$build" --parallel 2 --target newlangc
python3 evidence/p352/build.py "$build"
python3 evidence/p352/observe.py "$build" "$base_build"
python3 evidence/p352/legacy_equivalence.py "$base_build" "$build/legacy-equivalence.json"
