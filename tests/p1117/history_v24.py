"""Append-only C212 fixture entry-API successor of preserved v23."""
from pathlib import Path
import sys
V23='tests/p990/current-freeze-v23.json'
SHA256='1b05cf3c2756839811395fabbdd877df7a7fb495d6230c5f2169495282caec9a'
OID='06dc183f02095ff7cf43c819bc72ea97490ddde5'
def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1117'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v23 import verify as previous_verify
    from versioned_freeze import digest,git_oid,local,manifest_data,json_data,require
    count=previous_verify(root,current)
    raw=local(root,V23).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed first-v23 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed first-v23 Git blob')
    old=json_data(raw)
    require(old['generation']==23 and len(old['files'])==3134,'P1117 first-v23 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=24 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v23 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved first-v23 data: '+name)
    return count
