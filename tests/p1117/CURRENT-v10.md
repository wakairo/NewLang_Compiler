The second old precision oracle is the P603 unused two-usize/Bool-result
function definition. It now CHECKs successfully and its unit main emits/runs
successfully, without claiming a supported multiarg Bool native ABI. The
original source/script/oracle/candidate remain unchanged; the stopped full
GCC log is archived. Prospective wrapper v2 only adds that source's explicit
real CHECK0/C170/native0 at O0/O2 to frozen v9 expectations. Every other old
assertion remains; no subprocess, status, runtime output or old JSON is forged.
Production remains 7b7881a. Local Clang16/16, UBSan16/16 and ASan/LSan host
15/15 succeeded on dc42663 (same production). No CI or LOOP success claimed.
