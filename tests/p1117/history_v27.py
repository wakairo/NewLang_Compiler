"""Append-only C212 fixture actual-return and direct IF successor of preserved v26."""
from pathlib import Path
import sys
V26='tests/p990/current-freeze-v26.json'
SHA256='f7d52ed08f241fa841e83382fa02f13c9ab7c1e3c18919c4fe1bb199e8f9b2b3'
OID='bc6219d2ef38b6a94ec3b3acb9df062815318f4e'
def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1117'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v26 import verify as previous_verify
    from versioned_freeze import digest,git_oid,local,manifest_data,json_data,require
    count=previous_verify(root,current)
    raw=local(root,V26).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed first-v26 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed first-v26 Git blob')
    old=json_data(raw)
    require(old['generation']==26 and len(old['files'])==3162,'P1117 first-v26 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=27 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v26 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved first-v26 data: '+name)
    return count
