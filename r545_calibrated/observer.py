#!/usr/bin/env python3
"""R545 read-only observer inserted in actual compiler-emitted C17. No guest writes."""
import difflib
import hashlib
from pathlib import Path
import sys

if len(sys.argv) != 4:
    raise SystemExit("usage: observer.py PLAIN.c OBSERVED.c DIFF.txt")
plain, observed, delta = map(Path, sys.argv[1:])
raw = plain.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
expected = "516de3980e1b667dcc0e22a85de3780df2e3df9daf751e1b9dc5402ca85a750c"
if sha != expected:
    raise SystemExit(f"R545 STOP plain C17 changed: {sha} != {expected}")
src = raw.decode("utf-8")
needle = "#include <stdint.h>\n"
if src.count(needle) != 1:
    raise SystemExit("R545 STOP generated include anchor")
src = src.replace(needle, needle + "#include <inttypes.h>\n#include <stdio.h>\n#include <stdlib.h>\n", 1)
anchor = "    (void)nl_local_2;\n"
if src.count(anchor) != 1 or src.count("    (void)nl_local_1;\n") != 1:
    raise SystemExit("R545 STOP actual original-local anchors changed")
instrument = r'''    /* R545 passive observer: actual original source locals, both currently alive.
     * No writes to guest values, no seeded/synthetic arrays, no guest authority. */
    {
        const nl_option_10 *const a = nl_local_1;
        const nl_option_10 *const b = nl_local_2;
        const uintptr_t a0 = (uintptr_t)(const void *)&a[0];
        const uintptr_t b0 = (uintptr_t)(const void *)&b[0];
        const uintptr_t ae = (uintptr_t)(const void *)&a[4];
        const uintptr_t be = (uintptr_t)(const void *)&b[4];
        if (sizeof(nl_local_1) != 4 * sizeof(nl_option_10) ||
            sizeof(nl_local_2) != 4 * sizeof(nl_option_10) ||
            a0 == b0 || a0 >= ae || b0 >= be ||
            !((ae <= b0) || (be <= a0))) abort();
        printf("R545 physical rootA=%" PRIxPTR " rootB=%" PRIxPTR
               " sizeofA=%zu sizeofB=%zu option_stride=%zu disjoint=1\n",
               a0, b0, sizeof(nl_local_1), sizeof(nl_local_2),
               sizeof(nl_option_10));
        for (size_t i = 0; i < 4; ++i) {
            const nl_option_10 *const ea = &a[i];
            const nl_option_10 *const eb = &b[i];
            const uintptr_t xa = (uintptr_t)(const void *)ea;
            const uintptr_t xb = (uintptr_t)(const void *)eb;
            if (ea->tag != NL_ARRAY4_NONE || eb->tag != NL_ARRAY4_NONE ||
                ea->payload != 0 || eb->payload != 0) abort();
            if (i && (xa - (uintptr_t)(const void *)&a[i-1] != sizeof(*ea) ||
                      xb - (uintptr_t)(const void *)&b[i-1] != sizeof(*eb)))
                abort();
            printf("R545 physical i=%zu a=%" PRIxPTR " b=%" PRIxPTR
                   " a_tag=%u b_tag=%u a_payload=%u b_payload=%u\n",
                   i, xa, xb, (unsigned)ea->tag, (unsigned)eb->tag,
                   (unsigned)ea->payload, (unsigned)eb->payload);
        }
    }
'''
result = src.replace(anchor,anchor+instrument,1)
observed.write_bytes(result.encode("utf-8"))
diff = "".join(difflib.unified_diff(raw.decode("utf-8").splitlines(True),
                                   result.splitlines(True),
                                   fromfile="plain.c17",tofile="passive-observed.c17"))
delta.write_text(diff,encoding="utf-8")
print(f"R545 plain C17 original SHA256={sha} bytes={len(raw)}",flush=True)
print(f"R545 observed C17 SHA256={hashlib.sha256(result.encode()).hexdigest()} bytes={len(result)}",flush=True)
print(f"R545 passive observation changed={len(diff.splitlines())} logged diff lines",flush=True)
