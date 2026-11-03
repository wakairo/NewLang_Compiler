"""Append-only C212 fixture CLI stage-status successor of preserved v25."""
from pathlib import Path
import sys
V25='tests/p990/current-freeze-v25.json'
SHA256='eddec40a8b2bed0ff78d4b249a718eefecd924f4c39dabdf116caa7bd114e1be'
OID='59e318822faa02a19eafcf6d39b126b94d4ec4bd'
def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1117'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v25 import verify as previous_verify
    from versioned_freeze import digest,git_oid,local,manifest_data,json_data,require
    count=previous_verify(root,current)
    raw=local(root,V25).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed first-v25 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed first-v25 Git blob')
    old=json_data(raw)
    require(old['generation']==25 and len(old['files'])==3153,'P1117 first-v25 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=26 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v25 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved first-v25 data: '+name)
    return count
