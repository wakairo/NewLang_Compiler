"""Append-only C212 mechanical successor of the preserved first P1117 v22."""
from pathlib import Path
import sys
V22='tests/p990/current-freeze-v22.json'
SHA256='1cb06b118e7df294d1ec0303ea4d01884371e4bd9a855e19399732364f50b0e9'
OID='2004909972e2b07bb67b49ef5327149e8775607f'
def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1117'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v22 import verify as previous_verify
    from versioned_freeze import digest,git_oid,local,manifest_data,json_data,require
    count=previous_verify(root,current)
    raw=local(root,V22).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed first-v22 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed first-v22 Git blob')
    old=json_data(raw)
    require(old['generation']==22 and len(old['files'])==3124,'P1117 first-v22 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=23 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v22 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved first-v22 data: '+name)
    return count
