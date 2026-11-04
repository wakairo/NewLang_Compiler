"""R1157 fixed-authority transport for unchanged inherited freeze/history tests.

Actions only. Builds an isolated exact af16 Git tree using a private index and
read-only object alternates. Inherited source/fixture/assertion bytes are unchanged;
compiler, host compiler and sanitizer argv remain those of the actual R build.
"""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

PIN = 'af16bbaebcaada68ff45afdf236d1debfc90b565'
TREE = '74a1bdc728398eb85bb690a9eaff34194f316b50'
OWN = Path(__file__).resolve().parent


def invoke(command, *, cwd=None, data=None, env=None):
    p = subprocess.run([str(x) for x in command], input=data, capture_output=True,
                       cwd=cwd, env=env, check=True)
    return p.stdout


def blob(data):
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()


def prepare(repo, build):
    repo, build = Path(repo).resolve(), Path(build).resolve()
    raw = json.loads((OWN / 'authority/files.json').read_text())
    assert raw['compiler'] == PIN and raw['tree'] == TREE
    base = dict(raw['files'])
    assert len(base) == len(raw['files']) == 5959
    # Check the actual R working tree against every pinned production/history blob.
    # Only CMake's R registrations are an allowed inherited-path difference.
    for name, expected in base.items():
        p = (OWN / 'authority/CMakeLists.txt') if name == 'CMakeLists.txt' else repo / name
        assert p.is_file() and not p.is_symlink() and p.resolve().is_relative_to(repo)
        assert blob(p.read_bytes()) == expected, 'fixed authority drift: ' + name
    tracked = set(invoke(['git', '-C', repo, 'ls-files', '-z']).decode().rstrip('\0').split('\0'))
    assert set(base) <= tracked
    assert all(x.startswith('tests/r1157/') for x in tracked - set(base))
    prospective = json.loads((OWN / 'v4/manifest.json').read_text())
    for name, expected in prospective['files'].items():
        p = repo / name
        assert p.is_file() and not p.is_symlink()
        assert hashlib.sha256(p.read_bytes()).hexdigest() == expected, 'R prospective digest: ' + name
    assert 'CMakeLists.txt' in prospective['files']
    assert tracked - set(base) <= set(prospective['files']) | {'tests/r1157/v4/manifest.json'}
    root = build / 'r1157-fixed-authority'
    lock = build / 'r1157-fixed-authority.lock'
    with lock.open('a') as held:
        fcntl.flock(held, fcntl.LOCK_EX)
        receipt_path = build / 'r1157-fixed-authority.json'
        if not receipt_path.exists():
            assert not root.exists(), 'partial fixed transport requires investigation'
            root.mkdir()
            invoke(['git', 'init', '--quiet', root])
            objects = Path(invoke(['git', '-C', repo, 'rev-parse', '--path-format=absolute', '--git-path', 'objects']).decode().strip())
            (root / '.git/objects/info/alternates').write_text(str(objects) + '\n')
            original_cmake = (OWN / 'authority/CMakeLists.txt').read_bytes()
            assert invoke(['git', '-C', root, 'hash-object', '-w', '--stdin'], data=original_cmake).decode().strip() == base['CMakeLists.txt']
            index_data = ''.join(f'100644 {oid}\t{name}\n' for name, oid in raw['files']).encode()
            invoke(['git', '-C', root, 'update-index', '--index-info'], data=index_data)
            assert invoke(['git', '-C', root, 'write-tree']).decode().strip() == TREE
            commit = (OWN / 'authority/commit.raw').read_bytes()
            assert hashlib.sha1(b'commit ' + str(len(commit)).encode() + b'\0' + commit).hexdigest() == PIN
            assert invoke(['git', '-C', root, 'hash-object', '-w', '-t', 'commit', '--stdin'], data=commit).decode().strip() == PIN
            invoke(['git', '-C', root, 'update-ref', 'refs/heads/r1157-fixed-authority', PIN])
            invoke(['git', '-C', root, 'symbolic-ref', 'HEAD', 'refs/heads/r1157-fixed-authority'])
            # Match an ordinary depth-one checkout: no claim about unavailable ancestry.
            (root / '.git/shallow').write_text(PIN + '\n')
            invoke(['git', '-C', root, 'checkout-index', '--all', '--force'])
            assert invoke(['git', '-C', root, 'rev-parse', 'HEAD^{tree}']).decode().strip() == TREE
            for name, expected in raw['files']:
                assert blob((root / name).read_bytes()) == expected, 'snapshot materialization: ' + name
            assert invoke(['git', '-C', root, 'status', '--porcelain', '--untracked-files=no']) == b''
            receipt = {'authority_commit': PIN, 'authority_tree': TREE, 'files': len(base),
                       'actual_r_checkout': invoke(['git', '-C', repo, 'rev-parse', 'HEAD']).decode().strip(),
                       'actual_build': str(build), 'snapshot': str(root),
                       'mechanism': 'private Git index; full fixed-tree bytes; unchanged original assertions and argv'}
            receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
        else:
            receipt = json.loads(receipt_path.read_text())
            assert receipt['authority_commit'] == PIN and receipt['authority_tree'] == TREE
            assert invoke(['git', '-C', root, 'rev-parse', 'HEAD']).decode().strip() == PIN
            assert invoke(['git', '-C', root, 'status', '--porcelain', '--untracked-files=no']) == b''
    return root


def main():
    repo, build = map(lambda x: Path(x).resolve(), sys.argv[1:3])
    root = prepare(repo, build)
    if len(sys.argv) == 3:
        print('R1157 fixed-authority transport PASS ' + str(root))
        return 0
    actual = sys.argv[3:]
    translated = []
    for index, arg in enumerate(actual):
        # Native argv are [Python, script, actual CLI, actual CC, SAN, ...].
        # Preserve all three explicitly even if the SDK is inside the R checkout.
        if (len(actual) >= 5 and index in (2, 3, 4)) or arg == str(build) or arg.startswith(str(build) + '/'):
            translated.append(arg)
        elif arg == str(repo) or arg.startswith(str(repo) + '/'):
            translated.append(str(root) + arg[len(str(repo)):])
        else:
            translated.append(arg)
    print('R1157 inherited fixed-tree transport ' + json.dumps({'original': actual, 'executed': translated}), flush=True)
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    return subprocess.run(translated, cwd=root, env=env).returncode


if __name__ == '__main__':
    sys.exit(main())
