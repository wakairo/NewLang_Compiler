"""Append-only C212 legacy integration oracle successor of preserved v27."""
from pathlib import Path
import sys
V27='tests/p990/current-freeze-v27.json'
SHA256='26703f29e5715f8b85c9e4bc35c359a340aed00a1bfc124141b832c5d95a590f'
OID='98ce470951f3d45722693b236426420184fb51bd'
def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1117'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v27 import verify as previous_verify
    from versioned_freeze import digest,git_oid,local,manifest_data,json_data,require
    count=previous_verify(root,current)
    raw=local(root,V27).read_bytes()
    require(digest(raw)==SHA256,'P1117 fixed first-v27 SHA256')
    require(git_oid('blob',raw)==OID,'P1117 fixed first-v27 Git blob')
    old=json_data(raw)
    require(old['generation']==27 and len(old['files'])==3176,'P1117 first-v27 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=28 and old['files'].keys()<=value['files'].keys(),'P1117 inherited-v27 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'P1117 preserved first-v27 data: '+name)
    return count
