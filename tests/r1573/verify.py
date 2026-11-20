#!/usr/bin/env python3
"""Independent R1573 frozen FIRST SHA-256 byte gate."""
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
MANIFEST_SHA = "f18b495bf138a299b3fe295de8083799cd43c7ce3d1d2937fc6f7b7fcdfbe64e"

def check():
    raw = (ROOT / "FIRST.manifest.tsv").read_bytes()
    if len(raw) != 1300 or hashlib.sha256(raw).hexdigest() != MANIFEST_SHA:
        raise AssertionError("R1573_STOP original FIRST manifest mismatched")
    if not raw.endswith(b"\n") or b"\r" in raw:
        raise AssertionError("R1573_STOP manifest newline")
    rows = raw.decode("ascii").splitlines()
    if len(rows) != 14 or rows[0] != "filename\tbytes\tsha256":
        raise AssertionError("R1573_STOP manifest structure")
    seen = set()
    for record in rows[1:]:
        filename, expected_bytes, expected_hash = record.split("\t")
        if filename in seen:
            raise AssertionError("R1573_STOP duplicate FIRST path")
        seen.add(filename)
        group = "sources" if filename.endswith(".nl") else "oracles"
        data = (ROOT / group / filename).read_bytes()
        if (len(data) != int(expected_bytes) or
            hashlib.sha256(data).hexdigest() != expected_hash or
            not data.endswith(b"\n") or b"\r" in data or not data.isascii()):
            raise AssertionError("R1573_STOP FIRST changed: " + filename)
        print(f"R1573_FIRST|{filename}|{expected_bytes}|{expected_hash}",
              flush=True)
    present = {p.name for group in ("sources", "oracles")
               for p in (ROOT / group).iterdir() if p.is_file()}
    if seen != present or len(seen) != 13:
        raise AssertionError("R1573_STOP FIRST file set")
    print("R1573_FIRST_ALL_13_BYTE_SHA_PASS", flush=True)

if __name__ == "__main__":
    check()
