"""Prospective v21 externally fixes the adopted main v20 inventory."""
from pathlib import Path
import sys
V20 = 'tests/p990/current-freeze-v20.json'
SHA256 = '5568bfac7f44a3034a33991fe61361a68bf11c4d8fcaf5535c26507d7f0328a9'
OID = '708b89f47c4c3f9c4fdfecb84f3b4b84cb771644'
def verify(root, current):
    root = Path(root)
    sys.path.insert(0,str(root/'tests/p1098'))
    from history_v20 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root,current)
    raw = local(root,V20).read_bytes()
    require(digest(raw)==SHA256,'P1107 fixed adopted-v20 SHA256')
    require(git_oid('blob',raw)==OID,'P1107 fixed adopted-v20 Git blob')
    old=json_data(raw)
    require(old['generation']==20 and len(old['files'])==3041,'P1107 adopted-v20 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=21 and old['files'].keys()<=value['files'].keys(),'P1107 inherited-v20 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1107 preserved adopted-v20 data: '+name)
    return count
