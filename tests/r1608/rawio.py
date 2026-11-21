"""R1608 fixed data schema and bounded, non-following regular-file reads."""
import hashlib
import json
import os
import stat

CASES = (
    ("p1", "r1608_p1_main_zero_then.nl", False, 0),
    ("p2", "r1608_p2_helper_else.nl", False, 53),
    ("c1", "r1608_c1_selected_div_zero.nl", True, 53),
    ("c2", "r1608_c2_selected_rem_zero.nl", True, 53),
    ("c3", "r1608_c3_unselected_unit_arm.nl", True, 53),
    ("c4", "r1608_c4_hoisted_alternative.nl", False, 53),
)
FIRST = {
    "r1608_p1_main_zero_then.nl": "01980812c162c599bbb1d95a148f9e5c6f3de02798550156e545f5138a61e948",
    "r1608_p2_helper_else.nl": "4123b20b911149ffaaaf21bbd9184fba333e806cd61e0db61769132cc461e548",
    "r1608_c1_selected_div_zero.nl": "9a668f0b475544e4e64262519916eb01115ae6d333c970c4ffc91beec6870d0b",
    "r1608_c2_selected_rem_zero.nl": "3edb789d72b295117f0e0500121a5e61cc79a10181c8b725fa13ce407e759d5a",
    "r1608_c3_unselected_unit_arm.nl": "8a35c6fac4b353c49bfd7590784fa48d56868480fe2b13392a6be8e1923a1670",
    "r1608_c4_hoisted_alternative.nl": "f0e172aec2fd66566d5c14ceeb3d59e755823f3d81fd54d3f4e428ce1f33d1f2",
    "r1608_oracle.md": "8e75b130bd39e4fcbcec78920fdf8437eb292a9f4c2b26c2a87d4e812efed60c",
}
FIRST_MANIFEST_SHA = "91f79b36bc923673a2291a23195b285b19e6857950629d36d0120b771d836032"
PHASES = ("observer", "check", "emit", "cc", "native")
ROOTS = {
    "full": ("build", "r1608-raw-v1"),
    "partial": ("build-frontend", "r1608-raw-v1"),
}
STREAM_LIMIT = 65536
TOTAL_PUBLISH_BYTES = 8 * 1024 * 1024
CHUNK_BYTES = 2048

def canonical(value):
    return (json.dumps(value, sort_keys=True, separators=(",", ":"),
                       ensure_ascii=True) + "\n").encode("ascii")

def sha(data):
    return hashlib.sha256(data).hexdigest()

def allowlist():
    names = {
        "suite.start.json": 16384,
        "suite.result.json": 16384,
        "first.manifest": 2048,
        "adapter.nl": 64,
    }
    for cid, _name, _invalid, _number in CASES:
        names[cid + ".source.nl"] = 4096
        names[cid + ".decision.json"] = 16384
        names[cid + ".observer.rendered"] = STREAM_LIMIT
        names[cid + ".emitted.c"] = 262144
        names[cid + ".native.info.json"] = 16384
        for phase in PHASES:
            stem = cid + "." + phase
            names[stem + ".start.json"] = 16384
            names[stem + ".result.json"] = 16384
            names[stem + ".stdout.bin"] = STREAM_LIMIT
            names[stem + ".stderr.bin"] = STREAM_LIMIT
    return names

def signature(st):
    return (st.st_dev, st.st_ino, st.st_mode, st.st_nlink, st.st_size,
            st.st_mtime_ns, st.st_ctime_ns)

def open_directory(path):
    return os.open(path, os.O_RDONLY | os.O_DIRECTORY |
                   os.O_NOFOLLOW | os.O_NONBLOCK)

def open_fixed_root(workspace, mode):
    if mode not in ROOTS or not os.path.isabs(workspace):
        raise ValueError("invalid fixed raw root")
    fd = open_directory(workspace)
    try:
        for part in ROOTS[mode]:
            next_fd = os.open(part, os.O_RDONLY | os.O_DIRECTORY |
                              os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=fd)
            os.close(fd)
            fd = next_fd
        return fd
    except BaseException:
        os.close(fd)
        raise

def read_regular_at(directory_fd, name, cap):
    if not name or "/" in name or name in (".", ".."):
        raise ValueError("non-flat name")
    named = os.stat(name, dir_fd=directory_fd, follow_symlinks=False)
    if (not stat.S_ISREG(named.st_mode) or named.st_nlink != 1 or
            named.st_size < 0 or named.st_size > cap):
        raise ValueError("name is not a bounded regular single-link file")
    fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK,
                 dir_fd=directory_fd)
    try:
        before = os.fstat(fd)
        if signature(named) != signature(before):
            raise ValueError("file changed between non-following stat and open")
        if (not stat.S_ISREG(before.st_mode) or before.st_nlink != 1 or
                before.st_size < 0 or before.st_size > cap):
            raise ValueError("not a bounded regular single-link file")
        data = bytearray()
        while len(data) <= before.st_size:
            part = os.read(fd, min(65536, before.st_size + 1 - len(data)))
            if not part:
                break
            data.extend(part)
        after = os.fstat(fd)
        by_name = os.stat(name, dir_fd=directory_fd, follow_symlinks=False)
        if (signature(before) != signature(after) or
                signature(before) != signature(by_name) or
                len(data) != before.st_size):
            raise ValueError("file identity, timestamps, size or contents length changed")
        return bytes(data), signature(after)
    finally:
        os.close(fd)

def write_new(directory_fd, name, data):
    if not name or "/" in name or name in (".", ".."):
        raise ValueError("non-flat name")
    fd = os.open(name, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                 os.O_NOFOLLOW | os.O_NONBLOCK, 0o600, dir_fd=directory_fd)
    try:
        offset = 0
        while offset != len(data):
            wrote = os.write(fd, data[offset:])
            if wrote <= 0:
                raise OSError("short write")
            offset += wrote
        os.fsync(fd)
    finally:
        os.close(fd)
