#!/usr/bin/env bash
set -euo pipefail
b=${1:-build-p365}
mapfile -t keys < <(python3 - <<'PY'
import subprocess,re
s=subprocess.check_output(['git','show','82578ff6eb23970e3ae934c98a38383ca01468f7:evidence/p338/build.sh'],text=True)
k=re.findall(r'-D(NEWLANG_EXPERIMENTAL_\w+)=ON',s);assert len(k)==10
print('\n'.join(k))
PY
)
args=(); for k in "${keys[@]}"; do args+=("-D$k=ON"); done
cmake -S . -B "$b" -DCMAKE_BUILD_TYPE="${P365_BUILD_TYPE:-Debug}" -DNEWLANG_SANITIZER="${P365_SANITIZER:-none}" "${args[@]}" -DNEWLANG_EXPERIMENTAL_FINITE_CHANGE_REPLAY=OFF
cmake --build "$b" --parallel 2 --target newlangc
python3 evidence/p365/build.py "$b"
"$b/baseline" evidence/p365/returned_owner.nl > "$b/stage0-owned.jsonl"
"$b/graph" evidence/p365/returned_owner.nl > "$b/graph.jsonl"
