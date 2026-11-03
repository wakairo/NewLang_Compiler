"""Append-only C212 legacy integration oracle successor of preserved v29."""
from pathlib import Path
import sys
V29='tests/p990/current-freeze-v29.json'
SHA256='8648835e97ce3014dbe59bbd7b83ff8db3a84392aae11a38cac8104841622148'
OID='e16dbd26dadf5e947461d15999287e35b2d9401a'
def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1117'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v29 import verify as previous_verify
    from versioned_freeze import digest,git_oid,local,manifest_data,json_data,require
    count=previous_verify(root,current)
    raw=local(root,V29).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed first-v29 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed first-v29 Git blob')
    old=json_data(raw)
    require(old['generation']==29 and len(old['files'])==3196,'P1117 first-v29 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=30 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v29 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved first-v29 data: '+name)
    return count
