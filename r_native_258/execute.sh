#!/usr/bin/env bash
set -euo pipefail
. .deps/activate.sh
mkdir -p r_native_258/out/cases r_native_258/out/native r_native_258/out/mutants
python3 r_native_258/corpus.py > r_native_258/out/corpus-manifest.log
printf 'case\temit_exit\tgenerated_bytes\tgcc_compile\tbare_native\tobserver_compile\n' > r_native_258/out/case-results.tsv
for source in r_native_258/out/[0-1][0-9]_*.nl; do
  id=$(basename "$source" .nl)
  generated="r_native_258/out/cases/$id.c"
  diagnostic="r_native_258/out/cases/$id.emit.stderr"
  set +e
  ./build-r258/newlangc "$source" > "$generated" 2>"$diagnostic"
  e=$?
  set -e
  gcc_result=NA
  bare_result=NA
  observer_result=NA
  if [ "$e" -eq 0 ]; then
    set +e
    gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror "$generated" -o "r_native_258/out/native/$id" >"r_native_258/out/cases/$id.gcc.log" 2>&1
    g=$?
    set -e
    gcc_result=$g
    if [ "$g" -eq 0 ]; then
      set +e
      "r_native_258/out/native/$id" > "r_native_258/out/cases/$id.bare.log" 2>&1
      bare_result=$?
      set -e
    fi
    set +e
    gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror -I. \
      '-DNEWLANG_FIVE_OBSERVER="r_native_258/observer.h"' \
      "$generated" r_native_258/observer.c -Wl,--wrap=malloc -Wl,--wrap=free \
      -o "r_native_258/out/native/$id-observed" > "r_native_258/out/cases/$id.obs-compile.log" 2>&1
    observer_result=$?
    set -e
  fi
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$id" "$e" "$(wc -c < "$generated")" "$gcc_result" "$bare_result" "$observer_result" | tee -a r_native_258/out/case-results.tsv
done
# Run six real fault-injection worlds through independent read-only observer.
for id in 00_canonical 01_alpha_locals 02_alpha_nominal 03_permute_A_siblings 04_permute_B_siblings 05_reverse_disjoint_changes 06_swap_last_sum_arms; do
  exe="r_native_258/out/native/$id-observed"
  if [ -x "$exe" ]; then
    for world in 0 1 2 3 4 5; do
      set +e
      R258_FAIL_SITE="$world" "$exe" >"r_native_258/out/cases/$id.world$world.stdout" 2>"r_native_258/out/cases/$id.world$world.stderr"
      result=$?
      set -e
      echo "WORLD $id $world exit=$result $(cat "r_native_258/out/cases/$id.world$world.stdout")" | tee -a r_native_258/out/world-results.log
    done
  fi
done
# Application-policy controls are expected to FAIL canonical-topology oracle
# without pretending memory-safety rejection is required.
for id in 07_policy_A_prev_B 08_policy_B_next_A 09_policy_src_child_C 10_policy_wrong_dst_initial 11_target_A_next_as_prev; do
  exe="r_native_258/out/native/$id-observed"
  if [ -x "$exe" ]; then
    set +e
    R258_FAIL_SITE=0 "$exe" >"r_native_258/out/cases/$id.policy.stdout" 2>"r_native_258/out/cases/$id.policy.stderr"
    rc=$?
    set -e
    echo "POLICY $id observer_exit=$rc $(tail -1 "r_native_258/out/cases/$id.policy.stderr")" | tee -a r_native_258/out/policy-results.log
  fi
done
# Independent observer sensitivity: four classes of safe fresh-C mutants.
python3 r_native_258/mutants.py | tee r_native_258/out/mutant-manifest.log
for mutant in r_native_258/out/mutants/*.c; do
  id=$(basename "$mutant" .c)
  set +e
  gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror -I. \
    '-DNEWLANG_FIVE_OBSERVER="r_native_258/observer.h"' \
    "$mutant" r_native_258/observer.c -Wl,--wrap=malloc -Wl,--wrap=free \
    -o "r_native_258/out/mutants/$id" >"r_native_258/out/mutants/$id.compile" 2>&1
  c=$?
  rc=NA
  if [ "$c" -eq 0 ]; then
    R258_FAIL_SITE=0 "r_native_258/out/mutants/$id" >"r_native_258/out/mutants/$id.stdout" 2>"r_native_258/out/mutants/$id.stderr"
    rc=$?
  fi
  set -e
  echo "MUTANT $id compile=$c observer_exit=$rc $(tail -1 "r_native_258/out/mutants/$id.stderr" 2>/dev/null || true)" | tee -a r_native_258/out/mutant-results.log
done
# Sanitizers on all 0..5 worlds; independent of compiler's own build.
for san in address undefined; do
  set +e
  clang-23 -std=c17 -Wall -Wextra -Wpedantic -Werror -g -fno-omit-frame-pointer -fsanitize="$san" -fno-sanitize-recover=all -I. \
    '-DNEWLANG_FIVE_OBSERVER="r_native_258/observer.h"' \
    r_native_258/out/generated.c r_native_258/observer.c -Wl,--wrap=malloc -Wl,--wrap=free \
    -o "r_native_258/out/native/canonical-$san" >"r_native_258/out/$san.compile" 2>&1
  c=$?
  set -e
  echo "SAN $san compile=$c" | tee -a r_native_258/out/san-results.log
  if [ "$c" -eq 0 ]; then
    for world in 0 1 2 3 4 5; do
      set +e
      ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 R258_FAIL_SITE="$world" \
        "r_native_258/out/native/canonical-$san" >"r_native_258/out/$san.world$world.stdout" 2>"r_native_258/out/$san.world$world.stderr"
      rc=$?
      set -e
      echo "SAN $san world=$world exit=$rc $(cat "r_native_258/out/$san.world$world.stdout")" | tee -a r_native_258/out/san-results.log
    done
  fi
done
find r_native_258/out/cases r_native_258/out/native r_native_258/out/mutants -type f \( -name '*.c' -o -name '*.nl' -o -perm /111 \) -print0 | sort -z | xargs -0 -r sha256sum > r_native_258/out/derived-sha256.txt
git fetch --no-tags --depth=20 origin "$GITHUB_REF_NAME"
git diff --name-status b75baea96a644e68634baee383299b66981b3c62 HEAD | tee r_native_258/out/frozen-compare.txt
cat r_native_258/out/world-results.log r_native_258/out/mutant-results.log r_native_258/out/san-results.log
