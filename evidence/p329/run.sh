#!/usr/bin/env bash
set -euo pipefail
build=${1:-build-p329}
compiler_host=${CC:-cc}
mkdir -p "$build"
cmake -S . -B "$build" -DCMAKE_BUILD_TYPE=Debug \
 -DNEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON -DNEWLANG_EXPERIMENTAL_NESTED_CALLER=ON \
 -DNEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL=ON -DNEWLANG_EXPERIMENTAL_OWNER_AGGREGATES=ON \
 -DNEWLANG_EXPERIMENTAL_THREE_ARG_OWNER_CALL=ON -DNEWLANG_EXPERIMENTAL_SCOPED_FIELD_CALL=ON \
 -DNEWLANG_EXPERIMENTAL_SCOPED_PREV_FIELD_CALL=ON -DNEWLANG_EXPERIMENTAL_TWO_SCOPED_FIELD_CALLS=ON \
 -DNEWLANG_EXPERIMENTAL_CROSS_ROOT_A_NEXT_AFTER_TWO_CALLS=ON \
 -DNEWLANG_EXPERIMENTAL_CROSS_ROOT_C_PREV_AFTER_A_NEXT=ON
cmake --build "$build" --parallel 2 --target newlangc cross_root_c_prev_evidence five_root_source_gate_evidence newlang_checked_c_node
"$compiler_host" -g -std=c17 -Wall -Wextra -Wpedantic -Werror -Iinclude evidence/p329/public_observer.c \
 "$build/libnewlang_semantic.a" "$build/libnewlang_checked_c_node.a" "$build/libnewlang_checked.a" \
 "$build/libnewlang_parser.a" "$build/libnewlang_diagnostic.a" "$build/libnewlang_syntax.a" \
 "$build/libnewlang_lexer.a" "$build/libnewlang_source.a" \
 -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -o "$build/p329-public-observer"
"$build/cross_root_c_prev_evidence" tests/fixtures/experimental_cross_root_c_prev.nl > "$build/p329-calibration.jsonl"
python3 evidence/p329/source_matrix.py "$build/newlangc" "$build/p329-public-observer" \
 "$build/five_root_source_gate_evidence" "$build/cross_root_c_prev_evidence" "$build/p329-observations.json"
"$build/p329-public-observer" evidence/p329/returned_owner.nl oom > "$build/p329-owned-oom.jsonl"
sha256sum "$build/newlangc" > "$build/p329-compiler-sha256.txt"
echo 'P329_ASSERTED_SAME_OWNED_FOUR_CHANGES_PURE_RETURN_DONOR_FIRST_RETURNED_ONLY_TERMINAL_OK'
