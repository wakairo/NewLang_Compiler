#!/usr/bin/env bash
# Archival reproduction only. No additional proof or acceptance is claimed.
set -euo pipefail
b=${1:?usage: reproduce-refusal.sh ABSOLUTE_BUILD_DIRECTORY}
python3 evidence/p365/isolate.py "$b"
set +e
"$b/isolated/observer" evidence/p365/returned_owner.nl > "$b/first-refusal.jsonl" 2> "$b/first-refusal.trace"
status=$?
set -e
printf 'observer_exit=%s\n' "$status"
test "$status" -eq 1
rg -q 'P365_ENDPOINT place=96' "$b/first-refusal.trace"
rg -q 'P365_HOLD node=149 reason=ENDPOINT_PLACE' "$b/first-refusal.trace"
# A successful script here means the known FALSE REJECTION reproduced,
# never a successful custody witness or rejection of a corrupted artifact.
