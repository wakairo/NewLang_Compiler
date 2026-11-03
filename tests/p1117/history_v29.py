"""Append-only C212 legacy integration oracle successor of preserved v28."""
from pathlib import Path
import sys
V28='tests/p990/current-freeze-v28.json'
SHA256='cd5c42cfb28fded70d036ce76601ecf02c9cb77e1cffe75c4587466b1544f978'
OID='40522f32178af8407284eda44ab7535341b3b5a0'
def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1117'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v28 import verify as previous_verify
    from versioned_freeze import digest,git_oid,local,manifest_data,json_data,require
    count=previous_verify(root,current)
    raw=local(root,V28).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed first-v28 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed first-v28 Git blob')
    old=json_data(raw)
    require(old['generation']==28 and len(old['files'])==3186,'P1117 first-v28 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=29 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v28 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved first-v28 data: '+name)
    return count
