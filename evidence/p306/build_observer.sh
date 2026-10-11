#!/usr/bin/env bash
set -euo pipefail
build=${1:?build directory}
mode=${2:-none}
flags=()
case "$mode" in
  address) flags=(-fsanitize=address -fno-omit-frame-pointer) ;;
  undefined) flags=(-fsanitize=undefined -fno-omit-frame-pointer) ;;
  none) ;;
  *) exit 2 ;;
esac
"${CC:-cc}" -std=c17 -g -Wall -Wextra -Werror "${flags[@]}" -Iinclude \
  evidence/p306/owned_observer.c -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc \
  -L"$build" -lnewlang_semantic -lnewlang_checked -lnewlang_parser \
  -lnewlang_diagnostic -lnewlang_syntax -lnewlang_lexer -lnewlang_source \
  -o "$build/p306_owned_observer"
