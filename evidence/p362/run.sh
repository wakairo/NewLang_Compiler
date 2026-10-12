#!/usr/bin/env bash
set -euo pipefail
base=${1:-build-p362}
mapfile -t legacy < <(python3 - <<'PY'
import re,subprocess
s=subprocess.check_output(['git','show','82578ff6eb23970e3ae934c98a38383ca01468f7:evidence/p338/build.sh'],text=True)
keys=re.findall(r'-D(NEWLANG_EXPERIMENTAL_\w+)=ON',s);assert len(keys)==10
for k in keys:print(k)
PY
)
for config in old-only both new-only; do
  args=()
  for key in "${legacy[@]}"; do
    value=ON; [[ $config != new-only ]] || value=OFF
    args+=("-D$key=$value")
  done
  new=ON; [[ $config != old-only ]] || new=OFF
  build="$base-$config"
  cmake -S . -B "$build" -DCMAKE_BUILD_TYPE="${P362_BUILD_TYPE:-Debug}" \
   -DNEWLANG_SANITIZER="${P362_SANITIZER:-none}" "${args[@]}" \
   -DNEWLANG_EXPERIMENTAL_FINITE_CHANGE_REPLAY="$new"
  cmake --build "$build" --parallel 2 --target newlangc
  python3 evidence/p362/build.py "$build"
done
python3 evidence/p362/observe.py "$base"
python3 evidence/p362/trace.py "$base-both"
python3 evidence/p362/r359.py "$base-new-only"
