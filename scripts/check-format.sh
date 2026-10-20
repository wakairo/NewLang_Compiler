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

# Preserve the original strict clang-format failure and generate suggestions only
# after it fails. The formatter, rather than our own formatting heuristics, owns
# every replacement byte; the source files remain unmodified.
if "$formatter" --dry-run --Werror "${files[@]}"; then
  exit 0
else
  format_status=$?
fi

echo "clang-format failed (exit ${format_status}); generating exact suggestions with $formatter" >&2
echo "Checked checkout: $(git rev-parse HEAD)" >&2
echo "clang-format version: $("$formatter" --version)" >&2

scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
candidate="$scratch/suggestions.patch"
: > "$candidate"
count=0

for file in "${files[@]}"; do
  # A git-applicable unified patch must not ambiguously encode line breaks.
  if [[ "$file" == *\n'* || "$file" == *\r'* || "$file" == *\t'* ]]; then
    echo "Unsupported C/H filename for patch: refusing ambiguous diff" >&2
    exit 1
  fi
  canonical="$scratch/canonical"
  if ! "$formatter" -- "$file" > "$canonical"; then
    echo "Formatter could not produce canonical bytes for $file" >&2
    exit 1
  fi
  if ! cmp -s -- "$file" "$canonical"; then
    if diff -u --label "a/$file" --label "b/$file" -- "$file" "$canonical" >> "$candidate"; then
      echo "Unexpected empty diff for mismatched $file" >&2
      exit 1
    else
      diff_status=$?
      if ((diff_status != 1)); then
        echo "Failed to generate exact unified diff for $file" >&2
        exit 1
      fi
    fi
    count=$((count + 1))
  fi
done

# No patch should be advertised without checking that Git can apply it to
# the unmodified checkout. Never turn a failed formatting check into PASS.
if ((count == 0)) || ! git apply --check "$candidate"; then
  echo "Formatter check failed but no verified, git-applicable patch was produced" >&2
  exit 1
fi

patch_path="${FORMAT_SUGGESTIONS_PATCH:-format-suggestions.patch}"
if [[ -L "$patch_path" ]]; then
  echo "Refusing symlink patch output: $patch_path" >&2
  exit 1
fi
cp -- "$candidate" "$patch_path"
echo "Suggested patch for $count C/H file(s): $patch_path" >&2
echo "Review the complete patch, then: git apply --check '$patch_path' && git apply '$patch_path'" >&2
echo "Alternative after review: bash scripts/format.sh" >&2
echo "----- BEGIN EXACT clang-format SUGGESTIONS -----" >&2
cat -- "$candidate" >&2
echo "----- END EXACT clang-format SUGGESTIONS -----" >&2
exit "$format_status"
