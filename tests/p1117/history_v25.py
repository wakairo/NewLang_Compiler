"""Append-only C212 fixture API-name successor of preserved v24."""
from pathlib import Path
import sys
V24='tests/p990/current-freeze-v24.json'
SHA256='24f86ffd74c471574f7c461c8b08e7a29380f2e48c590e2058023bb5b0efb592'
OID='9bd2b7a8af62465dd99611d39b26b5712a6d3ddd'
def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1117'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v24 import verify as previous_verify
    from versioned_freeze import digest,git_oid,local,manifest_data,json_data,require
    count=previous_verify(root,current)
    raw=local(root,V24).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed first-v24 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed first-v24 Git blob')
    old=json_data(raw)
    require(old['generation']==24 and len(old['files'])==3144,'P1117 first-v24 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=25 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v24 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved first-v24 data: '+name)
    return count
