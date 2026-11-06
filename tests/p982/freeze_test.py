"""Prospective C103 successor; original checker/manifests remain in parent history."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
from versioned_freeze import verify

CURRENT = "tests/p990/current-freeze-v55.json"
# Prospective v9 pins the externally fixed original v9 after the unchanged
# v1-v8 historical algorithms/literals; predecessor source/data stay intact.
sys.path.insert(0, str(root / "tests/p1062"))
from history_v12 import verify as verify_v12
sys.path.insert(0, str(root / "tests/p1098"))
from history_v20 import verify as verify_v20

sys.path.insert(0, str(root / "tests/p1107"))
from history_v21 import verify as verify_v21
sys.path.insert(0, str(root / "tests/p1117"))
from history_v22 import verify as verify_v22
from history_v23 import verify as verify_v23
from history_v24 import verify as verify_v24
from history_v25 import verify as verify_v25
from history_v26 import verify as verify_v26
from history_v27 import verify as verify_v27
from history_v28 import verify as verify_v28
from history_v29 import verify as verify_v29
from history_v30 import verify as verify_v30
from history_v31 import verify as verify_v31
sys.path.insert(0, str(root / "tests/p1123"))
from history_v32 import verify as verify_v32
from history_v33 import verify as verify_v33
sys.path.insert(0, str(root / "tests/p1133"))
from history_v34 import verify as verify_v34
sys.path.insert(0, str(root / "tests/p1139"))
from history_v35 import verify as verify_v35
sys.path.insert(0, str(root / "tests/p1146"))
from history_v36 import verify as verify_v36
sys.path.insert(0, str(root / "tests/p1152"))
from history_v37 import verify as verify_v37
sys.path.insert(0, str(root / "tests/p1158"))
from history_v38 import verify as verify_v38
sys.path.insert(0, str(root / "tests/p1169"))
from history_v39 import verify as verify_v39
sys.path.insert(0, str(root / "tests/p1176"))
from history_v40 import verify as verify_v40
sys.path.insert(0, str(root / "tests/p1184"))
from history_v41 import verify as verify_v41
from history_v42 import verify as verify_v42
sys.path.insert(0, str(root / "tests/p1188"))
from history_v43 import verify as verify_v43
from history_v44 import verify as verify_v44
from history_v45 import verify as verify_v45
from history_v46 import verify as verify_v46
from history_v47 import verify as verify_v47
from history_v48 import verify as verify_v48
from history_v49 import verify as verify_v49
sys.path.insert(0, str(root / "tests/p1193"))
from history_v50 import verify as verify_v50
from history_v51 import verify as verify_v51
from history_v52 import verify as verify_v52
sys.path.insert(0, str(root / "tests/p1203"))
from history_v53 import verify as verify_v53
sys.path.insert(0, str(root / "tests/p1208/c266"))
from history_v54 import verify as verify_v54
from history_v55 import verify as verify_v55
count = verify_v55(root, CURRENT)
print("P982 original Git/source/oracle lineage and current versioned hashes PASS", count)
