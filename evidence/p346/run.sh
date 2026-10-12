#!/usr/bin/env bash
set -euo pipefail
build=${1:-build-p346}
output=${2:-"$build/p346-export"}
# Unmodified parent runner first: exact sources/ten flags and original proofs.
bash evidence/p338/build.sh "$build"
cc_host=${CC:-cc}
"$cc_host" -g -std=c17 -Wall -Wextra -Wpedantic -Werror -Iinclude -Itests/support \
 evidence/p346/export.c "$build/libnewlang_semantic.a" "$build/libnewlang_checked.a" \
 "$build/libnewlang_parser.a" "$build/libnewlang_diagnostic.a" "$build/libnewlang_syntax.a" \
 "$build/libnewlang_lexer.a" "$build/libnewlang_source.a" -o "$build/export"
python3 evidence/p346/collect.py "$build" "$output"
