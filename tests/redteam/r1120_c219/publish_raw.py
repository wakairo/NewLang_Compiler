#!/usr/bin/env python3
"""Transport already-recorded R bytes through CTest's standard post-test log."""
import argparse
import base64
import hashlib
import io
import json
from pathlib import Path
import zipfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()
    root = Path(args.output_dir).resolve()
    if not root.exists():
        print("R1120_C219_RAW_ABSENT: no R execution; registration-only invocation.")
        return 0
    entries, payload = [], []
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise RuntimeError("STOP: symlink in R evidence")
        if path.is_file():
            data = path.read_bytes()
            name = str(path.relative_to(root))
            entries.append({"path": name, "bytes": len(data),
                            "sha256": hashlib.sha256(data).hexdigest()})
            payload.append((name, data))
    if not payload:
        raise RuntimeError("STOP: existing R raw directory is empty")
    manifest = {"transport": "R-owned original ZIP in CTest post-test stdout",
                "not_github_native_artifact_zip": True,
                "executables": "Excluded; binary SHA256 and sizes recorded in summary",
                "files": entries}
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, data in payload:
            archive.writestr(name, data)
        archive.writestr("RAW_FILE_MANIFEST.json",
                         json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    original_zip = buffer.getvalue()
    digest = hashlib.sha256(original_zip).hexdigest()
    (root.parent / "r1120-c219-raw-original.zip").write_bytes(original_zip)
    metadata = {"sha256": digest, "bytes": len(original_zip),
                "entries": len(payload) + 1,
                "uncompressed_file_bytes": sum(x["bytes"] for x in entries)}
    print("R1120_C219_RAW_ZIP_BEGIN " + json.dumps(metadata, sort_keys=True), flush=True)
    encoded = base64.b64encode(original_zip).decode("ascii")
    for index in range(0, len(encoded), 8192):
        print("R1120_C219_RAW_ZIP_CHUNK " + str(index // 8192) + " " +
              encoded[index:index + 8192], flush=True)
    print("R1120_C219_RAW_ZIP_END " + digest, flush=True)
    summary_path = root / "summary.json"
    if summary_path.exists():
        summary = json.loads(summary_path.read_text())
        print("R1120_C219_RAW_VERDICT " + json.dumps({
            "verdict": summary.get("verdict"),
            "reason": summary.get("reason"),
            "original_verdict": summary.get("original", {}).get("verdict"),
            "native_target": summary.get("native_target"),
            "native_compiler": summary.get("native_compiler"),
            "sanitizer": summary.get("sanitizer"),
            "cases": [{"file": x["file"], "disposition": x.get("disposition")}
                      for x in summary.get("cases", [])]
        }, sort_keys=True), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
