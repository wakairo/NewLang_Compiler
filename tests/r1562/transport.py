import base64
import hashlib
import json
import pathlib
import sys

# Pass-independent post-CTest transport. Never changes a test outcome.
try:
    root = pathlib.Path(sys.argv[1])
    maximum = 600000
    total = 0
    if not root.is_dir():
        print("R1562_TRANSPORT_MISSING " + str(root), flush=True)
    else:
        allowed = {".json", ".jsonl", ".stdout", ".stderr", ".c", ".nl",
                   ".txt", ".patch"}
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix not in allowed:
                continue
            data = path.read_bytes()
            relative = path.relative_to(root).as_posix()
            if len(data) > 200000 or total + len(data) > maximum:
                print("R1562_TRANSPORT_TRUNCATED " +
                      json.dumps({"path": relative, "bytes": len(data)}),
                      flush=True)
                continue
            total += len(data)
            meta = {"path": relative, "bytes": len(data),
                    "sha256": hashlib.sha256(data).hexdigest()}
            print("R1562_FILE_BEGIN " + json.dumps(meta, sort_keys=True), flush=True)
            encoded = base64.b64encode(data).decode("ascii")
            for start in range(0, len(encoded), 3000):
                print("R1562_DATA " + encoded[start:start + 3000], flush=True)
            print("R1562_FILE_END " + json.dumps(relative), flush=True)
        print("R1562_TRANSPORT_DONE " + json.dumps({"bytes": total}), flush=True)
except Exception as exc:
    print("R1562_TRANSPORT_ERROR " + repr(exc), flush=True)
sys.exit(0)
