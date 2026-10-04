#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

formatter="${CLANG_FORMAT:-clang-format-23}"
if ! command -v "$formatter" >/dev/null 2>&1; then
  echo "Missing $formatter; run python3 scripts/bootstrap.py and source .deps/activate.sh" >&2
  exit 2
fi

mapfile -d '' files < <(git ls-files -z -- '*.c' '*.h')
if (("${#files[@]}" == 0)); then
  exit 0
fi

"$formatter" -i "${files[@]}"
git diff --exit-code -- "${files[@]}"
