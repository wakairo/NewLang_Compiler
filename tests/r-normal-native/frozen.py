"""R656 byte identity gate; run before any compiler observation."""
import hashlib
import json
from pathlib import Path

FROZEN = {
    "phase2_true.nl": (
        "415a7d4be78e72c6825af130abb62ae58ef6d505",
        "22c95ca06448d5292ddf50ae3d723e7c08923e0a972ef9ec2507ffa31064cb10",
    ),
    "phase2_false.nl": (
        "0a6b7f2904e277152d0b1e635c5e662425d291f1",
        "794772acda78ca75a6965a6bc4b56dab7fea148b0c5fc17192024aeb92c47768",
    ),
    "PHASE2_ORACLE.json": (
        "77459cf82c8644f2e95c1f969a99db9225ba01dd",
        "e2ee609f54f439f796c30f4f2a92716ddeb3d2eef72615a54d7ac59ebcea88f3",
    ),
}


def verify(root: Path) -> dict:
    for name, (blob, sha256) in FROZEN.items():
        data = (root / name).read_bytes()
        header = f"blob {len(data)}\0".encode()
        if (hashlib.sha1(header + data).hexdigest() != blob
                or hashlib.sha256(data).hexdigest() != sha256):
            raise RuntimeError(f"STOP: frozen identity drift: {name}")
        print(f"FROZEN {name} blob={blob} sha256={sha256}", flush=True)
    return json.loads((root / "PHASE2_ORACLE.json").read_text())


if __name__ == "__main__":
    verify(Path(__file__).resolve().parent)
