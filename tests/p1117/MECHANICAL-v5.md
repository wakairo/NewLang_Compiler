The next first RED was a fixture entry-API mismatch: a binary comparison was
passed to the restricted check_expression entry (NAME/CALL only), not the
existing check_source entry. This returns INTERNAL_ERROR before comparison.
It does not indicate an unknown Bool, dependency or production failure.

safety_v2.c changes exactly that fixture call to check_source. All current,
foreign source/world/signature/graph, A/B destruction and dependency assertions
are unchanged. The original safety.c and first log remain immutable; the OOM
binary continues to use it because that fixture function is not compiled there.
Production is unchanged from candidate4fe9c1f.

History24 externally pins prior history23. Original history22/23 and all prior
freeze JSON/archives stay exact. P1107 appends harness9 for CMake only. A common
current-history selector reads the already trusted P982 wrapper and runs all
original evidence assertions and every immutable P1117 freeze, avoiding another
copy of each evidence wrapper at each future version. It mints no authority.
