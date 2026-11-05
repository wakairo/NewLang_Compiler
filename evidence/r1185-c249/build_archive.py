"""Build a reversible custody archive from this R's own already preserved files."""
import base64
import datetime
import gzip
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import tarfile


BASE = Path(__file__).resolve().parent
ROOT = BASE.parent
STAGE = BASE / "stage"
SHA = lambda data: hashlib.sha256(data).hexdigest()


def write_json(path, value):
    data = (json.dumps(value, indent=2) + "\n").encode()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return {"path": str(path.relative_to(STAGE)), "bytes": len(data), "sha256": SHA(data)}


def main():
    STAGE.mkdir(exist_ok=False)
    files, objects, raw_bytes = [], {}, {}
    for path in sorted(ROOT.rglob("*")):
        rel = path.relative_to(ROOT)
        if rel.parts[0] == "archive-c249" or "__pycache__" in rel.parts or not path.is_file():
            continue
        if path.is_symlink():
            raise ValueError("unexpected symlink in original R custody")
        data = path.read_bytes()
        key = SHA(data)
        files.append({"path": str(rel), "bytes": len(data), "sha256": key,
                      "mode": stat.S_IMODE(path.stat().st_mode)})
        objects.setdefault(key, {"sha256": key, "bytes": len(data), "source": {"kind": "stored"}})
        raw_bytes.setdefault(key, data)
    by_path = {r["path"]: r for r in files}
    logs = [r for r in files if re.fullmatch(r"execution/ci/(?:fast\d|normal\d/[^/]+)/job-\d+\.log", r["path"])]
    assert len(logs) == 22
    log_ids = {int(re.search(r"job-(\d+)\.log$", r["path"])[1]) for r in logs}
    metadata_ids = set()
    for path in (ROOT / "execution/ci/terminal-metadata").glob("run-*.json"):
        metadata = json.loads(path.read_bytes())
        assert metadata["run"]["status"] == "completed" and metadata["run"]["run_attempt"] == 1
        metadata_ids |= {j["id"] for j in metadata["jobs"]["jobs"]}
    assert metadata_ids == log_ids
    log_hashes = {r["sha256"] for r in logs}
    archives = [r for r in files if re.fullmatch(r"execution/ci/normal[123]/[^/]+/recovered/original-export.tar.gz", r["path"])]
    assert len(archives) == 18
    for item in archives:
        recovery = ROOT / item["path"]
        receipt = json.loads(recovery.with_name("recovery-receipt.json").read_bytes())
        log_path = Path(receipt["original_log"])
        log_name = str(log_path.relative_to(ROOT) if log_path.is_absolute() else log_path)
        log = by_path[log_name]
        assert log["sha256"] == receipt["original_log_sha256"]
        assert item["sha256"] == receipt["gzip_sha256"] and item["bytes"] == receipt["gzip_bytes"]
        text = raw_bytes[log["sha256"]].decode("utf-8")
        headers = list(re.finditer(r"R1185_RAW_BEGIN sha256=([0-9a-f]{64}) bytes=(\d+)", text))
        assert len(headers) == 1 and headers[0].group(1) == item["sha256"]
        tail = text[headers[0].end():]
        assert tail.count("R1185_RAW_END") == 1
        encoded = "".join(re.findall(r"R1185_RAW ([A-Za-z0-9+/=]+)", tail.split("R1185_RAW_END", 1)[0]))
        assert base64.b64decode(encoded, validate=True) == raw_bytes[item["sha256"]]
        objects[item["sha256"]]["source"] = {"kind": "log_export", "log_sha256": log["sha256"]}
    for item in archives:
        with tarfile.open(fileobj=io.BytesIO(raw_bytes[item["sha256"]]), mode="r:gz") as tar:
            for member in tar.getmembers():
                assert member.isfile() and not PurePosixPath(member.name).is_absolute() and ".." not in PurePosixPath(member.name).parts
                stream = tar.extractfile(member)
                assert stream is not None
                data = stream.read();key = SHA(data)
                assert len(data) == member.size
                if key in objects and key not in log_hashes and objects[key]["source"]["kind"] == "stored":
                    assert data == raw_bytes[key]
                    objects[key]["source"] = {"kind": "tar_member", "archive_sha256": item["sha256"], "member": member.name}
    for item in archives:
        encoded = base64.b64encode(raw_bytes[item["sha256"]]);key = SHA(encoded)
        if key in objects and key not in log_hashes:
            assert encoded == raw_bytes[key]
            objects[key]["source"] = {"kind": "base64", "object_sha256": item["sha256"]}
    stored = [r for r in objects.values() if r["source"]["kind"] == "stored"]
    packed_file = BASE / "stored-objects.tar.gz"
    with packed_file.open("wb") as output:
        with gzip.GzipFile(filename="", mode="wb", fileobj=output, mtime=0, compresslevel=6) as zipped:
            with tarfile.open(fileobj=zipped, mode="w|", format=tarfile.USTAR_FORMAT) as tar:
                for item in sorted(stored, key=lambda r: r["sha256"]):
                    data = raw_bytes[item["sha256"]]
                    info = tarfile.TarInfo("objects/" + item["sha256"])
                    info.size = len(data);info.mode = 0o444;info.mtime = 0
                    tar.addfile(info, io.BytesIO(data))
    packed = packed_file.read_bytes();parts = []
    for index, offset in enumerate(range(0, len(packed), 450000)):
        data = packed[offset:offset + 450000];encoded = base64.b64encode(data)
        path = STAGE / "chunks" / (f"objects-{index:04d}.b64")
        path.parent.mkdir(parents=True, exist_ok=True);path.write_bytes(encoded)
        parts.append({"path": str(path.relative_to(STAGE)), "bytes": len(encoded), "sha256": SHA(encoded),
                      "decoded_bytes": len(data), "decoded_sha256": SHA(data)})
    file_parts = [write_json(STAGE / "file-index" / f"files-{i//1500:03d}.json", files[i:i+1500])
                  for i in range(0, len(files), 1500)]
    ordered = sorted(objects.values(), key=lambda r: r["sha256"])
    object_parts = [write_json(STAGE / "object-index" / f"objects-{i//1000:03d}.json", ordered[i:i+1000])
                    for i in range(0, len(ordered), 1000)]
    shutil.copyfile(BASE / "restore.py", STAGE / "restore.py")
    shutil.copyfile(BASE / "build_archive.py", STAGE / "build_archive.py")
    results = STAGE / "results";results.mkdir()
    for name in ("summary.json", "FINAL.md", "result-bundle-receipt.json"):
        shutil.copyfile(ROOT / "execution/final" / name, results / name)
    (results / "result-receipts.tar.gz.b64").write_bytes(base64.b64encode((ROOT / "execution/final/result-receipts.tar.gz").read_bytes()))
    manifest = {"schema": "R1185_C249_REVERSIBLE_ARCHIVE_V1",
        "frozen_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "release": "https://github.com/wakairo/NewLang_Compiler/issues/1185#issuecomment-6659250946",
        "parent": "a6367e6bc8e52acfee620ca9d840afdc4b700d38", "parent_tree": "43ab70ee1fd91d7d17e3054b9831345767f31c6e",
        "branch": "r-archive/r1185-c249-evidence", "append_prefix": "evidence/r1185-c249/",
        "file_count": len(files), "logical_bytes": sum(x["bytes"] for x in files),
        "unique_object_count": len(objects), "stored_object_count": len(stored),
        "file_index_parts": file_parts, "object_index_parts": object_parts,
        "stored_object_bundle": {"bytes": len(packed), "sha256": SHA(packed), "parts": parts},
        "evidence_counts": {"whole_decoded_logs": 22, "terminal_run_metadata": 10, "actual_jobs": 22,
                            "original_raw_gzip": 18, "V1_V2_files": 44,
                            "E0_E6_snapshot_files": sum(r["path"].startswith(tuple("execution/equipment-e"+str(i)+"/" for i in range(7))) for r in files)},
        "workflow_scope": {"compiler-ci.yml": "push main / pull_request", "compiler-frontend-ci.yml": "push main / pull_request", "r-preflight.yml": "push r-fast/**", "archive_prefix_matches": False, "new_PR": 0},
        "limits": ["whole decoded log UTF-8 bytes preserved; original GitHub ZIP binary not acquired and no independent ZIP hash claimed",
                   "original exported gzip bytes restored exactly from original log framing, never recompressed",
                   "actual C/AST/map/passive/raw and FIRST/RED/NOT_RUN restored from exact original tar member bytes",
                   "no compiler/native/build/AST/observer semantic execution; restoration is hash/base64/gzip/tar only",
                   "Python __pycache__ and this archive staging directory are not evidence inputs and are excluded"]}
    write_json(STAGE / "manifest.json", manifest)
    readme = """# R1185 C249 immutable evidence archive\n\nThis archive is append-only beneath evidence/r1185-c249. Its sole commit parent is the completed R execution commit a6367e6bc8e52acfee620ca9d840afdc4b700d38. The execution branch and DRAFT/NEVERMERGE PR1189 are not moved. New PR/Actions/compiler/build/native/AST/observer semantic execution: zero.\n\n## Restore exact original files\n\nRun `python restore.py . /path/to/new-empty-output` from this directory. For hash verification without writing recovered files, run `python restore.py . --verify-only`. Only Python's standard library is needed. No compiler, target program, observer, network, or subprocess is invoked. Do not run the restored execution drivers locally.\n\nThe manifest lists every original path, byte length, mode and SHA256 through file-index parts. Object-index parts describe exact-byte custody. Whole decoded logs are stored objects. Each original raw gzip is recovered by joining its original log's R1185_RAW payload lines between the unique declared BEGIN/END pair and verifying its full byte count/hash. Original C/AST/map/passive/raw, metadata, FIRST/RED/NOT_RUN and frozen inputs are exact tar members. Repeated files share content identities; the six frozen base64 reuse assets are the exact base64 encoding of their original gzip objects. This removes duplicate storage without removing any original file or changing any original bytes.\n\nObjects that cannot be recovered from existing raw are stored under content hashes in the gzip bundle. Join decoded chunks in manifest order; the script checks each encoded/decoded chunk, the complete bundle, each stored/reconstructed object and every original file. The root MANIFEST.sha256 covers all repository archive files except itself. Git blob hashes are recorded separately in the publication receipt.\n\n## Evidence and limits\n\nAll 22 whole decoded logs, 10 terminal run/job metadata sets, 18 NORMAL1/2/3 original raw gzip exports, V1/V2, E0-E6, original failures and NOT_RUN, all captured artifacts, final reports and reconstruction scripts are preserved. UTF-8 decoded job-log bytes are what the connector supplied; no independently acquired binary job-log ZIP hash is claimed. V1 remains NOT_RUN. V2 is unchanged. Historical target/budget text in immutable V2 is retained; actual execution binding is C247/C248 fixed faad092b5b0deb608a048ff60bee0056082f9a59 and explicit release receipts.\n\nFinal finite results were accepted by C249. This is evidence preservation, not a new execution or adoption gate. Ternary choices are inference from captured operands and actual AST; caller results and internal RETURN_VALUE observations remain separate. Public IDs require artifact/world qualifiers and are not native addresses. No machine CALL/STORE survival, general freshness/authority/soundness or unavailable whole-loop-validator proof is asserted. The complete concise result is in results/FINAL.md; the small results bundle is independently reconstructible from results/result-receipts.tar.gz.b64.\n"""
    (STAGE / "README.md").write_text(readme)
    added=[]
    for p in sorted(STAGE.rglob("*")):
        if p.is_file():
            data=p.read_bytes();rel=str(p.relative_to(STAGE));added.append({"path":"evidence/r1185-c249/"+rel,"bytes":len(data),"sha256":SHA(data),"git_blob_sha1":hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()})
    outer="".join(f"{r['sha256']}\t{r['bytes']}\t{r['path'].removeprefix('evidence/r1185-c249/')}\n" for r in added).encode()
    (STAGE / "MANIFEST.sha256").write_bytes(outer)
    added.append({"path":"evidence/r1185-c249/MANIFEST.sha256","bytes":len(outer),"sha256":SHA(outer),"git_blob_sha1":hashlib.sha1(b"blob "+str(len(outer)).encode()+b"\0"+outer).hexdigest()})
    (BASE / "publication-files.json").write_text(json.dumps(added,indent=2)+"\n")
    receipt={"original_files":len(files),"logical_bytes":manifest["logical_bytes"],"unique_objects":len(objects),"stored_objects":len(stored),"stored_bundle_bytes":len(packed),"stored_bundle_sha256":SHA(packed),"archive_paths":len(added),"archive_git_payload_bytes":sum(x["bytes"] for x in added),"manifest_json_sha256":SHA((STAGE/'manifest.json').read_bytes()),"outer_manifest_sha256":SHA(outer),"chunk_count":len(parts)}
    (BASE/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+"\n")
    print(json.dumps(receipt,indent=2))


if __name__ == "__main__":
    main()
