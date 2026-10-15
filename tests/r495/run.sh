#!/usr/bin/env bash
set -euo pipefail
test "$(git rev-parse HEAD^)" = b7b9f10dabe8bdcefe531392411d285898219c05
test "$(sha256sum tests/p493/nonmatch_link.nl | awk '{print $1}')" = 06c96f6df6688167f2f9d8e2768bfc5bc2ea9f93e09838502400039d2de5f8aa
build=/tmp/r495-build
cc="$R495_CC"
cmake -S . -B "$build" -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_EXPERIMENTAL_ORDINARY_CALL_PROOF=OFF -DCMAKE_C_COMPILER="$cc"
cmake --build "$build" --target p493_proof_test p493_default_test newlangc --parallel 2
for profile in proof default; do
  if test "$profile" = proof; then
    target=p493_proof_test
    options=""
    output=r495_observer
  else
    target=p493_default_test
    options="-DR495_EXPECT_OFF=1"
    output=r495_off
  fi
  "$cc" -std=c17 -Wall -Wextra -Wpedantic -Werror -Iinclude $options -c tests/r495/observer.c -o "$build/$profile.o"
  sed -e "s#CMakeFiles/$target.dir/tests/p493/proof_test.c.o#$profile.o#g" -e "s#-o $target#-o $output#g" -e 's# -Wl,--wrap=malloc##g' -e 's# -Wl,--wrap=realloc##g' -e 's# -Wl,--wrap=calloc##g' "$build/CMakeFiles/$target.dir/link.txt" > "$build/r495-$profile-link.sh"
  (cd "$build"; bash "r495-$profile-link.sh")
  "$build/$output" tests/p493/nonmatch_link.nl
done
