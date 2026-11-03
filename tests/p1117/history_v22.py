"""Prospective C212: pin adopted main's v21 and inherit protected inputs."""
from pathlib import Path
import sys

V21 = 'tests/p990/current-freeze-v21.json'
SHA256 = 'e11b9c813c74ebd07784bd8a777568656fa20813530b005daeeb3fda80b4eb2f'
# Fixed from main65055be, independent of all mutable successor manifests.
OID = 'a8b9d6287971a15b5dcd554e3400672f492b3025'

def verify(root, current):
    root = Path(root)
    sys.path.insert(0,str(root/'tests/p1107'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v21 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root,current)
    raw = local(root,V21).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed adopted-v21 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed adopted-v21 Git blob')
    old=json_data(raw)
    require(old['generation']==21 and len(old['files'])==3091,'P1117 adopted-v21 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=22 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v21 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved adopted-v21 data: '+name)
    return count
