The adjacent C file is a historical checksum fixture, not a compiler input.
It is the exact `src/usize_addition.c` Git blob
`901f758a9f1b937cdb854c86be42bdaedd32cdad`, recovered from adopted base
`f7c277af59a4c4281c07fd6d3cc1d98f8bf88f48`. Its 16811 bytes have SHA256
`31e1b76beaae03471fc2243e0a1c8402b95a8ccbd42e2c10a68235f496cfbbc7`,
matching the unchanged original `tests/p605/frozen.json` arithmetic digest.

P1098 authenticates both the historical Git object ID/SHA256 and the current
source's binding to actual Git HEAD, then copies historical bytes only into the
isolated P605/P634 provenance fixture. This also works in shallow offline CI.
All original P634 proof-file mutation attacks run on that historical fixture.
P605 still runs the original source/public-validator/strict-native checks with
the actual current compiler/probe; the historical file is never compiled into
those executables. No source, oracle, expected digest or test is disabled.
